@echo off
setlocal enabledelayedexpansion

REM --------------------------------------------------
REM Vérification environnement
REM --------------------------------------------------

if "%JAVA_HOME%"=="" (
  echo Erreur : JAVA_HOME n'est pas defini.
  exit /b 1
)
if "%ANDROID_HOME%"=="" if "%ANDROID_SDK_ROOT%"=="" (
  echo Erreur : ANDROID_HOME ou ANDROID_SDK_ROOT n'est pas defini.
  exit /b 1
)

REM --------------------------------------------------
REM Build Android via Gradle + export des libs statiques CMake (.a)
REM
REM NOTE IMPORTANTE :
REM - Android Gradle Plugin génère souvent "RelWithDebInfo" (pas un vrai "Release")
REM - Les libs statiques (.a) ne sont PAS dans intermediates/cxx/.../obj
REM   mais dans : android-project/app/.cxx/<Config>/<hash>/<abi>/librc2d_static.a
REM - Ce script copie les .a vers :
REM   build/android/<ABI>/<Config>/librc2d_static.a
REM --------------------------------------------------

REM Generate lib for Android (arm64-v8a + armeabi-v7a)
set "GRADLE=gradlew.bat"

REM Dossier de sortie (aligné avec tes autres scripts)
set "OUT_BASE=build\android"

REM --------------------------------------------------
REM Aller dans le projet Android
REM --------------------------------------------------
cd android-project

REM Ensure wrapper exists
if not exist "gradlew.bat" (
  echo Erreur : gradlew.bat introuvable dans android-project\
  exit /b 1
)

REM --------------------------------------------------
REM Clean Gradle
REM --------------------------------------------------
echo.
echo Clean project (Gradle)...
call %GRADLE% clean
if errorlevel 1 exit /b 1

REM --------------------------------------------------
REM Build Gradle
REM --------------------------------------------------
echo.
echo Build project for Release (Gradle)...
call %GRADLE% assembleRelease
if errorlevel 1 exit /b 1

echo.
echo Build project for Debug (Gradle)...
call %GRADLE% assembleDebug
if errorlevel 1 exit /b 1

cd ..

REM --------------------------------------------------
REM Trouver les libs statiques générées par CMake via AGP :
REM android-project/app/.cxx/<Config>/<hash>/<abi>/librc2d_static.a
REM
REM IMPORTANT :
REM - Pour "Release", AGP utilise souvent "RelWithDebInfo" (selon ta config Gradle)
REM - Pour "Debug", c'est généralement "Debug"
REM --------------------------------------------------

REM ---- Debug ----
set "CXX_DIR_DEBUG=android-project\app\.cxx\Debug"
REM arm64-v8a
for /D %%H in ("%CXX_DIR_DEBUG%\*") do (
  if exist "%%H\arm64-v8a\librc2d_static.a" (
    set "SRC_DEBUG_ARM64=%%H\arm64-v8a\librc2d_static.a"
  )
  if exist "%%H\armeabi-v7a\librc2d_static.a" (
    set "SRC_DEBUG_ARM32=%%H\armeabi-v7a\librc2d_static.a"
  )
)

REM ---- Release (souvent RelWithDebInfo) ----
set "CXX_DIR_RELEASE=android-project\app\.cxx\RelWithDebInfo"
for /D %%H in ("%CXX_DIR_RELEASE%\*") do (
  if exist "%%H\arm64-v8a\librc2d_static.a" (
    set "SRC_REL_ARM64=%%H\arm64-v8a\librc2d_static.a"
  )
  if exist "%%H\armeabi-v7a\librc2d_static.a" (
    set "SRC_REL_ARM32=%%H\armeabi-v7a\librc2d_static.a"
  )
)

REM --------------------------------------------------
REM Vérifs sources
REM --------------------------------------------------
if not defined SRC_DEBUG_ARM64 (
  echo Erreur : librc2d_static.a introuvable pour Debug arm64-v8a dans %CXX_DIR_DEBUG%
  exit /b 1
)
if not defined SRC_DEBUG_ARM32 (
  echo Erreur : librc2d_static.a introuvable pour Debug armeabi-v7a dans %CXX_DIR_DEBUG%
  exit /b 1
)
if not defined SRC_REL_ARM64 (
  echo Erreur : librc2d_static.a introuvable pour Release^(RelWithDebInfo^) arm64-v8a dans %CXX_DIR_RELEASE%
  exit /b 1
)
if not defined SRC_REL_ARM32 (
  echo Erreur : librc2d_static.a introuvable pour Release^(RelWithDebInfo^) armeabi-v7a dans %CXX_DIR_RELEASE%
  exit /b 1
)

REM --------------------------------------------------
REM Créer les dossiers de sortie :
REM build/android/<ABI>/Debug/
REM build/android/<ABI>/Release/
REM --------------------------------------------------
mkdir "%OUT_BASE%\arm64-v8a\Debug" 2>nul
mkdir "%OUT_BASE%\armeabi-v7a\Debug" 2>nul
mkdir "%OUT_BASE%\arm64-v8a\Release" 2>nul
mkdir "%OUT_BASE%\armeabi-v7a\Release" 2>nul

REM --------------------------------------------------
REM Copier les .a
REM --------------------------------------------------
copy /Y "%SRC_DEBUG_ARM64%" "%OUT_BASE%\arm64-v8a\Debug\librc2d_static.a" >nul
if errorlevel 1 exit /b 1
copy /Y "%SRC_DEBUG_ARM32%" "%OUT_BASE%\armeabi-v7a\Debug\librc2d_static.a" >nul
if errorlevel 1 exit /b 1

copy /Y "%SRC_REL_ARM64%" "%OUT_BASE%\arm64-v8a\Release\librc2d_static.a" >nul
if errorlevel 1 exit /b 1
copy /Y "%SRC_REL_ARM32%" "%OUT_BASE%\armeabi-v7a\Release\librc2d_static.a" >nul
if errorlevel 1 exit /b 1

echo.
echo Lib RC2D static Android generated successfully.
echo Output:
echo   %OUT_BASE%\arm64-v8a\Debug\librc2d_static.a
echo   %OUT_BASE%\armeabi-v7a\Debug\librc2d_static.a
echo   %OUT_BASE%\arm64-v8a\Release\librc2d_static.a  ^(built as RelWithDebInfo^)
echo   %OUT_BASE%\armeabi-v7a\Release\librc2d_static.a ^(built as RelWithDebInfo^)
echo.

exit /b 0