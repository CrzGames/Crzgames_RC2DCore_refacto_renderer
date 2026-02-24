@echo off
setlocal EnableDelayedExpansion

REM ==================================================
REM RC2D - Build Android (Windows)
REM ==================================================
REM Ce script :
REM   1) Détecte la racine du repo (2 niveaux au-dessus du script)
REM   2) Vérifie JAVA_HOME et ANDROID_HOME
REM   3) Build APK (assembleDebug/assembleRelease)
REM   4) Build AAB (bundleDebug/bundleRelease)
REM   5) Exporte APK/AAB dans build\android\apk et build\android\aab
REM   6) Récupère librc2d_static.a (AGP/CMake) et copie vers build\android\<ABI>\<Config>\
REM
REM Notes :
REM - AGP utilise souvent "RelWithDebInfo" pour la config CMake du "Release"
REM - Les .a sont dans : android-project\app\.cxx\<Config>\<hash>\<abi>\librc2d_static.a
REM - ABIs ciblées : arm64-v8a + armeabi-v7a
REM ==================================================

REM --------------------------------------------------
REM Détection racine du repo :
REM Le .bat est dans build-scripts\generate-project\
REM Donc racine = ..\.. (Crzgames_RC2D)
REM --------------------------------------------------
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

for %%I in ("%SCRIPT_DIR%\..\..") do set "ROOT=%%~fI"

REM --------------------------------------------------
REM Vérification environnement
REM --------------------------------------------------

REM JAVA_HOME obligatoire
if "%JAVA_HOME%"=="" (
  call :die "JAVA_HOME n'est pas defini. Exemple : setx JAVA_HOME ""C:\Program Files\Eclipse Adoptium\jdk-17.0.18.8-hotspot"""
)
if not exist "%JAVA_HOME%" (
  call :die "Le chemin JAVA_HOME n'existe pas : %JAVA_HOME%"
)

REM ANDROID_HOME obligatoire
if "%ANDROID_HOME%"=="" (
  call :die "ANDROID_HOME n'est pas defini. Exemple : setx ANDROID_HOME ""C:\Users\<your-user>\AppData\Local\Android\Sdk"""
)

REM Retirer trailing backslash si présent
set "ANDROID_HOME=%ANDROID_HOME%"
if "%ANDROID_HOME:~-1%"=="\" set "ANDROID_HOME=%ANDROID_HOME:~0,-1%"

if not exist "%ANDROID_HOME%" (
  call :die "Le chemin Android SDK n'existe pas : %ANDROID_HOME%"
)

REM --------------------------------------------------
REM Variables
REM --------------------------------------------------
set "ANDROID_PROJECT=%ROOT%\android-project"
set "GRADLE=gradlew.bat"

set "OUT_BASE=%ROOT%\build\android"
set "OUT_APK_BASE=%OUT_BASE%\apk"
set "OUT_AAB_BASE=%OUT_BASE%\aab"

REM --------------------------------------------------
REM Vérifier que android-project existe
REM --------------------------------------------------
if not exist "%ANDROID_PROJECT%" (
  call :die "android-project introuvable : %ANDROID_PROJECT%"
)

REM --------------------------------------------------
REM Aller dans le projet Android
REM --------------------------------------------------
pushd "%ANDROID_PROJECT%" || call :die "Impossible d'entrer dans %ANDROID_PROJECT%"

REM Ensure wrapper exists
if not exist "%GRADLE%" (
  popd
  call :die "gradlew.bat introuvable dans %ANDROID_PROJECT%"
)

REM --------------------------------------------------
REM Clean Gradle
REM --------------------------------------------------
echo.
echo ==================================================
echo Clean project (Gradle)...
echo ==================================================
call "%GRADLE%" clean
if errorlevel 1 (
  popd
  exit /b 1
)

REM --------------------------------------------------
REM Build Gradle (APK)
REM --------------------------------------------------
echo.
echo ==================================================
echo Build APK Release (assembleRelease)...
echo ==================================================
call "%GRADLE%" assembleRelease
if errorlevel 1 (
  popd
  exit /b 1
)

echo.
echo ==================================================
echo Build APK Debug (assembleDebug)...
echo ==================================================
call "%GRADLE%" assembleDebug
if errorlevel 1 (
  popd
  exit /b 1
)

REM --------------------------------------------------
REM Build Gradle (AAB)
REM --------------------------------------------------
echo.
echo ==================================================
echo Build AAB Release (bundleRelease)...
echo ==================================================
call "%GRADLE%" bundleRelease
if errorlevel 1 (
  popd
  exit /b 1
)

echo.
echo ==================================================
echo Build AAB Debug (bundleDebug)...
echo ==================================================
call "%GRADLE%" bundleDebug
if errorlevel 1 (
  popd
  exit /b 1
)

REM --------------------------------------------------
REM Export APK / AAB vers build\android\apk et build\android\aab
REM --------------------------------------------------
echo.
echo ==================================================
echo Export APK/AAB outputs...
echo ==================================================

mkdir "%OUT_APK_BASE%\Debug" 2>nul
mkdir "%OUT_APK_BASE%\Release" 2>nul
mkdir "%OUT_AAB_BASE%\Debug" 2>nul
mkdir "%OUT_AAB_BASE%\Release" 2>nul

REM ---- Trouver 1 APK Debug/Release (premier trouvé) ----
set "APK_DEBUG="
for /R "app\build\outputs" %%F in (*debug*.apk) do (
  if not defined APK_DEBUG set "APK_DEBUG=%%F"
)

set "APK_RELEASE="
for /R "app\build\outputs" %%F in (*release*.apk) do (
  if not defined APK_RELEASE set "APK_RELEASE=%%F"
)

if defined APK_DEBUG (
  copy /Y "!APK_DEBUG!" "%OUT_APK_BASE%\Debug\rc2d-debug.apk" >nul
  echo APK Debug  -> %OUT_APK_BASE%\Debug\rc2d-debug.apk
) else (
  echo ^(warn^) Aucun APK Debug trouvé dans app\build\outputs\
)

if defined APK_RELEASE (
  copy /Y "!APK_RELEASE!" "%OUT_APK_BASE%\Release\rc2d-release.apk" >nul
  echo APK Release -> %OUT_APK_BASE%\Release\rc2d-release.apk
) else (
  echo ^(warn^) Aucun APK Release trouvé dans app\build\outputs\
)

REM ---- Trouver 1 AAB Debug/Release (premier trouvé) ----
set "AAB_DEBUG="
for /R "app\build\outputs" %%F in (*debug*.aab) do (
  if not defined AAB_DEBUG set "AAB_DEBUG=%%F"
)

set "AAB_RELEASE="
for /R "app\build\outputs" %%F in (*release*.aab) do (
  if not defined AAB_RELEASE set "AAB_RELEASE=%%F"
)

if defined AAB_DEBUG (
  copy /Y "!AAB_DEBUG!" "%OUT_AAB_BASE%\Debug\rc2d-debug.aab" >nul
  echo AAB Debug  -> %OUT_AAB_BASE%\Debug\rc2d-debug.aab
) else (
  echo ^(warn^) Aucun AAB Debug trouvé dans app\build\outputs\
)

if defined AAB_RELEASE (
  copy /Y "!AAB_RELEASE!" "%OUT_AAB_BASE%\Release\rc2d-release.aab" >nul
  echo AAB Release -> %OUT_AAB_BASE%\Release\rc2d-release.aab
) else (
  echo ^(warn^) Aucun AAB Release trouvé dans app\build\outputs\
)

REM --------------------------------------------------
REM Revenir à la racine pour chercher les libs .a dans app\.cxx
REM --------------------------------------------------
popd

REM --------------------------------------------------
REM Trouver les libs statiques générées par CMake via AGP :
REM android-project\app\.cxx\<Config>\<hash>\<abi>\librc2d_static.a
REM --------------------------------------------------

REM ---- Debug ----
set "CXX_DIR_DEBUG=%ANDROID_PROJECT%\app\.cxx\Debug"
for /D %%H in ("%CXX_DIR_DEBUG%\*") do (
  if exist "%%H\arm64-v8a\librc2d_static.a" set "SRC_DEBUG_ARM64=%%H\arm64-v8a\librc2d_static.a"
  if exist "%%H\armeabi-v7a\librc2d_static.a" set "SRC_DEBUG_ARM32=%%H\armeabi-v7a\librc2d_static.a"
)

REM ---- Release (souvent RelWithDebInfo) ----
set "CXX_DIR_RELEASE=%ANDROID_PROJECT%\app\.cxx\RelWithDebInfo"
for /D %%H in ("%CXX_DIR_RELEASE%\*") do (
  if exist "%%H\arm64-v8a\librc2d_static.a" set "SRC_REL_ARM64=%%H\arm64-v8a\librc2d_static.a"
  if exist "%%H\armeabi-v7a\librc2d_static.a" set "SRC_REL_ARM32=%%H\armeabi-v7a\librc2d_static.a"
)

REM --------------------------------------------------
REM Vérifs sources
REM --------------------------------------------------
if not defined SRC_DEBUG_ARM64 call :die "librc2d_static.a introuvable pour Debug arm64-v8a dans %CXX_DIR_DEBUG%"
if not defined SRC_DEBUG_ARM32 call :die "librc2d_static.a introuvable pour Debug armeabi-v7a dans %CXX_DIR_DEBUG%"
if not defined SRC_REL_ARM64 call :die "librc2d_static.a introuvable pour Release^(RelWithDebInfo^) arm64-v8a dans %CXX_DIR_RELEASE%"
if not defined SRC_REL_ARM32 call :die "librc2d_static.a introuvable pour Release^(RelWithDebInfo^) armeabi-v7a dans %CXX_DIR_RELEASE%"

REM --------------------------------------------------
REM Créer les dossiers de sortie :
REM build\android\<ABI>\Debug\
REM build\android\<ABI>\Release\
REM --------------------------------------------------
mkdir "%OUT_BASE%\arm64-v8a\Debug" 2>nul
mkdir "%OUT_BASE%\armeabi-v7a\Debug" 2>nul
mkdir "%OUT_BASE%\arm64-v8a\Release" 2>nul
mkdir "%OUT_BASE%\armeabi-v7a\Release" 2>nul

REM --------------------------------------------------
REM Copier les .a
REM --------------------------------------------------
copy /Y "%SRC_DEBUG_ARM64%" "%OUT_BASE%\arm64-v8a\Debug\librc2d_static.a" >nul || exit /b 1
copy /Y "%SRC_DEBUG_ARM32%" "%OUT_BASE%\armeabi-v7a\Debug\librc2d_static.a" >nul || exit /b 1
copy /Y "%SRC_REL_ARM64%" "%OUT_BASE%\arm64-v8a\Release\librc2d_static.a" >nul || exit /b 1
copy /Y "%SRC_REL_ARM32%" "%OUT_BASE%\armeabi-v7a\Release\librc2d_static.a" >nul || exit /b 1

echo.
echo ==================================================
echo Lib RC2D static Android generated successfully.
echo ==================================================
echo Outputs (.a):
echo   %OUT_BASE%\arm64-v8a\Debug\librc2d_static.a
echo   %OUT_BASE%\armeabi-v7a\Debug\librc2d_static.a
echo   %OUT_BASE%\arm64-v8a\Release\librc2d_static.a  ^(built as RelWithDebInfo^)
echo   %OUT_BASE%\armeabi-v7a\Release\librc2d_static.a ^(built as RelWithDebInfo^)
echo Outputs (APK):
echo   %OUT_BASE%\apk\Debug\rc2d-debug.apk
echo   %OUT_BASE%\apk\Release\rc2d-release.apk
echo Outputs (AAB):
echo   %OUT_BASE%\aab\Debug\rc2d-debug.aab
echo   %OUT_BASE%\aab\Release\rc2d-release.aab
echo.

exit /b 0

REM --------------------------------------------------
REM Helpers
REM --------------------------------------------------
:die
echo.
echo [ERROR] %~1
exit /b 1