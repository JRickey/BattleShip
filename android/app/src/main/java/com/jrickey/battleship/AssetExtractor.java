package com.jrickey.battleship;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

/**
 * Copies bundled APK assets (f3d.o2r, config.yml, yamls/) into the app's
 * externalFilesDir on first launch and on app updates.
 *
 * - externalFilesDir is what LUS's Ship::Context::GetAppDirectoryPath()
 *   returns on Android (via SDL_AndroidGetExternalStoragePath), so files
 *   landed here are reachable through the same path the desktop build
 *   uses for cwd-relative archive lookups (f3d.o2r).
 * - We mark "extracted" by writing a versioned sentinel file. If the
 *   APK's versionCode bumps (i.e. the user updated the app), the sentinel
 *   no longer matches and we re-extract. This handles asset format
 *   changes that ship inside the APK.
 * - BattleShip.o2r is NOT extracted here — that comes from libtorch_runner.so
 *   in Phase 4.4 once the user picks their ROM. NEVER add anything
 *   ROM-derived to the APK assets.
 */
public final class AssetExtractor {
    private static final String TAG = "ssb64.assets";

    /** Sentinel file: contains "<versionCode>:<sha-or-anything>" if extraction matches. */
    private static final String SENTINEL = ".bundled_assets_version";

    private AssetExtractor() { /* static */ }

    /**
     * Extract bundled assets into externalFilesDir if they aren't already
     * up-to-date for this versionCode.
     *
     * @return null on success, or a human-readable error message on failure.
     */
    public static String extractIfNeeded(Context ctx) {
        File dst = ctx.getExternalFilesDir(null);
        if (dst == null) {
            return "externalFilesDir is null — external storage not mounted?";
        }

        String wantedVersion = String.valueOf(BuildConfig.VERSION_CODE);
        File sentinel = new File(dst, SENTINEL);
        if (sentinel.isFile()) {
            try {
                String have = new String(readAll(sentinel),
                                         StandardCharsets.UTF_8).trim();
                if (wantedVersion.equals(have)) {
                    Log.i(TAG, "Bundled assets are current (versionCode=" + wantedVersion + ")");
                    return null;
                }
                Log.i(TAG, "Bundled assets stale (have=" + have + " want=" + wantedVersion + "), re-extracting");
            } catch (IOException ioe) {
                Log.w(TAG, "Couldn't read sentinel, re-extracting: " + ioe.getMessage());
            }
        } else {
            Log.i(TAG, "First-run extraction into " + dst);
        }

        AssetManager am = ctx.getAssets();
        try {
            // Top-level files. Order doesn't matter — they're independent.
            copyAsset(am, "f3d.o2r",    new File(dst, "f3d.o2r"));
            copyAsset(am, "config.yml", new File(dst, "config.yml"));

            // yamls/ subtree — Torch reads these at extraction time to
            // know the ROM's file table layout. Walk the asset directory
            // recursively so we pick up every reloc_*.yml.
            copyAssetTree(am, "yamls", new File(dst, "yamls"));
        } catch (IOException e) {
            Log.e(TAG, "Asset extraction failed", e);
            return "Asset extraction failed: " + e.getMessage();
        }

        try (OutputStream out = new FileOutputStream(sentinel)) {
            out.write(wantedVersion.getBytes(StandardCharsets.UTF_8));
        } catch (IOException ioe) {
            Log.w(TAG, "Couldn't write sentinel — extraction will repeat next launch: " + ioe.getMessage());
            // Not fatal; the assets are extracted and the game can run.
        }

        Log.i(TAG, "Asset extraction complete");
        return null;
    }

    private static byte[] readAll(File f) throws IOException {
        try (InputStream in = new FileInputStream(f)) {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            byte[] buf = new byte[64 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) {
                bos.write(buf, 0, n);
            }
            return bos.toByteArray();
        }
    }

    private static void copyAsset(AssetManager am, String assetPath, File outFile) throws IOException {
        File parent = outFile.getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new IOException("mkdirs " + parent + " failed");
        }
        try (InputStream in = am.open(assetPath);
             OutputStream out = new FileOutputStream(outFile)) {
            byte[] buf = new byte[64 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
        }
    }

    /**
     * Recursively copy an asset directory subtree. AssetManager.list() returns
     * an empty array for files, non-empty for directories — this is the
     * documented way to walk it.
     */
    private static void copyAssetTree(AssetManager am, String assetPath, File outDir) throws IOException {
        String[] entries = am.list(assetPath);
        if (entries == null || entries.length == 0) {
            // It's a leaf file (or empty dir). Treat as file copy.
            copyAsset(am, assetPath, outDir);
            return;
        }
        if (!outDir.isDirectory() && !outDir.mkdirs()) {
            throw new IOException("mkdirs " + outDir + " failed");
        }
        for (String entry : entries) {
            String childAsset = assetPath + "/" + entry;
            File   childOut   = new File(outDir, entry);
            copyAssetTree(am, childAsset, childOut);
        }
    }

    /** Whether BattleShip.o2r already exists in externalFilesDir. Phase 4.4 uses this. */
    public static boolean haveExtractedRom(Context ctx) {
        File dst = ctx.getExternalFilesDir(null);
        return dst != null && new File(dst, "BattleShip.o2r").isFile();
    }
}
