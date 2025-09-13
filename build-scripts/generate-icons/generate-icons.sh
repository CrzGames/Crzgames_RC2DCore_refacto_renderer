#!/bin/bash

# Vérifier si Cargo est installé
if ! command -v cargo &> /dev/null; then
    echo "Cargo n'est pas installé. Veuillez l'installer pour continuer."
    exit 1
fi

# Fonction pour créer un dossier s'il n'existe pas
create_directory_if_not_exists() {
    if [ ! -d "$1" ]; then
        mkdir -p "$1"
    fi
}

# Créer le dossier 'outputs' dans 'icons' s'il n'existe pas
create_directory_if_not_exists "icons/outputs"

# Exécuter la commande Tauri pour générer les icônes
cargo tauri icon --output icons/outputs icons/app-icon-default.png

# Renommer et copier les fichiers icônes dans leurs dossiers respectifs
# Windows
cp icons/outputs/icon.ico icons/windows/app-icon.ico 2>/dev/null || echo "icon.ico non trouvé."
# macOS
cp icons/outputs/icon.icns icons/macos/app-icon.icns 2>/dev/null || echo "icon.icns non trouvé."
# Linux
cp icons/outputs/32x32.png icons/linux/app-icon-32x32.png 2>/dev/null || echo "32x32.png non trouvé."
cp icons/outputs/128x128.png icons/linux/app-icon-128x128.png 2>/dev/null || echo "128x128.png non trouvé."
cp icons/outputs/128x128@2x.png icons/linux/app-icon-256x256.png 2>/dev/null || echo "128x128@2x.png non trouvé."
cp icons/outputs/icon.png icons/linux/app-icon.png 2>/dev/null || echo "icon.png non trouvé."

echo -e "\e[32m Icons Windows, macOS, Linux generer avec succès !\e[0m"

cd ../
rm -rf outputs

# Copier les icônes Android vers le répertoire de destination du projet Android.
cp -r android/* ../android-project/app/src/main/res/
echo -e "\e[32m Icons Android copiés avec succès vers : android-project/app/src/main/res/\e[0m"
