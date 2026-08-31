package com.jrickey.battleship;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.ComponentActivity;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;

import java.io.File;

/**
 * BootActivity — launcher entry, drives first-run setup.
 *
 * Stages, in order:
 *   1. Extract bundled APK assets (f3d.o2r, config.yml, yamls/) into
 *      externalFilesDir if absent or stale — see {@link AssetExtractor}.
 *   2. If BattleShip.o2r doesn't exist, prompt the user to pick a ROM
 *      via SAF ACTION_OPEN_DOCUMENT, stream it into cacheDir, and run
 *      libtorch_runner.so against it ({@link RomImporter}).
 *   3. Launch BattleShipActivity (the SDLActivity subclass) once
 *      BattleShip.o2r is in place.
 *
 * Heavy work runs on background threads so the UI stays responsive
 * during the multi-second extraction.
 */
public class BootActivity extends ComponentActivity {
    private static final String TAG = "ssb64.boot";

    /** Phase number shown in error UIs to make state self-describing. */
    private static final String PHASE = "Phase 4.4";

    private TextView mStatus;
    private Button   mPickButton;

    /** Result handler for the SAF picker. Registered before super.onCreate exits. */
    private final ActivityResultLauncher<String[]> mPickRom
        = registerForActivityResult(
            new ActivityResultContracts.OpenDocument(),
            this::onRomPicked);

    /** Result handler for the hi-res-pack SAF picker (import-pack flow).
     *  Registered eagerly so it survives Activity recreation mid-pick. */
    private final ActivityResultLauncher<String[]> mPickPack
        = registerForActivityResult(
            new ActivityResultContracts.OpenDocument(),
            this::onPackPicked);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        super.onCreate(savedInstanceState);
        buildUi();
        startAssetExtraction();
    }

    /**
     * Dev/test affordance: bypass the SAF picker and run extraction against
     * a known absolute path. Trigger with:
     *   adb shell am start -n com.jrickey.battleship.debug/.BootActivity \
     *                       --es ssb64.dev_rom /sdcard/Download/baserom.us.z64
     * Only meaningful in debug builds and only for files the app already
     * has read access to (typically /sdcard/Download after MEDIA permission
     * — but adb-pushed files there are world-readable on emulator images).
     */
    private static final String EXTRA_DEV_ROM = "ssb64.dev_rom";

    /**
     * If true on the launching Intent, BootActivity deletes the existing
     * BattleShip.o2r before evaluating the assets-ready check — forcing
     * the SAF picker (or dev_rom shortcut) to run again. Used to swap
     * a different region/version of the ROM without wiping app data.
     *
     *   adb shell am start -n com.jrickey.battleship.debug/.BootActivity \
     *                       --ez ssb64.repick true
     *
     * Or via the launcher long-press shortcut "Re-extract ROM"
     * (defined in res/xml/shortcuts.xml).
     */
    private static final String EXTRA_REPICK = "ssb64.repick";

    /**
     * If true on the launching Intent, BootActivity shows a SAF picker for a
     * hi-res texture pack (.zip) after assets are ready, copies it into the
     * app-owned mods/ folder ({@link PackImporter}), then continues to the
     * normal route. Surfaced via the "Import texture pack" launcher shortcut
     * (res/xml/shortcuts.xml); also drivable for dev/testing:
     *
     *   adb shell am start -n com.jrickey.battleship.debug/.BootActivity \
     *                       --ez ssb64.import_pack true
     *
     * Import runs BEFORE the SDL game starts, so HiResPack's scan sees the new
     * pack on a single thread at boot — no rescan race against the renderer.
     */
    private static final String EXTRA_IMPORT_PACK = "ssb64.import_pack";

    private boolean isImportPackRequest() {
        return getIntent() != null
            && getIntent().getBooleanExtra(EXTRA_IMPORT_PACK, false);
    }

    /**
     * Dev/test: import a pack from an absolute path the app can read, skipping
     * the SAF picker. Mirrors {@link #EXTRA_DEV_ROM}.
     *   adb shell am start -n com.jrickey.battleship.debug/.BootActivity \
     *                       --es ssb64.dev_pack /sdcard/Download/pack.zip
     */
    private static final String EXTRA_DEV_PACK = "ssb64.dev_pack";

    private void buildUi() {
        mStatus = new TextView(this);
        mStatus.setGravity(Gravity.CENTER);
        mStatus.setPadding(64, 64, 64, 32);
        mStatus.setTextSize(18f);
        mStatus.setText("Preparing assets…");

        mPickButton = new Button(this);
        mPickButton.setText("Choose your SSB64 ROM (.z64)");
        mPickButton.setVisibility(View.GONE);
        mPickButton.setOnClickListener(v -> launchRomPicker());

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER);
        root.setPadding(32, 32, 32, 32);
        root.addView(mStatus);
        root.addView(mPickButton);
        setContentView(root);
    }

    /* ===================================================================== */
    /*  Stage 1: bundled-asset extraction                                    */
    /* ===================================================================== */

    private void startAssetExtraction() {
        // Honor the --ez ssb64.repick true Intent extra: delete the existing
        // BattleShip.o2r so routeAfterAssets() falls into the SAF picker
        // path again. Bundled assets (config.yml / yamls / f3d.o2r) stay
        // intact — those don't depend on the user's ROM.
        boolean repick = getIntent() != null
            && getIntent().getBooleanExtra(EXTRA_REPICK, false);
        if (repick) {
            File extDir = getExternalFilesDir(null);
            if (extDir != null) {
                File rom = new File(extDir, "BattleShip.o2r");
                if (rom.exists() && !rom.delete()) {
                    Log.w(TAG, "Couldn't delete BattleShip.o2r for repick");
                }
            }
        }

        new Thread(() -> {
            String err = AssetExtractor.extractIfNeeded(getApplicationContext());
            runOnUi(() -> {
                if (err != null) {
                    showError("Asset extraction failed:\n\n" + err);
                    return;
                }
                // Debug-only: BootActivity is the exported launcher entry, so a
                // release build must not let another app drive an arbitrary
                // absolute-path import through this extra (confused deputy).
                String devPack = (BuildConfig.DEBUG && getIntent() != null)
                    ? getIntent().getStringExtra(EXTRA_DEV_PACK) : null;
                if (devPack != null && !devPack.isEmpty()) {
                    importDevPack(new File(devPack));
                    return;
                }
                if (isImportPackRequest()) {
                    showPackPicker();
                    return;
                }
                routeAfterAssets();
            });
        }, "ssb64-boot-extract").start();
    }

    private void routeAfterAssets() {
        if (AssetExtractor.haveExtractedRom(this)) {
            startGame();
            return;
        }

        // Dev shortcut: --es ssb64.dev_rom <abs path> → run extractor on
        // that path instead of opening the SAF picker. Survives the
        // assets-already-extracted fast path above; if the user wants to
        // re-extract from a dev rom, they can wipe BattleShip.o2r first.
        String devRom = getIntent() != null ? getIntent().getStringExtra(EXTRA_DEV_ROM) : null;
        if (devRom != null && !devRom.isEmpty()) {
            extractFromAbsolutePath(new File(devRom));
            return;
        }

        showRomPicker();
    }

    /** Common extraction worker, called from both SAF result and dev shortcut. */
    private void extractFromAbsolutePath(File romFile) {
        extractFromAbsolutePath(romFile, false);
    }

    /**
     * @param deleteAfter delete romFile once extraction finishes (used for
     *        the SAF-staged copy in cacheDir — the picker UI promises the
     *        ROM is discarded, and cacheDir is otherwise only reclaimed
     *        under storage pressure). Dev-shortcut ROMs pass false.
     */
    private void extractFromAbsolutePath(File romFile, boolean deleteAfter) {
        if (!romFile.canRead()) {
            showError("Can't read ROM at " + romFile + " — check permissions.");
            return;
        }
        mPickButton.setVisibility(View.GONE);
        mStatus.setText("Extracting ROM into BattleShip.o2r — this can take a few seconds…");

        new Thread(() -> {
            File extDir = getExternalFilesDir(null);
            int rc = RomImporter.extractO2R(
                romFile.getAbsolutePath(),
                extDir.getAbsolutePath(),
                extDir.getAbsolutePath());

            if (deleteAfter && !romFile.delete()) {
                Log.w(TAG, "Could not delete staged ROM " + romFile);
            }

            if (rc != 0) {
                final String msg;
                switch (rc) {
                    case 3:
                        // Torch bailed before configuring output — in practice
                        // an unrecognized ROM dump (SHA-1 not in config.yml).
                        msg = "That ROM isn't a supported dump. BattleShip needs "
                            + "Super Smash Bros. (U) v1.0 or the Japanese release, "
                            + "in native big-endian .z64 format from a known-good "
                            + "cartridge dump. Pick a different ROM file.";
                        break;
                    case 4:
                        msg = "Couldn't write BattleShip.o2r — free up storage "
                            + "space and try again.";
                        break;
                    default:
                        msg = "ROM extraction failed (code " + rc + "). Logcat "
                            + "tag ssb64.torch has the detailed error.";
                        break;
                }
                runOnUi(() -> {
                    showError(msg);
                    // A bad ROM pick is recoverable — let the user try another
                    // file instead of dead-ending on the error text.
                    if (rc == 3) mPickButton.setVisibility(View.VISIBLE);
                });
                return;
            }
            if (!AssetExtractor.haveExtractedRom(BootActivity.this)) {
                // Should be unreachable now that the bridge verifies the
                // archive on disk before returning 0; kept as a last-resort
                // assertion.
                runOnUi(() -> showError(
                    "Torch returned success but BattleShip.o2r is not in " +
                    extDir));
                return;
            }
            runOnUi(() -> {
                mStatus.setText("Done — launching game…");
                startGame();
            });
        }, "ssb64-rom-extract").start();
    }

    /* ===================================================================== */
    /*  Stage 2: ROM picker → libtorch_runner.so → BattleShip.o2r            */
    /* ===================================================================== */

    private void showRomPicker() {
        mStatus.setText(
            "Welcome!\n\n" +
            "BattleShip needs a copy of the original Super Smash Bros. (US) " +
            "ROM to extract its assets. Provide your own dump — Nintendo's " +
            "ROM is not distributed with the app.\n\n" +
            "After you pick the file, the assets stay on your device and the " +
            "ROM is discarded."
        );
        mPickButton.setVisibility(View.VISIBLE);
    }

    private void launchRomPicker() {
        // application/octet-stream lets us see .z64/.n64/.v64 (none have
        // a registered MIME type, so we accept everything and validate
        // by header inside Torch).
        mPickRom.launch(new String[] { "*/*" });
    }

    private void onRomPicked(Uri romUri) {
        if (romUri == null) {
            // User cancelled — leave the picker visible.
            return;
        }
        mPickButton.setVisibility(View.GONE);
        mStatus.setText("Staging ROM…");

        new Thread(() -> {
            File staged = RomImporter.stageRomFromUri(getApplicationContext(), romUri);
            if (staged == null) {
                runOnUi(() -> showError(
                    "Couldn't read the ROM file you picked.\n\n" +
                    "Try a different copy or use a file that's already saved " +
                    "to your device storage."));
                return;
            }
            // Hand off to the shared extraction worker. It posts to the UI
            // thread itself, so we can safely call from this background.
            runOnUi(() -> extractFromAbsolutePath(staged, true));
        }, "ssb64-rom-stage").start();
    }

    /* ===================================================================== */
    /*  Optional: hi-res texture pack import (ssb64.import_pack)              */
    /* ===================================================================== */

    private void showPackPicker() {
        mStatus.setText(
            "Import a hi-res texture pack.\n\n" +
            "Pick the pack's .zip file. It's copied into the app's mods folder " +
            "and applied the next time the game loads textures."
        );
        mPickButton.setText("Choose texture pack (.zip)");
        mPickButton.setOnClickListener(v -> launchPackPicker());
        mPickButton.setVisibility(View.VISIBLE);
    }

    private void launchPackPicker() {
        // Bias toward zips but fall back to "*/*" (some providers report a
        // pack as octet-stream); PackImporter validates by actually opening it.
        mPickPack.launch(new String[] { "application/zip", "*/*" });
    }

    private void importDevPack(File packFile) {
        mPickButton.setVisibility(View.GONE);
        mStatus.setText("Importing texture pack (dev)…");
        new Thread(() -> {
            File pack = PackImporter.importPackFromFile(getApplicationContext(), packFile);
            runOnUi(() -> {
                Toast.makeText(BootActivity.this,
                    pack != null
                        ? "Texture pack imported (dev)."
                        : "Dev pack import failed — see logcat tag ssb64.pack.",
                    Toast.LENGTH_LONG).show();
                routeAfterAssets();
            });
        }, "ssb64-pack-import-dev").start();
    }

    private void onPackPicked(Uri packUri) {
        if (packUri == null) {
            // Cancelled — go on to the game/ROM flow without a new pack.
            routeAfterAssets();
            return;
        }
        mPickButton.setVisibility(View.GONE);
        mStatus.setText("Importing texture pack…");

        new Thread(() -> {
            File pack = PackImporter.importPack(getApplicationContext(), packUri);
            runOnUi(() -> {
                // Toast survives the hand-off to the game Activity, so the
                // result is visible even though we route on immediately.
                Toast.makeText(BootActivity.this,
                    pack != null
                        ? "Texture pack imported."
                        : "Couldn't import that file — is it a valid .zip pack?",
                    Toast.LENGTH_LONG).show();
                routeAfterAssets();
            });
        }, "ssb64-pack-import").start();
    }

    /* ===================================================================== */
    /*  Stage 3: hand off to the SDL game Activity                           */
    /* ===================================================================== */

    private void startGame() {
        Log.i(TAG, "Assets ready, launching BattleShipActivity");
        startActivity(new Intent(this, BattleShipActivity.class));
        finish();
    }

    /* ===================================================================== */
    /*  Helpers                                                              */
    /* ===================================================================== */

    private void showError(String message) {
        mStatus.setText("[" + PHASE + "] " + message);
        mPickButton.setVisibility(View.GONE);
        Log.e(TAG, message);
    }

    private void runOnUi(Runnable r) {
        new Handler(Looper.getMainLooper()).post(r);
    }
}
