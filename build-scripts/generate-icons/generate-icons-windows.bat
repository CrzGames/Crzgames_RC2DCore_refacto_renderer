@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM Vérifier si Cargo est installé
REM ============================================================
where cargo >nul 2>&1
if %errorlevel% neq 0 (
    echo Cargo n'est pas installe. Veuillez l'installer pour continuer.
    exit /b 1
)

REM ============================================================
REM Fonction pour créer un dossier s'il n'existe pas
REM ============================================================
set OUTPUT_DIR=icons\outputs

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM ============================================================
REM Générer les icônes avec Tauri
REM ============================================================
cargo tauri icon --output icons\outputs icons\app-icon-default.png

REM ============================================================
REM Copier les icônes vers leurs dossiers respectifs
REM ============================================================

REM Windows
if exist icons\outputs\icon.ico (
    copy /Y icons\outputs\icon.ico icons\windows\app-icon.ico >nul
) else (
    echo icon.ico non trouve.
)

REM macOS
if exist icons\outputs\icon.icns (
    copy /Y icons\outputs\icon.icns icons\macos\app-icon.icns >nul
) else (
    echo icon.icns non trouve.
)

REM Linux
if exist icons\outputs\32x32.png (
    copy /Y icons\outputs\32x32.png icons\linux\app-icon-32x32.png >nul
) else (
    echo 32x32.png non trouve.
)

if exist icons\outputs\128x128.png (
    copy /Y icons\outputs\128x128.png icons\linux\app-icon-128x128.png >nul
) else (
    echo 128x128.png non trouve.
)

if exist icons\outputs\128x128@2x.png (
    copy /Y icons\outputs\128x128@2x.png icons\linux\app-icon-256x256.png >nul
) else (
    echo 128x128@2x.png non trouve.
)

if exist icons\outputs\icon.png (
    copy /Y icons\outputs\icon.png icons\linux\app-icon.png >nul
) else (
    echo icon.png non trouve.
)

echo.
echo ==========================================
echo Icons Windows, macOS, Linux generes avec succes !
echo ==========================================

REM ============================================================
REM Supprimer le dossier outputs parent si nécessaire
REM ============================================================
cd ..
if exist outputs (
    rmdir /s /q outputs
)

REM ============================================================
REM Copier les icônes Android
REM ============================================================
if exist android (
    xcopy android\* ..\android-project\app\src\main\res\ /E /I /Y >nul
    echo Icons Android copies avec succes vers : android-project\app\src\main\res\
) else (
    echo Dossier android non trouve.
)

echo.
echo Script termine.
pause