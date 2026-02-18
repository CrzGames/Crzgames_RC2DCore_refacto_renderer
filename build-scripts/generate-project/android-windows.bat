@echo off
setlocal enabledelayedexpansion

REM Generate lib for Android (arm64-v8a + armeabi-v7a)

set "GRADLE=gradlew.bat"

REM Define base directories
set "BASE_BUILD_DIR_RELEASE=android-project\app\build\intermediates\cmake\release\obj"
set "DIST_DIR_RELEASE=dist\lib\android\Release"
set "BASE_BUILD_DIR_DEBUG=android-project\app\build\intermediates\cmake\debug\obj"
set "DIST_DIR_DEBUG=dist\lib\android\Debug"

REM Create destination directories
mkdir "%DIST_DIR_RELEASE%\arm64-v8a" 2>nul
mkdir "%DIST_DIR_RELEASE%\armeabi-v7a" 2>nul
mkdir "%DIST_DIR_DEBUG%\arm64-v8a" 2>nul
mkdir "%DIST_DIR_DEBUG%\armeabi-v7a" 2>nul

REM Vérifier si ANDROID_HOME est défini
if "%ANDROID_HOME%"=="" (
  echo Erreur : ANDROID_HOME n'est pas defini.
  echo Veuillez definir ANDROID_HOME pour pointer vers votre repertoire Android SDK/NDK.
  echo Exemple : set ANDROID_HOME=C:\Users\Corentin\AppData\Local\Android\Sdk
  exit /b 1
)

REM Change to project directory
cd android-project

REM Ensure wrapper exists
if not exist "gradlew.bat" (
  echo Erreur : gradlew.bat introuvable dans android-project\
  exit /b 1
)

REM Clean and build the project
echo.
echo Clean project...
call %GRADLE% clean
if errorlevel 1 exit /b 1

echo.
echo Build project for Release...
call %GRADLE% assembleRelease
if errorlevel 1 exit /b 1

echo.
echo Build project for Debug...
call %GRADLE% assembleDebug
if errorlevel 1 exit /b 1

cd ..

REM Copy .so files to respective directories
copy /Y "%BASE_BUILD_DIR_RELEASE%\arm64-v8a\librc2d.so" "%DIST_DIR_RELEASE%\arm64-v8a\" >nul
if errorlevel 1 exit /b 1
copy /Y "%BASE_BUILD_DIR_RELEASE%\armeabi-v7a\librc2d.so" "%DIST_DIR_RELEASE%\armeabi-v7a\" >nul
if errorlevel 1 exit /b 1

copy /Y "%BASE_BUILD_DIR_DEBUG%\arm64-v8a\librc2d.so" "%DIST_DIR_DEBUG%\arm64-v8a\" >nul
if errorlevel 1 exit /b 1
copy /Y "%BASE_BUILD_DIR_DEBUG%\armeabi-v7a\librc2d.so" "%DIST_DIR_DEBUG%\armeabi-v7a\" >nul
if errorlevel 1 exit /b 1

echo.
echo Lib RC2D for Android ^>= 9.0 generated successfully. Go to dist\lib\android\
echo.

exit /b 0
