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

# Uninstall the existing APK from the connected device
echo -e "\e[32m \n Uninstalling existing APK from device...\e[0m"
adb uninstall com.crzgames.testexe # Remplacez 'com.crzgames.testexe' par le nom de package de votre application

# Build the project in real device connected
echo -e "\e[32m \n Install APK in real device connected...\e[0m"
$GRADLE installDebug

