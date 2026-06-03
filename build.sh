#!/usr/bin/env bash
# build.sh — compile NameTags mod and package as .qmod
# Requirements: QPM, Android NDK r23+, zip

set -e

echo "==> Restoring QPM dependencies..."
qpm restore

echo "==> Compiling with NDK..."
# Adjust NDK path if needed
${ANDROID_NDK_HOME:-$NDK_ROOT}/ndk-build \
    NDK_PROJECT_PATH=. \
    APP_BUILD_SCRIPT=./Android.mk \
    NDK_APPLICATION_MK=./Application.mk \
    APP_ABI=arm64-v8a \
    -j$(nproc)

SO_PATH="libs/arm64-v8a/libNameTags.so"

if [ ! -f "$SO_PATH" ]; then
    echo "ERROR: Build failed — $SO_PATH not found."
    exit 1
fi

echo "==> Packaging .qmod..."
cp "$SO_PATH" libNameTags.so

zip -j NameTags.qmod \
    mod.json \
    libNameTags.so

rm libNameTags.so

echo ""
echo "✅  Done! NameTags.qmod is ready."
echo "    Install with: adb push NameTags.qmod /sdcard/ModsBeforeFriday/Mods/"
echo "    or use BMBF / SideQuest."
