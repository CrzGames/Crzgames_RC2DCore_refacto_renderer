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

# --------------------------------------------------
# Build Android via Gradle + export des libs statiques CMake (.a)
#
# NOTE IMPORTANTE :
# - Android Gradle Plugin génère souvent "RelWithDebInfo" (pas un vrai "Release")
# - Les libs statiques (.a) ne sont PAS dans intermediates/cxx/.../obj
#   mais dans : android-project/app/.cxx/<Config>/<hash>/<abi>/librc2d_static.a
# - Ce script copie les .a vers :
#   build/android/<ABI>/<Config>/librc2d_static.a
# --------------------------------------------------

GRADLE="./gradlew"

# Sortie (alignée avec les autres scripts)
OUT_BASE="build/android"

# --------------------------------------------------
# Aller dans le projet Android
# --------------------------------------------------
cd android-project

# Ensure wrapper exists
if [ ! -f "gradlew" ]; then
  echo "Erreur : gradlew introuvable dans android-project/"
  exit 1
fi
chmod +x ./gradlew

# --------------------------------------------------
# Clean Android project (Gradle)
# --------------------------------------------------
echo -e "\e[32m\nClean project (Gradle)...\e[0m"
$GRADLE clean

# --------------------------------------------------
# Build Gradle
# --------------------------------------------------
echo -e "\e[32m\nBuild project for Release (Gradle)...\e[0m"
$GRADLE assembleRelease

echo -e "\e[32m\nBuild project for Debug (Gradle)...\e[0m"
$GRADLE assembleDebug

cd ..

# --------------------------------------------------
# Trouver les libs statiques générées par CMake via AGP :
# android-project/app/.cxx/<Config>/<hash>/<abi>/librc2d_static.a
#
# - Debug => android-project/app/.cxx/Debug/<hash>/<abi>/...
# - "Release" => souvent android-project/app/.cxx/RelWithDebInfo/<hash>/<abi>/...
# --------------------------------------------------

find_one() {
  local cfg="$1"
  local abi="$2"
  local base="android-project/app/.cxx/${cfg}"
  local found=""

  if [ -d "$base" ]; then
    # Le hash change à chaque génération => on cherche dans tous les sous-dossiers
    found="$(find "$base" -type f -path "*/${abi}/librc2d_static.a" 2>/dev/null | head -n 1 || true)"
  fi

  if [ -z "$found" ]; then
    echo "Erreur : librc2d_static.a introuvable pour cfg='${cfg}' abi='${abi}' dans ${base}"
    exit 1
  fi

  echo "$found"
}

# Debug
SRC_DEBUG_ARM64="$(find_one "Debug" "arm64-v8a")"
SRC_DEBUG_ARM32="$(find_one "Debug" "armeabi-v7a")"

# Release (souvent RelWithDebInfo)
SRC_REL_ARM64="$(find_one "RelWithDebInfo" "arm64-v8a")"
SRC_REL_ARM32="$(find_one "RelWithDebInfo" "armeabi-v7a")"

# --------------------------------------------------
# Dossiers de sortie
# build/android/<ABI>/Debug/
# build/android/<ABI>/Release/
# --------------------------------------------------
mkdir -p "${OUT_BASE}/arm64-v8a/Debug" "${OUT_BASE}/armeabi-v7a/Debug"
mkdir -p "${OUT_BASE}/arm64-v8a/Release" "${OUT_BASE}/armeabi-v7a/Release"

# --------------------------------------------------
# Copie des .a
# --------------------------------------------------
cp -f "${SRC_DEBUG_ARM64}" "${OUT_BASE}/arm64-v8a/Debug/librc2d_static.a"
cp -f "${SRC_DEBUG_ARM32}" "${OUT_BASE}/armeabi-v7a/Debug/librc2d_static.a"

cp -f "${SRC_REL_ARM64}" "${OUT_BASE}/arm64-v8a/Release/librc2d_static.a"
cp -f "${SRC_REL_ARM32}" "${OUT_BASE}/armeabi-v7a/Release/librc2d_static.a"

echo -e "\e[32m\nLib RC2D static Android generated successfully.\e[0m"
echo "Output:"
echo "  ${OUT_BASE}/arm64-v8a/Debug/librc2d_static.a"
echo "  ${OUT_BASE}/armeabi-v7a/Debug/librc2d_static.a"
echo "  ${OUT_BASE}/arm64-v8a/Release/librc2d_static.a (built as RelWithDebInfo)"
echo "  ${OUT_BASE}/armeabi-v7a/Release/librc2d_static.a (built as RelWithDebInfo)"
echo
