#!/bin/bash

# Define ANSI color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Vérifier le système d'exploitation
if [[ "$OSTYPE" == "linux-gnu" || "$OSTYPE" == "darwin"* ]]; then
    # Système Unix
    GRADLE="./gradlew"

    # Vérifier si ANDROID_HOME est défini (macOS)
    if [ -z "$JAVA_HOME" ]; then
        echo "Erreur : JAVA_HOME n'est pas défini."
        echo "Veuillez définir JAVA_HOME pour pointer vers votre répertoire Java JDK."
        echo "Exemple macOS : JAVA_HOME=/opt/homebrew/opt/openjdk@17"
        exit 1
    fi

    # Vérifier si un appareil Android est connecté
    if ! adb devices | grep -q 'device$'; then
        echo -e "\e[31mErreur : Aucun appareil Android n'est connecté.\e[0m"
        exit 1
    fi
else
    # Système Windows
    GRADLE="gradle"

    # Vérifier si un appareil Android est connecté
    if ! adb devices | findstr "\<device\>" >nul 2>&1; then
        echo -e "${RED}Erreur : Aucun appareil connecté. Veuillez connecter un appareil Android.${NC}"
        exit 1
    fi
fi

# Change to project directory
cd android-project

# Clean and build the project
echo -e "\e[32m \n Clean project...\e[0m"
$GRADLE clean

# Build the project in real device connected
echo -e "\e[32m \n Install APK in real device connected...\e[0m"
$GRADLE installDebug

# Clear previous logcat logs
echo -e "\e[32m \n Clearing previous logcat logs...\e[0m"
adb logcat -c

# Lancer l'application sur l'appareil connecté
echo -e "\e[32m \n Launching the application on the connected device...\e[0m"
adb shell am start -n com.crzgames.testexe/.MyGame # Remplacez 'com.crzgames.testexe/.MyGame' par le nom de package et l'activité principale de votre application

# Afficher les logs de l'application via logcat
echo -e "\e[32m \n Displaying logcat for SDL and SDL/APP...\e[0m"
adb logcat -s SDL:V "SDL/APP:V"
