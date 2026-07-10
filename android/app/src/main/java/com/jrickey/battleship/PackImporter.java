package com.jrickey.battleship;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Locale;
import java.util.zip.ZipFile;

/**
 * Imports a hi-res texture pack (.zip) from a SAF content URI into the app's
 * own mods/ directory under externalFilesDir.
 *
 * Why copy at all: under scoped storage the native HiResPack scanner can only
 * read files the app itself owns. A pack adb-pushed or dropped by a file
 * manager into Android/data/&lt;pkg&gt;/ is owned by another uid and the scan
 * fails with "Permission denied". Copying the picked stream here makes the
 * file app-owned, so {@code HiResPack::Init} reads it (in place, no extraction)
 * on the next game launch.
 *
 * Security/robustness (mirrors the v1.3 Android pass on RomImporter/BootActivity):
 *   - The pack is hundreds of MB, so the stream goes STRAIGHT to its final
 *     mods/ path — never staged into cacheDir and copied again. Bytes flow in
 *     256 KB chunks, so peak heap is bounded regardless of pack size.
 *   - Written to a "&lt;name&gt;.zip.part" temp and renamed only after it
 *     validates, so an interrupted/torn copy never leaves a half-written .zip
 *     the scanner would trip on. Every failure path deletes the partial — no
 *     orphaned hundred-MB files.
 *   - A full disk (write IOException) fails fast instead of publishing a
 *     truncated archive.
 *   - The SAF display name is sanitized to a bare basename (no path
 *     separators / traversal) and forced to end in .zip.
 *   - The copied file is untrusted: it must open as a real ZIP with at least
 *     one entry before it's published. Extension/MIME are not trusted.
 *   - A size cap rejects an absurd download (DoS / accidental wrong file).
 */
public final class PackImporter {
    private static final String TAG = "ssb64.pack";

    /** Hard ceiling on an imported pack. Real packs are ~200 MB; this only
     *  guards against an accidental wrong file or a malicious huge archive. */
    private static final long MAX_PACK_BYTES = 1024L * 1024L * 1024L; // 1 GB

    /**
     * Copy the picked zip into &lt;externalFilesDir&gt;/mods/. Heavy I/O — call
     * from a background thread.
     *
     * @return the published File on success, or null on any failure (already
     *         logged under tag "ssb64.pack"; the partial is cleaned up).
     */
    public static File importPack(Context ctx, Uri uri) {
        if (uri == null) {
            return null;
        }
        try (InputStream in = ctx.getContentResolver().openInputStream(uri)) {
            if (in == null) {
                Log.e(TAG, "openInputStream returned null for " + uri);
                return null;
            }
            return copyToMods(ctx, in, queryDisplayName(ctx, uri));
        } catch (IOException e) {
            Log.e(TAG, "open content uri failed", e);
            return null;
        }
    }

    /**
     * Dev/test path: import a pack from an absolute file the app can already
     * read (e.g. an adb-pushed /sdcard/Download/*.zip). Mirrors RomImporter's
     * dev_rom shortcut; not part of the user-facing SAF flow.
     */
    public static File importPackFromFile(Context ctx, File src) {
        if (src == null || !src.canRead()) {
            Log.e(TAG, "dev pack not readable: " + src);
            return null;
        }
        try (InputStream in = new FileInputStream(src)) {
            return copyToMods(ctx, in, src.getName());
        } catch (IOException e) {
            Log.e(TAG, "open dev pack failed", e);
            return null;
        }
    }

    /** Stream {@code in} into mods/&lt;safe&gt;.zip via a validated temp+rename. */
    private static File copyToMods(Context ctx, InputStream in, String displayName) {
        File extDir = ctx.getExternalFilesDir(null);
        if (extDir == null) {
            Log.e(TAG, "externalFilesDir is null — external storage not mounted?");
            return null;
        }
        File modsDir = new File(extDir, "mods");
        if (!modsDir.isDirectory() && !modsDir.mkdirs()) {
            Log.e(TAG, "could not create mods dir " + modsDir);
            return null;
        }

        File dst = new File(modsDir, safeZipName(displayName));
        File tmp = new File(modsDir, dst.getName() + ".part");

        long total = 0;
        try (OutputStream out = new FileOutputStream(tmp)) {
            byte[] buf = new byte[256 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) {
                total += n;
                if (total > MAX_PACK_BYTES) {
                    // Treated as a copy failure so the single catch cleans up.
                    throw new IOException("pack exceeds " + MAX_PACK_BYTES + "-byte cap");
                }
                out.write(buf, 0, n);
            }
        } catch (IOException e) {
            Log.e(TAG, "copy failed (full disk / read error / too large?)", e);
            tmp.delete();
            return null;
        }

        // Validate (untrusted input — confirm it parses as a ZIP with >=1 entry)
        // and publish. The `published` flag + finally guarantees the .part temp
        // is removed on every exit short of a successful publish — including a
        // Throwable (e.g. OutOfMemoryError raised inside ZipFile on a hostile
        // archive), which would otherwise leave a ~200 MB orphan.
        boolean published = false;
        try {
            if (!isValidZip(tmp)) {
                Log.e(TAG, "rejected " + total + " bytes: not a valid .zip archive");
                return null;
            }
            // rename(2) on the same filesystem (tmp and dst share mods/)
            // atomically replaces any existing same-named pack — no
            // delete-then-rename window that could leave mods/ with neither the
            // old nor the new pack. Fall back to delete+rename only if a strict
            // renameTo refuses to overwrite.
            if (!tmp.renameTo(dst)) {
                dst.delete();
                if (!tmp.renameTo(dst)) {
                    throw new IOException("rename " + tmp + " -> " + dst + " failed");
                }
            }
            published = true;
            Log.i(TAG, "imported " + total + " bytes -> " + dst);
            return dst;
        } catch (IOException e) {
            Log.e(TAG, "publish failed", e);
            return null;
        } finally {
            if (!published) {
                tmp.delete();
            }
        }
    }

    /** Best-effort SAF display name (OpenableColumns.DISPLAY_NAME); null on miss. */
    private static String queryDisplayName(Context ctx, Uri uri) {
        try (Cursor c = ctx.getContentResolver().query(
                uri, new String[] { OpenableColumns.DISPLAY_NAME },
                null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) {
                    return c.getString(idx);
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "display-name query failed; using fallback name", e);
        }
        return null;
    }

    /**
     * Turn a SAF display name into a safe basename ending in .zip. Strips any
     * path components (a content provider could hand back "../x" or "a/b") and
     * restricts to a conservative charset so nothing escapes mods/.
     */
    static String safeZipName(String displayName) {
        String base = (displayName == null) ? "" : displayName;
        int cut = Math.max(base.lastIndexOf('/'), base.lastIndexOf('\\'));
        if (cut >= 0) {
            base = base.substring(cut + 1);
        }
        base = base.replaceAll("[^A-Za-z0-9 ._-]", "_").trim();
        // Also reject names that are nothing but the extension (e.g. a SAF name
        // of " .zip" trims to ".zip", which would land as a hidden file).
        if (base.isEmpty() || base.equals(".") || base.equals("..")
                || base.equalsIgnoreCase(".zip")) {
            base = "pack";
        }
        if (!base.toLowerCase(Locale.ROOT).endsWith(".zip")) {
            base = base + ".zip";
        }
        if (base.length() > 100) {
            base = base.substring(0, 96) + ".zip";
        }
        return base;
    }

    /** True if {@code f} opens as a ZIP with at least one entry. */
    private static boolean isValidZip(File f) {
        try (ZipFile zf = new ZipFile(f)) {
            return zf.size() > 0;
        } catch (IOException e) {
            return false;
        }
    }

    private PackImporter() { /* static */ }
}
