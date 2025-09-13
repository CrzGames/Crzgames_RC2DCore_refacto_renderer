#!/bin/bash
set -e

# Define ANSI color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

CONFIGURATION="Debug" # Default configuration

# Process command line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --config)
            CONFIGURATION="$2"
            shift # pass to next argument
            ;;
        *)
            echo -e "${RED}Erreur : Argument non reconnu : $1${NC}"
            exit 1
            ;;
    esac
    shift # pass to next argument or value
done

# Define your project-specific variables (nouveau chemin)
BUILD_DIR="./build/ios/iphoneos"

# ➕ Générer le projet Xcode si le dossier n'existe pas
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${GREEN}Generating Xcode project for iOS (iphoneos)...${NC}"
    cmake -S . -B "$BUILD_DIR" -G Xcode -DCMAKE_OSX_SYSROOT=iphoneos
fi

# Clean and rebuild the project
echo -e "${GREEN}Cleaning and rebuilding the project...${NC}"
cmake --build "$BUILD_DIR" --target clean
cmake --build "$BUILD_DIR" --config "$CONFIGURATION"

# Define the path to the .app file for iphoneos
APP_PATH=$(find "$BUILD_DIR/$CONFIGURATION" -name "*.app" -print -quit)

# Check if the .app file exists
if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}Erreur : Aucun fichier .app trouvé dans $BUILD_DIR/$CONFIGURATION${NC}"
    exit 1
fi

# Vérifier que ios-deploy est installé
if ! command -v ios-deploy &> /dev/null; then
    echo -e "${RED}Erreur : ios-deploy n'est pas installé.${NC}"
    echo "👉 Installe-le avec : brew install ios-deploy"
    exit 1
fi

# Extraire l'ID du premier périphérique connecté via USB (supposant qu'il y en ait au moins un)
DEVICE_ID=$(ios-deploy --no-wifi -c | grep -oE 'Found ([0-9A-Za-z\-]+)' | sed 's/Found //g')

# Vérifier si un périphérique a été détecté
if [ -n "$DEVICE_ID" ]; then
  # Uninstall the existing App from the connected device
  echo -e "\e[32m\nUninstalling the existing application from device...\e[0m"
  #ios-deploy --uninstall_only --bundle "$APP_PATH" --id "$DEVICE_ID"
  
  # Run the application in real device
  echo -e "\e[32m\nApplication installed in real device now...\e[0m"
  ios-deploy --justlaunch --bundle "$APP_PATH" --id "$DEVICE_ID"
  
  # Start log with a filter for your application (équivalent adb: SDL:V "SDL/APP:V")
  echo -e "\e[32m\nStarting device logs (filter: SDL, SDL/APP)...\e[0m"
  idevicesyslog -u "$DEVICE_ID" | grep -E --line-buffered "SDL|SDL/APP"
else
  echo -e "${RED}Warning: Aucun device iOS détecté.${NC}"
fi
