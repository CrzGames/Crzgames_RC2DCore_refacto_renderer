#!/bin/bash
set -euo pipefail

# --------------------------------------------------
# Vérification environnement
# --------------------------------------------------

# JAVA_HOME obligatoire (Gradle tourne sur Java)
if [ -z "${JAVA_HOME:-}" ]; then
  die "JAVA_HOME n'est pas défini. Exemple macOS : export JAVA_HOME=/Library/Java/JavaVirtualMachines/temurin-17.jdk"
  die "Assure-toi d'avoir Java JDK 17 installé (Temurin) et que JAVA_HOME pointe vers le dossier racine du JDK."
fi

# ANDROID_HOME obligatoire (SDK requis par Gradle + adb)
if [ -z "${ANDROID_HOME:-}" ]; then
  die "ANDROID_HOME n'est pas défini. Exemple macOS : export ANDROID_HOME=/Users/<your-user>/Library/Android/sdk"
  die "Assure-toi d'avoir le SDK Android installé (via Android Studio) et que ANDROID_HOME pointe vers le dossier racine du SDK."
fi

ANDROID_HOME="${ANDROID_HOME%/}"

if [ ! -d "${ANDROID_HOME}" ]; then
  die "Le chemin Android SDK n'existe pas : ${ANDROID_HOME}"
fi

# --------------------------------------------------
# Build Android via Gradle + export des libs statiques CMake (.a)
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

# --------------------------------------------------
# Export APK / AAB
# --------------------------------------------------
echo -e "\e[32m\nExport APK/AAB outputs...\e[0m"

OUT_APK_BASE="../${OUT_BASE}/apk"
OUT_AAB_BASE="../${OUT_BASE}/aab"

mkdir -p "${OUT_APK_BASE}/Debug" "${OUT_APK_BASE}/Release"
mkdir -p "${OUT_AAB_BASE}/Debug" "${OUT_AAB_BASE}/Release"

# APK (debug/release)
APK_DEBUG="$(find "app/build/outputs" -type f -name "*debug*.apk" 2>/dev/null | head -n 1 || true)"
APK_RELEASE="$(find "app/build/outputs" -type f -name "*release*.apk" 2>/dev/null | head -n 1 || true)"

if [ -n "$APK_DEBUG" ]; then
  cp -f "$APK_DEBUG" "${OUT_APK_BASE}/Debug/$(basename "$APK_DEBUG")"
  echo "APK Debug  -> ${OUT_APK_BASE}/Debug/$(basename "$APK_DEBUG")"
else
  echo "⚠️  Aucun APK Debug trouvé dans app/build/outputs/"
fi

if [ -n "$APK_RELEASE" ]; then
  cp -f "$APK_RELEASE" "${OUT_APK_BASE}/Release/$(basename "$APK_RELEASE")"
  echo "APK Release -> ${OUT_APK_BASE}/Release/$(basename "$APK_RELEASE")"
else
  echo "⚠️  Aucun APK Release trouvé dans app/build/outputs/"
fi

# AAB (souvent nécessite bundleDebug/bundleRelease)
# On tente de builder les bundles si dispo (sans échouer si la task n'existe pas).
if $GRADLE tasks --all 2>/dev/null | grep -qE '(^| )bundleRelease\b'; then
  echo -e "\e[32m\nBuild AAB Release (Gradle bundleRelease)...\e[0m"
  $GRADLE bundleRelease
fi
if $GRADLE tasks --all 2>/dev/null | grep -qE '(^| )bundleDebug\b'; then
  echo -e "\e[32m\nBuild AAB Debug (Gradle bundleDebug)...\e[0m"
  $GRADLE bundleDebug
fi

AAB_DEBUG="$(find "app/build/outputs" -type f -name "*debug*.aab" 2>/dev/null | head -n 1 || true)"
AAB_RELEASE="$(find "app/build/outputs" -type f -name "*release*.aab" 2>/dev/null | head -n 1 || true)"

if [ -n "$AAB_DEBUG" ]; then
  cp -f "$AAB_DEBUG" "${OUT_AAB_BASE}/Debug/$(basename "$AAB_DEBUG")"
  echo "AAB Debug  -> ${OUT_AAB_BASE}/Debug/$(basename "$AAB_DEBUG")"
else
  echo "⚠️  Aucun AAB Debug trouvé dans app/build/outputs/"
fi

if [ -n "$AAB_RELEASE" ]; then
  cp -f "$AAB_RELEASE" "${OUT_AAB_BASE}/Release/$(basename "$AAB_RELEASE")"
  echo "AAB Release -> ${OUT_AAB_BASE}/Release/$(basename "$AAB_RELEASE")"
else
  echo "⚠️  Aucun AAB Release trouvé dans app/build/outputs/"
fi

cd ..

# --------------------------------------------------
# Trouver les libs statiques générées par CMake via AGP :
# android-project/app/.cxx/<Config>/<hash>/<abi>/librc2d_static.a
# --------------------------------------------------

find_one() {
  local cfg="$1"
  local abi="$2"
  local base="android-project/app/.cxx/${cfg}"
  local found=""

  if [ -d "$base" ]; then
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
echo "Outputs:"
echo "  ${OUT_BASE}/arm64-v8a/Debug/librc2d_static.a"
echo "  ${OUT_BASE}/armeabi-v7a/Debug/librc2d_static.a"
echo "  ${OUT_BASE}/arm64-v8a/Release/librc2d_static.a (built as RelWithDebInfo)"
echo "  ${OUT_BASE}/armeabi-v7a/Release/librc2d_static.a (built as RelWithDebInfo)"
echo "  ${OUT_BASE}/apk/Debug/(apk...)"
echo "  ${OUT_BASE}/apk/Release/(apk...)"
echo "  ${OUT_BASE}/aab/Debug/(aab...)"
echo "  ${OUT_BASE}/aab/Release/(aab...)"