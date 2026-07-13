// app/build.gradle.kts — the BattleShip APK module.
//
// Wires AGP's externalNativeBuild to the existing repo-root CMakeLists.txt,
// which produces libmain.so + libSDL2.so for arm64-v8a. AGP scans the CMake
// output dir for SHARED libs and bundles them under jniLibs/<ABI>/ in the
// APK automatically.

import java.util.Properties

plugins {
    id("com.android.application")
}

// Optional release signing config. Reads from keystore.properties (rooted
// at android/keystore.properties, gitignored — see keystore.properties.example
// for schema) first, then env vars for CI. Returns null if neither is
// configured; the release buildType falls back to the debug signingConfig
// in that case so `./gradlew assembleRelease` works locally without setup.
//
// Production releases:
//   1. Generate keystore once:
//        keytool -genkey -v -keystore release.jks \
//                -alias ssb64-release -keyalg RSA -keysize 2048 -validity 10000
//   2. Either set env vars in your shell:
//        export SSB64_RELEASE_KEYSTORE=/path/to/release.jks
//        export SSB64_RELEASE_KEYSTORE_PASSWORD=...
//        export SSB64_RELEASE_KEY_ALIAS=ssb64-release
//        export SSB64_RELEASE_KEY_PASSWORD=...
//      or copy keystore.properties.example to keystore.properties and fill in.
//   3. ./gradlew bundleRelease  → AAB at app/build/outputs/bundle/release/
val keystoreProps = Properties()
run {
    val f = rootProject.file("keystore.properties")
    if (f.exists()) {
        f.inputStream().use { keystoreProps.load(it) }
    }
}
fun signingProp(key: String, env: String): String? =
    keystoreProps.getProperty(key) ?: System.getenv(env)
val haveReleaseKeystore: Boolean =
    signingProp("storeFile", "SSB64_RELEASE_KEYSTORE") != null

// Repo root = the outer git checkout, two levels up from the agent
// worktree's android/app/build.gradle.kts:
//   <repo>/.claude/worktrees/<slug>/android/app/build.gradle.kts
// Resolve relative to projectDir so it works from any worktree.
val repoRoot = rootProject.projectDir.resolve("..").canonicalFile

/** Resolve a generated asset by checking the desktop build dir first
 *  (where new-worktree.sh symlinks main-tree outputs into build/), then
 *  the repo root, then fail with a clear message. */
fun findGameArtifact(name: String): java.io.File {
    val candidates = listOf(
        repoRoot.resolve("build").resolve(name),
        repoRoot.resolve(name),
    )
    return candidates.firstOrNull { it.exists() }
        ?: error("Missing $name — looked in:\n" +
                 candidates.joinToString("\n") { "  $it" } +
                 "\nRun the desktop cmake build (targets: GenerateF3DO2R, " +
                 "GeneratePortO2R, ExtractAssets) before assembleDebug.")
}

// Stage f3d.o2r + Torch's runtime config (config.yml + yamls/) into a
// generated dir that AGP merges into the APK's assets/.
//
// NEVER bundle anything ROM-derived. BattleShip.o2r is produced on-device
// from the user's own ROM at first launch (Phase 4.4 SAF picker →
// libtorch_runner.so → BattleShip.o2r in externalFilesDir). f3d.o2r is
// open-source Fast3D shaders, config.yml + yamls/ are layout metadata —
// none of those contain Nintendo content, so they're safe in the APK.
val stagedAssetsDir = layout.buildDirectory.dir("generated/staged_assets")

val stageGameAssets = tasks.register<Copy>("stageGameAssets") {
    description = "Stage f3d.o2r / config.yml / yamls into APK assets/"
    group = "android"
    into(stagedAssetsDir)

    // Resolved at task-config time so we fail before AGP even starts the
    // merge — avoids a half-built APK with missing assets.
    from(findGameArtifact("f3d.o2r"))
    from(repoRoot.resolve("config.yml"))
    from(repoRoot.resolve("yamls")) {
        into("yamls")
    }

    doFirst {
        require(repoRoot.resolve("config.yml").exists()) {
            "config.yml not found at $repoRoot — agent worktree should " +
            "always have this file from git"
        }
    }
}

// Run the staging task before any AGP task that consumes assets. The
// merge*Assets tasks are the obvious one; AGP 8 + Gradle 9 also tracks
// asset inputs from the lint-vital-report-model task, so we have to
// declare the dependency there too or `assembleRelease` fails with
// "Reason: Task ':app:generateReleaseLintVitalReportModel' uses this
// output of task ':app:stageGameAssets' without declaring an explicit
// or implicit dependency."
afterEvaluate {
    tasks.matching { task ->
        (task.name.startsWith("merge") && task.name.endsWith("Assets"))
        || task.name.contains("Lint")
        || task.name.startsWith("packageDebug")
        || task.name.startsWith("packageRelease")
    }.configureEach { dependsOn(stageGameAssets) }
}

android {
    namespace = "com.jrickey.battleship"
    compileSdk = 34

    // BuildConfig.VERSION_CODE is referenced from AssetExtractor.java for
    // re-extraction sentinel comparison. AGP 8.x disables this by default.
    buildFeatures {
        buildConfig = true
    }
    // NDK install is at /opt/homebrew/share/android-ndk via brew; AGP needs
    // ndkVersion to match the actual revision (read from source.properties)
    // or it errors out. r29 = 29.0.14206865.
    ndkVersion = "29.0.14206865"

    defaultConfig {
        applicationId = "com.jrickey.battleship"
        // Default floor is Android 7.0 (API 24) — the lowest this codebase can
        // build for on 32-bit ARM. libc++ <fstream> (pulled in via libultraship)
        // calls ::fseeko/::ftello, which Bionic only declares at API >= 24 on
        // ILP32; below that the native build fails to compile. This still drops
        // the floor from the old Android 8.0 (API 26) so pre-8.0 devices can
        // install. The former API-26 dependency (forced AAudio hint) is now
        // gated at runtime in port.cpp (OpenSL ES below 26). Override with
        // -Pssb64.minSdk=<n>; AGP forwards it into the CMake Android platform.
        val ssb64MinSdk = (project.findProperty("ssb64.minSdk") as String?)?.toInt() ?: 24
        minSdk = ssb64MinSdk
        targetSdk = 34       // Play Store requires >= 34 (since Aug 2024)
        versionCode = 1
        versionName = "0.1.0-spike"

        // ABIs to build. Default arm64-v8a (covers ~99% of in-use devices and
        // runs natively on the M-series emulator). Override at the command line
        // with -Pssb64.abis=armeabi-v7a (or a comma-separated list, e.g.
        // "arm64-v8a,armeabi-v7a") to build 32-bit ARM for older devices and
        // cheap Android TV boxes. See docs/android_armeabi_v7a_2026-06-20.md.
        // Add x86_64 if you need Intel-host emulators.
        val ssb64Abis = (project.findProperty("ssb64.abis") as String?)
            ?.split(",")?.map { it.trim() }?.filter { it.isNotEmpty() }
            ?: listOf("arm64-v8a")
        ndk {
            abiFilters += ssb64Abis
        }

        externalNativeBuild {
            cmake {
                // Forwarded to CMake configure. -DUSE_AUTO_VCPKG=OFF stops
                // libultraship from trying to bootstrap vcpkg on Android.
                arguments += listOf(
                    "-DUSE_AUTO_VCPKG=OFF",
                    "-DANDROID_STL=c++_shared",
                )
                // Targets we explicitly want AGP to drive a build for.
                //   ssb64         — libmain.so (the Activity loads this)
                //   torch_runner  — libtorch_runner.so (first-run ROM
                //                   extractor, loaded standalone by Java
                //                   before SDLActivity is up)
                // Transitive deps (libultraship.a, libSDL2.so, libtinyxml2,
                // libtorch.a, etc.) get pulled in automatically — and only
                // SHARED libs end up in the APK.
                targets += listOf("ssb64", "torch_runner")
            }
        }
    }

    signingConfigs {
        if (haveReleaseKeystore) {
            create("release") {
                storeFile     = file(signingProp("storeFile", "SSB64_RELEASE_KEYSTORE")!!)
                storePassword = signingProp("storePassword", "SSB64_RELEASE_KEYSTORE_PASSWORD")
                keyAlias      = signingProp("keyAlias",      "SSB64_RELEASE_KEY_ALIAS")
                keyPassword   = signingProp("keyPassword",   "SSB64_RELEASE_KEY_PASSWORD")
            }
        }
    }

    buildTypes {
        getByName("debug") {
            isJniDebuggable = true
            // Pure-native crashes need the unstripped .so to symbolicate;
            // packagingOptions below keeps debug symbols around in debug APKs.
            isMinifyEnabled = false
            applicationIdSuffix = ".debug"
            versionNameSuffix = "-debug"
        }
        getByName("release") {
            isMinifyEnabled = false   // No Java code worth shrinking yet
            isShrinkResources = false
            // Use the configured release keystore if present; otherwise the
            // debug key. The latter still produces a runnable APK for local
            // smoke-testing — it's just NOT distributable (Play Store
            // rejects debug-keyed builds).
            signingConfig = if (haveReleaseKeystore) {
                signingConfigs.getByName("release")
            } else {
                signingConfigs.getByName("debug")
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    externalNativeBuild {
        cmake {
            // Path is resolved relative to this build.gradle.kts:
            //   android/app/ → ../.. → repo root
            path = file("../../CMakeLists.txt")
            // Match the cmake_minimum_required in the root.
            version = "3.24.0+"
        }
    }

    // Stage host-built / source files into a generated assets dir, then
    // tell AGP to merge that dir into the APK assets/. The dir lives under
    // app/build/ so it gets cleaned with `./gradlew clean`.
    sourceSets {
        getByName("main") {
            assets.srcDirs("$buildDir/generated/staged_assets")
        }
    }

    packaging {
        // (sourceSets is configured above)
        // Keep our prebuilt SDL2's debug info in debug APKs so ndk-stack can
        // symbolicate native crashes. release APK strips by default.
        jniLibs {
            keepDebugSymbols += "**/*.so"
        }
        resources {
            // libultraship and SDL2 each ship a META-INF — collide harmlessly,
            // pick one.
            pickFirsts += listOf(
                "META-INF/LICENSE*",
                "META-INF/NOTICE*",
                "META-INF/AL2.0",
                "META-INF/LGPL2.1",
            )
        }
    }
}

// Force a single Kotlin stdlib version. AndroidX 1.9.3's transitive
// kotlin-stdlib-jdk8 1.6.21 collides with AGP's bundled kotlin-stdlib
// 1.8.22 (the older split stdlib was merged in 1.8). Pin both halves
// to 1.8.22 so dexer doesn't see duplicate classes.
configurations.all {
    resolutionStrategy.eachDependency {
        if (requested.group == "org.jetbrains.kotlin"
            && requested.name.startsWith("kotlin-stdlib")) {
            useVersion("1.9.25")
        }
    }
}

dependencies {
    // BootActivity uses the AndroidX result-API (registerForActivityResult)
    // for the SAF ROM picker. ComponentActivity is the base class that
    // wires the result-callback machinery into the Activity lifecycle.
    implementation("androidx.activity:activity:1.9.3")
}
