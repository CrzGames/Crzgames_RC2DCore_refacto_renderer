#!/bin/bash
set -euo pipefail

# --------------------------------------------------
# Vérification environnement
# --------------------------------------------------

# JAVA_HOME obligatoire
if [ -z "${JAVA_HOME:-}" ]; then
  echo "Erreur : JAVA_HOME n'est pas défini."
  echo "Exemple : export JAVA_HOME=/usr/lib/jvm/temurin-17-jdk"
  exit 1
fi
# ANDROID_HOME ou ANDROID_SDK_ROOT obligatoire
if [ -z "${ANDROID_HOME:-}" ] && [ -z "${ANDROID_SDK_ROOT:-}" ]; then
  echo "Erreur : ANDROID_HOME ou ANDROID_SDK_ROOT n'est pas défini."
  echo "Exemple : export ANDROID_HOME=~/Library/Android/sdk"
  exit 1
fi

# Generate lib for Android (arm64-v8a + armeabi-v7a)
GRADLE="./gradlew"

# Define base directories
BASE_BUILD_DIR_RELEASE="android-project/app/build/intermediates/cmake/release/obj"
DIST_DIR_RELEASE="dist/lib/android/Release"
BASE_BUILD_DIR_DEBUG="android-project/app/build/intermediates/cmake/debug/obj"
DIST_DIR_DEBUG="dist/lib/android/Debug"

# Create destination directories
mkdir -p "$DIST_DIR_RELEASE/arm64-v8a" "$DIST_DIR_RELEASE/armeabi-v7a"
mkdir -p "$DIST_DIR_DEBUG/arm64-v8a"   "$DIST_DIR_DEBUG/armeabi-v7a"

# Change to project directory
cd android-project

# Ensure wrapper exists
if [ ! -f "gradlew" ]; then
  echo "Erreur : gradlew introuvable dans android-project/"
  exit 1
fi
chmod +x ./gradlew

# Clean and build the project
echo -e "\e[32m\nClean project...\e[0m"
$GRADLE clean

echo -e "\e[32m\nBuild project for Release...\e[0m"
$GRADLE assembleRelease

echo -e "\e[32m\nBuild project for Debug...\e[0m"
$GRADLE assembleDebug

cd ..

# Copy .so files to respective directories
cp "$BASE_BUILD_DIR_RELEASE/arm64-v8a/librc2d.so"   "$DIST_DIR_RELEASE/arm64-v8a/"
cp "$BASE_BUILD_DIR_RELEASE/armeabi-v7a/librc2d.so" "$DIST_DIR_RELEASE/armeabi-v7a/"

cp "$BASE_BUILD_DIR_DEBUG/arm64-v8a/librc2d.so"     "$DIST_DIR_DEBUG/arm64-v8a/"
cp "$BASE_BUILD_DIR_DEBUG/armeabi-v7a/librc2d.so"   "$DIST_DIR_DEBUG/armeabi-v7a/"

echo -e "\e[32m\nLib RC2D for Android >= 9.0 generated successfully. Go to dist/lib/android/\n\e[0m"
