# Android Build Documentation - spt-tech

This document records the steps taken to resolve persistent build failures and infinite loops in the Android build process for the `spt-tech` project.

## 1. Environment Requirements
- **Qt Version**: 5.15.2 (Android)
- **NDK Version**: r22.1.7171670 (Locked to this version for Qt 5.15 compatibility)
- **JDK Version**: OpenLogic JDK 11 (JDK 16+ is incompatible with the project's Gradle version)
- **Gradle Version**: 5.6.4

## 2. Key Fixes Applied

### A. Build Loop Prevention
The build was entering an infinite loop because the NDK `make` tool saw generated Makefiles as "out-of-date" immediately after creation.
- **Fix**: Added `find . -name "Makefile*" -exec touch {} +` in the build script to align timestamps.
- **Fix**: Added `QMAKE="echo skipping qmake"` to `make` commands to prevent recursive triggering of the Qt metadata generator.

### B. Java & Pathing Issues
- **Space in Path**: Windows paths with spaces (like `C:\Program Files\...`) frequently break Gradle scripts.
- **Fix**: Created a directory junction at `C:\Users\Training\jdk11` pointing to the JDK.
- **Compatibility**: Gradle 5.6.4 requires Java 8 or 11. We installed OpenLogic JDK 11 to resolve Groovy initialization errors.

### C. Linker Indexing
- **Error**: `archive has no index; run ranlib to add one`.
- **Fix**: Added explicit `ranlib` calls for both `armeabi-v7a` and `arm64-v8a` static libraries (`libquazip`, `libgarlic`) right before the final application link.

### D. Offline Gradle Configuration
Due to restricted network access, the automated Gradle download was failing.
- **Fix**: Manually placed `gradle-5.6.4-bin.zip` in `C:\Users\Training\.gradle\wrapper\dists\gradle-5.6.4-bin\bxirm19lnfz6nurbatndyydux\`.
- **Fix**: Created a `.zip.ok` marker file to bypass the download verification check.

### E. Android SDK "Missing DX" Patch
Android Build Tools 31.0.0 removed the `dx` tool in favor of `d8`, but Gradle 5.6.4 specifically looks for `dx`.
- **Fix**: Created compatibility scripts in the SDK folder:
  - Copy `d8.bat` to `dx.bat`
  - Copy `lib/d8.jar` to `lib/dx.jar`

## 3. How to Run the Build
Run the following from the project root in a Git Bash or MSYS2 terminal:

```bash
bash build_scripts/player/2.1_buildAndroid.sh
```

The final APK will be generated at:
`android-build-5.15.2-debug/player-c2qml/android-build/build/outputs/apk/debug/android-build-debug.apk`
