# Crzgames - RC2D

## 🛠 Tech Stack
- C (Language)
- A C99 Compiler
- CI / CD (Github Actions)
- CMake (Build script)
- Compiler (GCC, CL, Clang, Ninja, NDK)

<br /><br />

---

<br /><br />

## 📁 Project Structure

```
📦 Crzgames_RC2DCore
├── 📁 .github                        # Configuration GitHub (workflows CI/CD)
├── 📁 android-project                # Projet Android contenant les fichiers nécessaires pour packager RC2D sous forme de .aar
├── 📁 build (git ignored)            # Projet générer via un script au préalable (lib RC2D static + projet exemple)
├── 📁 build-scripts                  # Scripts de build personnalisés (.sh / .bat).
├── 📁 cmake                          
│   └── 📄 setup_dependencies.cmake   # Script CMake chargé de lire `dependencies.txt` et cloner/configurer les dépendances dans `/dependencies`
├── 📁 dependencies (git ignored)     # Répertoire local contenant les dépendances clonées (ignoré par Git pour ne pas polluer le repo)
│   ├── 📁 cJSON                      # Libraire JSON
│   ├── 📁 Crzgames_Libraries         # Librairies précompilées (OpenSSL, ONNX Runtime, Crzgames_RCENet, ffmpeg et SDL_shadercross) propres à Crzgames
│   ├── 📁 SDL                        # SDL3 (dépendance principale du moteur)
│   ├── 📁 SDL_image                  # Extension SDL3 pour le support des images (PNG, JPEG, etc.)
│   ├── 📁 SDL_ttf                    # Extension SDL3 pour le rendu de polices TrueType
│   ├── 📁 SDL_mixer                  # Extension SDL3 pour la gestion audio avancée
├── 📁 docker-build-output-steamrt4 (git ignored) # Projet générer via le docker-compose pour SteamRT4 SDK x64/arm64
├── 📁 docs                           # Documentation du moteur (pages Markdown, auto-générées ou manuelles)
├── 📁 example                        # Exemples d’utilisation du moteur RC2D (projets de démo, test de fonctionnalités)
├── 📁 icons                          # Icones générer pour l'executable du projet d'exemple via au préalable via un script.
├── 📁 include                        # En-têtes publics exposés aux utilisateurs de la lib (API du moteur)
│   ├── 📁 external/lz4.h             # Dossier "external" qui contient des librairies link statiquement depuis leur fichier include directement et intégrer à RC2D.
│   ├── 📁 RC2D/*.h                   # Dossier des fichiers include de RC2D
├── 📁 platforms                      # Contient la base de donnée des gamecontrollersdl, des choses spécifique au plateforme pour le projet d'exemple
├── 📁 src                            # Code source interne de la bibliothèque RC2D (implémentations .c)
│   ├── 📁 external/lz4.c             # Dossier "external" qui contient des librairies link statiquement depuis leur fichier source directement et intégrer à RC2D.
│   ├── 📁 RC2D/*.c                   # Dossier des fichiers source de RC2D
├── 📁 tests                          # Tests unitaires (avec Criterion) pour vérifier les modules du moteur
├── 📄 .gitignore                     # Fichiers/dossiers à ignorer par Git (ex: /dependencies, builds temporaires)
├── 📄 CHANGELOG.md                   # Historique des versions avec les modifications apportées à chaque release
├── 📄 CMakeLists.txt                 # Point d’entrée de la configuration CMake (build multiplateforme)
├── 📄 dependencies.txt               # Fichier listant les dépendances à cloner (format : nom=repo:version)
├── 📄 docker-compose.yml             # Deux image SteamRT4 SDK x64 et arm64 pour build RC2D pour SteamRT4 spécifiquement.
├── 📄 README.md                      # Page d’accueil du dépôt (description, installation, exemples d’usage)
├── 📄 release-please-config.json     # Configuration pour `release-please` (outil Google de génération automatique de releases)
├── 📄 TODO.md                        # TODO en cours à faire
├── 📄 version.txt                    # Contient la version actuelle du moteur (utilisé dans le build ou les releases)

```

<br /><br />

---

<br /><br />

## 📋 Plateformes supportées
- 🟢 supporté
- 🟡 en cours
- 🔴 non supporté

| Platform | Architectures | System Version | Compatible |
|----------|---------------|----------------|------------|
| **Windows** | x64 / arm64 | Windows 10+ | 🟢 |
| **macOS** | Apple Silicon arm64 | macOS 15.0+ | 🟢 |
| **iOS/iPadOS** | arm64 (iphoneos) | iOS/iPadOS 18.0+ | 🟢 |
| **Android** | arm64-v8a / armeabi-v7a | Android 9.0+ | 🟢 |
| **Linux** | x64 / arm64 | glibc 2.35+ | 🟢 |
| **Steam Linux** | x64 / arm64 | Steam Linux Runtime 4.0 | 🟢 |
| **Steam Deck** | x64 | Steam Linux Runtime 4.0 | 🟢 |
| **Xbox Série X/S** | x64 | - | 🔴 |
| **Nintendo Switch 1** | arm64 | - | 🔴 |
| **Nintendo Switch 2** | arm64 | - | 🔴 |
| **Playstation 5** | x64 | - | 🔴 |


<br /><br />

---

<br /><br />

## 📱 Appareils compatibles par plateforme

### **iOS / iPadOS (18.0+)**

#### iPhones:
- iPhone XR / XS / XS Max
- iPhone SE (2/3ème génération)
- iPhone 11 / 12 / 13 / 14 / 15 / 16 (Normal, Mini, Plus, Pro, Pro Max, E) et plus récent

#### iPads:
- iPad mini (5/6ème génération, A17 Pro) et plus récent
- iPad (7/8/9/10ème génération, A16) et plus récent
- iPad Air (3/4/5ème génération, M2, M3) et plus récent
- iPad Pro (1/2/3/4/5/6ème génération, M4) et plus récent

### **macOS (15.0+)**
- Tous les modèles macOS Apple Silicon (M1, M2, M3, M4, M5) et plus récent.

### **Android (9.0+)**
- Samsung Galaxy S9+ (2018) et plus récent.
- Google Pixel 3 et plus récent.
- OnePlus 6T et plus récent.
- Galaxy Tab S4 (2018) et plus récent.

### **Linux (glibc 2.35+)**
- Ubuntu 22.04 et plus récent.
- Debian 12 et plus récent.
- Fedora 36 et plus récent.
- Linux Mint 21 et plus récent.
- elementary OS 7 et plus récent.
- CentOS/RHEL 10 et plus récent.

### **Windows (10+)**
- Windows 10 et plus récent.

### Steam Deck (Steam Linux Runtime 4.0+)
- Steam Deck 1 (LCD / OLED, sous SteamOS 3.0 ou supérieur) et plus récent.

### Steam Linux (Steam Linux Runtime 4.0+)
- Compatible avec toute distribution Linux x64 / arm64 supportant Steam.

<br /><br />

---

<br /><br />

## 🎯 Raisons techniques des versions minimales et autres par plateforme

### Windows
- **Version minimale** : Windows 10+ for x64 / arm64
- **Raison** :
  - SDL3 API GPU repose sur Direct3D12 (Level Feature 11_1)
  - Windows ARM64 nécessite également Windows 10+

### macOS
- **Version minimale** : macOS 15.0+ for arm64
- **Raison** :
  - Requis par ONNX Runtime pour la version >= à v1.24.1, il faut macOS >= 14.0
  - Requis par MSL version 3.2.0, il faut macOS >= 15.0
  - Pourquoi pas d'architecture x86_64 ? -> ONNX Runtime pour la version >= à v1.24.1 ne fournira plus de binaires x86_64 pour les systèmes d'exploitation macOS et iOS.

### iOS/iPadOS
- **Version minimale** : iOS/iPadOS 18.0+ for arm64
- **Raison** :
  - SDL3 API GPU supporté depuis iOS/iPadOS 13.0
  - CoreML pour ONNX Runtime nécessite iOS/iPadOS 13.0+
  - Requis par MSL version 3.2.0 (iOS/iPadOS 18.0+)
  - Pas de librairie pour iOS/iPadOS simulator parce que SDL3 GPU ne le supporte pas.
  - Pourquoi pas d'architecture x86_64 ? -> ONNX Runtime pour la version >= à v1.24.1 ne fournira plus de binaires x86_64 pour les systèmes d'exploitation macOS et iOS.

### Android
- **Version minimale** : Android 9.0 (API 28+)
- **Raison** :
  - SDL3 GPU utilise Vulkan (introduit à partir d'Android 7.0)
  - ONNX Runtime avec NNAPI demande Android 8.1+ et recommande Android 9.0+
  - Pas d'architecture Android : x86_64 et x86, parce que ONNX Runtime compatible que : arm64-v8a / armeabi-v7a

### Linux
- **Version minimale** : glibc 2.35+
- **Raison** :
  - CI/CD basée sur Ubuntu 22.04 LTS (donc librairie RC2D + dépendences construite sur glibc 2.35)
  - ONNX Runtime nécessite C++20 (glibc 2.31+)

### Steam Deck / Steam Linux
- **Version minimale** : Steam Linux Runtime 4.0
- **Raison** :
  - Steam recommande l'utilisation du runtime Sniper pour tous les nouveaux jeux compatibles Linux.
  - Le Steam Deck est livré avec SteamOS 3.0+, basé sur Arch Linux, et embarque nativement le runtime Sniper.
  - Toutes les dépendances système (glibc ≥ 2.35, Mesa Vulkan ≥ 22, etc.) sont fournies via le runtime, assurant un environnement stable et cohérent.

<br /><br />

---

<br /><br />

## 📦 Dépendances principales

> Les versions sont verrouillées afin de garantir des builds reproductibles sur toutes les plateformes.

| Librairie | Version / Commit SHA utilisé par RC2D | Rôle dans RC2D | Statut / Intégration | Impact si désactivé |
|------------|----------------------------------------|----------------|----------------------|----------------------|
| **LZ4** | v1.10.0 | Compression ultra-rapide utilisée par `RC2D_data` | ⭐ Obligatoire (intégré statiquement) |  |
| **SDL3** | commit `550394eecdc250c7ce542a99f0c2b55683521656` | Gestion fenêtre, entrées, rendu GPU | ⭐ Obligatoire |  |
| **SDL3_image** | commit `8bd9f3d7f2d2bb59ce4331f13b77d65254cd8c7b` | Chargement d’images (PNG, SVG, APNG…) | ⭐ Obligatoire |  |
| **SDL3_ttf** | commit `053bbc89517471427748a082583c9eada55c07b5` | Rendu de polices TrueType | ⭐ Obligatoire |  |
| **SDL3_mixer** | commit `8f7790334a7d8b680d90968ce2e211129b892492` | Gestion audio (WAV, OGG, OPUS…) | ⭐ Obligatoire | |
| **cJSON** | v1.7.19 | Parsing JSON léger | ⭐ Obligatoire (intégré statiquement) |  |
| **SDL3_shadercross** | commit `7b7365a86611b2a7b6462e521cf1c43a037d0970` | Transpilation shaders (HLSL → MSL / SPIR-V / DXIL / METALLIB…) | 🟡 Optionnel (Dev uniquement, marche uniquement pour Windows/macOS/Linux/SteamRT4) – `RC2D_GPU_SHADER_HOT_RELOAD_ENABLED` (ON par défaut) | Si désactivé : pas de hot-reload / compilation runtime des shaders, il faudras utiliser la compilation hors ligne des shaders via les scripts des shaders |
| **RCENet** | v1.4.6 | Communication réseau UDP (fork ENet) | 🟡 Optionnel – `RC2D_NET_MODULE_ENABLED` (ON par défaut) | Si désactivé : module réseau indisponible (`RC2D_net.h`) |
| **OpenSSL** | v3.6.1 | Hashing, chiffrement, crypto | 🟡 Optionnel – `RC2D_DATA_MODULE_ENABLED` (ON par défaut) | Si désactivé : module data/crypto indisponible (`RC2D_data.h`) |
| **ONNX Runtime** | v1.24.1 | Inférence IA (modèles ONNX) | 🟡 Optionnel – `RC2D_ONNX_MODULE_ENABLED` (ON par défaut) | Si désactivé : module IA indisponible (`RC2D_onnx.h`) |
| **FFmpeg** | v8.0.0 | Lecture vidéo (MP4, etc.) | 🟡 Optionnel – `RC2D_VIDEO_MODULE_ENABLED` (ON par défaut) | Si désactivé : module vidéo indisponible (`RC2D_video.h`) |

<br /><br />

---

<br /><br />

## ⚙️ Setup Environment Development
1. Cloner le projet :
  ```bash
  git clone git@github.com:CrzGames/Crzgames_RC2D.git
  ```
2. Steps by Platform :
  ```bash  
  # Windows (x64/arm64) :
  1. Requirements : Windows >= 10
  2. Download and Install Visual Studio == 2022 (MSVC >= v143 + Windows SDK >= 10) : https://visualstudio.microsoft.com/fr/downloads/
  3. Download and Install CMake >= 3.28.0 : https://cmake.org/download/ and add PATH ENVIRONMENT.
  4. Installer Rust + Cargo (for Icons) :
     winget install Rustlang.Rustup
  5. Installer Tauri CLI (for Icons) :
     cargo install tauri-cli
  6. Activer le support long path dans Windows (Powershell en adminstrateur) : 
     reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /v LongPathsEnabled /t REG_DWORD /d 1 /f
  7. Activer long paths dans Git :
     git config --global core.longpaths true
  8. Fermer/RéOuvrir un nouveau terminal pour prendre en compte les deux dernières étapes.



  # Linux (x64/arm64) :
  1. Requirements : glibc >= 2.35.0 (Exemple : Ubuntu >= 22.04 OR Debian >= 12.0), checker via : ldd --version
  2. Download and Install (gcc, g++, make..) :
     sudo apt update
     sudo apt install -y build-essential
  3. Download and Install CMake >= 3.28.0 : sudo apt install -y cmake
  4. Download and Install Patchelf : sudo apt install -y patchelf
  5. Download and Install dev dependencies for SDL3 :
    sudo apt-get update
    sudo apt-get -y install build-essential git make \
    pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
    libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
    libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
    libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev \
    ibpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev
  6. Download and Install fuse (for generate .AppImage) :
     Pour Ubuntu (>= 22.04) :
      sudo apt update
      sudo apt install libfuse2 
     Pour Debian (>= 13) et Ubuntu (>= 24.04) : 
      sudo apt update
      sudo apt install -y libfuse2t64
  7. Installer Rust + Cargo (for Icons) :
     curl https://sh.rustup.rs -sSf | sh
     source $HOME/.cargo/env
  8. Installer Tauri CLI (for Icons) :
     cargo install tauri-cli
  9. Install dev dependencies Tauri (for Icons) :
     sudo apt update
     sudo apt install libwebkit2gtk-4.1-dev \
      build-essential \
      curl \
      wget \
      file \
      libxdo-dev \
      libssl-dev \
      libayatana-appindicator3-dev \
      librsvg2-dev



  # SteamRT4 (for Steam Linux x64-arm64 / Steam Deck x64) :
  1. Utiliser un container via le tag de l image SteamRT4 voir : https://repo.steampowered.com/steamrt4/images/
  2. Download and Install (gcc, g++, make..) :
     sudo apt update
     sudo apt install -y build-essential
  3. Download and Install CMake >= 3.28.0 : sudo apt install -y cmake
  4. Download and Install Patchelf : sudo apt install -y patchelf
  5. Download and Install dev dependencies for SDL3 :
    sudo apt-get update
    sudo apt-get -y install build-essential git make \
    pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
    libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
    libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
    libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev \
    ibpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev



  # macOS (Apple Silicon arm64) :
  1. Requirements : MacOS X >= 15.0.0
  2. Download and Install xCode >= 16.4.0
  3. Download and Install Command Line Tools : xcode-select --install
  4. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  5. Download and Install CMake >= 3.28.0 : brew install cmake
  6. Check SDK version : 
    xcrun --sdk iphoneos --show-sdk-version
    xcrun --sdk macosx --show-sdk-version
  7. Lister tous les certificats installés sur ton Mac (si besoin) :
     security find-identity -v -p codesigning
  8. Installer Rust + Cargo (for Icons) :
     curl https://sh.rustup.rs -sSf | sh
     source $HOME/.cargo/env
  9. Installer Tauri CLI (for Icons) :
     cargo install tauri-cli
  10. Download and Install (pour le script de compilation des shaders hors ligne, notamment pour la compilation de metal / metallib : xcodebuild -downloadComponent MetalToolchain



  # Android (Linux x64-arm64 / macOS Apple Silicon arm64 / Windows x64-arm64) :
  1. Download and Install: Android Studio 2025.3.1 or newer
  2. Add environment variable `ANDROID_HOME` → path to Android SDK :
    - Windows :  
      `C:\Users\<your-user>\AppData\Local\Android\Sdk`
    - macOS (.zshrc):  
      `/Users/<your-user>/Library/Android/sdk`
    - Linux (.bashrc):  
      `/home/<your-user>/Android/Sdk`
  3. Open **Android Studio → SDK Manager** and install the following components:
    ### 📦 SDK Platforms (tab "SDK Platforms")
    - ✅ Android v16.0
    ### 🛠 SDK Tools (tab "SDK Tools")
    - ✅ Android SDK Build-Tools v36.1.0
    - ✅ NDK v29.0.14206865
    - ✅ Android SDK Command-line Tools v20.0
    - ✅ CMake v3.30.3
  4. Download and Install Java JDK 17 LTS (Temurin) : 
    https://adoptium.net/fr/temurin/releases?version=17&os=any&arch=any  
  5. Add environment variable `JAVA_HOME` → path to Java JDK Temurin :
    - Windows example:  
      `C:\Program Files\Eclipse Adoptium\jdk-17.0.18.8-hotspot`
    - macOS example: 
      `/Library/Java/JavaVirtualMachines/temurin-17.jdk`
  6. Install patchelf (Linux seulement) :
    sudo apt install -y patchelf
  7. Ajouter au PATH ENVIRONMENT pour adb (Windows seulement) : C:\Users\<your-user>\AppData\Local\Android\Sdk\platform-tools



  # iOS (only macOS) :
  1. Requirements : MacOS X >= 15.0.0
  2. Download and Install xCode >= 16.4.0
  3. Download and Install SDK iOS >= 18.0.0
  4. Download and Install Command Line Tools : xcode-select --install
  5. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  6. Download and Install cmake >= 3.28.0 : brew install cmake
  7. Checker les versions des SDK : 
     xcrun --sdk iphoneos --show-sdk-version
     xcrun --sdk macosx --show-sdk-version
  8. Lister tous les certificats installés sur ton Mac (si besoin) :
     security find-identity -v -p codesigning
  9. Download and Install (pour le script de compilation des shaders hors ligne, notamment pour la compilation de metal / metallib : xcodebuild -downloadComponent MetalToolchain
  ```
  
3. Avant toute compilation, exécute le script suivant :

```bash
cmake -P cmake/setup_dependencies.cmake
```

Ce script va :
- Lire `dependencies.txt`
- Cloner chaque dépôt dans `dependencies/`
- Faire un `git reset --hard` au commit_sha/tag fourni
- Initialiser les sous-modules si présents dans les librairies cloner

<br /><br />

---

<br /><br />

## 🔄 Updating Dependencies
Pour mettre à jour une ou des dépendance :
1. Modifiez le tag/commit_sha dans `dependencies.txt` de la librairie souhaiter.
2. Exécutez le script à la racine du projet :
```bash
cmake -P cmake/setup_dependencies.cmake
```

## Mettre à jour la base de donnée des gamepad
1. Récupérer la valeur du fichier appeler `gamecontrollerdb.txt` depuis : `https://github.com/mdqinc/SDL_GameControllerDB`
2. Copier/Coller la valeur du fichier dans `platforms/all/gamecontroller-db/gamecontrollersdl3-db.txt`
3. Télécharger et Installer Python >= 3.x.x
4. Run le script `generate_gamecontrollerdb_embedded.py`
```bash
# Unix
chmod +x ./build-scripts/generate_gamecontrollerdb_embedded.py
python ./build-scripts/generate_gamecontrollerdb_embedded.py

# Windows
python .\build-scripts\generate_gamecontrollerdb_embedded.py
```
Cela aura générer à nouveau le fichier à jour dans : `src/RC2D/RC2D_gamecontrollerdb_embedded.c` par rapport au nouveau `gamecontrollersdl3-db.txt`.

<br /><br />

---

<br /><br />

## 🧱 Générer RC2D (lib statique) + Projet d'exemple
1. **Par défaut** : ces scripts **génèrent un projet CMake** dans `./build/`, puis **compilent RC2D en bibliothèque statique** et **construisent le projet d’exemple** pour la plateforme choisie.

   - ✅ **Si le projet est déjà généré** (ex: solution **Visual Studio 2022**, projet Xcode, Ninja, etc.) : vous pouvez simplement **recompiler depuis votre IDE** ou via votre outil de build (Build/Run) **sans relancer les scripts**, tant que la configuration CMake ne change pas.

   - 🔁 **Quand relancer les scripts (ou rerun CMake)** :
     - Si vous modifiez des options CMake / flags / dépendances (ex: activation d’un module, ajout de libs, changement de toolchain, mise à jour `dependencies.txt`, etc.)
     - Si vous supprimez le dossier `build/` ou changez de plateforme/architecture/générateur.

   - 🧩 **Qu’est-ce qui demande une recompilation ?**
     - Si vous modifiez `src/RC2D/**` ou `include/RC2D/**` → vous modifiez la **lib RC2D** → **recompiler RC2D** (IDE ou scripts).
     - Si vous modifiez `examples/src/**` ou `examples/include/**` → vous modifiez **l’exemple** → **recompiler l’exemple** (IDE ou scripts).

```bash
# Linux - x64
chmod +x ./build-scripts/generate-project/linux-x64.sh
./build-scripts/generate-project/linux-x64.sh
# Spécifiquement pour Linux sur un VPS ou autres, après compilation réussi, lancer l'exemple via : 
# Executable classique : SDL_AUDIODRIVER=dummy ./rc2d_example
# Executable .AppImage : SDL_AUDIODRIVER=dummy ./rc2d_example.AppImage


# Linux - arm64
chmod +x ./build-scripts/generate-project/linux-arm64.sh
./build-scripts/generate-project/linux-arm64.sh
# Spécifiquement pour Linux sur un VPS ou autres, après compilation réussi, lancer l'exemple via : 
# Executable classique : SDL_AUDIODRIVER=dummy ./rc2d_example
# Executable .AppImage : SDL_AUDIODRIVER=dummy ./rc2d_example.AppImage


# macOS (Apple Silicon arm64) - No Signed Bundle .app for Project Example
chmod +x ./build-scripts/generate-project/macos-arm64-nosignedbundleapp.sh
./build-scripts/generate-project/macos-arm64-nosignedbundleapp.sh


# macOS (Apple Silicon arm64) - Signed Bundle .app for Project Example
chmod +x ./build-scripts/generate-project/macos-arm64-signedbundleapp.sh
./build-scripts/generate-project/macos-arm64-signedbundleapp.sh
# Informations : Rentrer les bonne informations dans le CMakelists ou il y a "RC2D_BUILD_EXAMPLES_APPLE_CODE_SIGNING"
# Concernant : XCODE_ATTRIBUTE_DEVELOPMENT_TEAM, XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY..etc


# Windows - x64
.\build-scripts\generate-project\windows-x64.bat


# Windows - arm64
.\build-scripts\generate-project\windows-arm64.bat


# SteamRT4 - x64
# Depuis Docker (Les artefacts générés (Debug / Release) sont disponibles dans le dossier : docker-build-output-steamrt4/): 
docker compose run --rm rc2d-builder-x64
# Depuis le script :
chmod +x ./build-scripts/generate-project/steamrt4-arm64.sh
./build-scripts/generate-project/steamrt4-arm64.sh


# SteamRT4 - arm64
# Depuis Docker (Les artefacts générés (Debug / Release) sont disponibles dans le dossier : docker-build-output-steamrt4/): 
docker compose run --rm rc2d-builder-arm64
# Depuis le script :
chmod +x ./build-scripts/generate-project/steamrt4-x64.sh
./build-scripts/generate-project/steamrt4-x64.sh


# Android - Unix (Linux x64/arm64 or macOS Apple Silicon arm64)
chmod +x ./build-scripts/generate-project/android-unix.sh
./build-scripts/generate-project/android-unix.sh
# Generate la librairie static RC2D + APK/AAB (donc le projet d'exemple RC2D qui seras dans le AAB/APK)


# Android - Windows (x64/arm64)
.\build-scripts\generate-project\android-windows.bat
# Generate la librairie static RC2D + APK/AAB (donc le projet d'exemple RC2D qui seras dans le AAB/APK)


# iOS (run in macOS) - No Signed Bundle .app for Project Example
chmod +x ./build-scripts/generate-project/ios-iphoneos-arm64-nosignedbundleapp.sh
./build-scripts/generate-project/ios-iphoneos-arm64-nosignedbundleapp.sh


# iOS (run in macOS) - Signed Bundle .app for Project Example
chmod +x ./build-scripts/generate-project/ios-iphoneos-arm64-signedbundleapp.sh
./build-scripts/generate-project/ios-iphoneos-arm64-signedbundleapp.sh
# Informations : Rentrer les bonne informations dans le CMakelists ou il y a "RC2D_BUILD_EXAMPLES_APPLE_CODE_SIGNING"
# Concernant : XCODE_ATTRIBUTE_DEVELOPMENT_TEAM, XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY..etc
```
3. Il y a un dossier `build` à la racine qui est générer.
```bash
# Pour Windows x64 par exemple, un projet Visual Studio 2022 à été générer au path suivant :
.\build\windows\x64

# La librairie RC2D static + l'exemple générer dans le même dossier :
Release : .\build\windows\x64\Debug
Debug : .\build\windows\x64\Release
```
4. Ouvrir le projet générer dans votre IDE favoris.

<br /><br />

## 📲 Build & Deploy Mobile (Android / iOS) - For Development
Les scripts dans `build-scripts/build-deploy-mobile/` permettent de **build + installer + lancer** l’app d’exemple RC2D directement sur un appareil réel.

## 🤖 Android
### Scripts
- macOS / Linux : `build-scripts/build-deploy-mobile/android-unix.sh`
- Windows : `build-scripts/build-deploy-mobile/android-windows.bat`

### 🔧 Variables à modifier
En haut du script :
```bash
# APP_COMPONENT = PACKAGE_NAME/.ACTIVITY_NAME (important quand tu changes d’app)
APP_COMPONENT="com.crzgames.testexe/.MyGame"
```

### 📋 Prérequis
Pour que le script `android-unix.sh ou android-windows.bat` fonctionne, il faut que :

#### 💻 Sur le Windows/macOS/Linux
- ✅ La valeur de la variable `APP_COMPONENT` du script (`android-unix.sh ou android-windows.bat`) corresponde exactement au package de l'application Android.

#### 📱 Sur l'Android
- ✅ Mode Developer Options activé
- ✅ USB Debugging activé
- ✅ Popup “Allow USB debugging” acceptée

### ▶️ Exécuter
```bash
# Linux x64/arm64 - macOS Apple Silicon arm64
chmod +x ./build-scripts/build-deploy-mobile/android-unix.sh
./build-scripts/build-deploy-mobile/android-unix.sh

# Windows x64/arm64
.\build-scripts\build-deploy-mobile\android-windows.bat
```

<br />

## 🍎 iOS
### Scripts
- macOS : `build-scripts/build-deploy-mobile/ios.sh`

### 🔧 Variables à modifier
En haut du script :
```bash
# Bundle ID (important quand tu changes d’app)
BUNDLE_ID="com.crzgames.testexe"

# Pattern de logs (le nom de l’exécutable / target CMake)
PATTERN="rc2d_example"
```

### 🔐 Configuration de Signature iOS (OBLIGATOIRE)
La signature est configurée dans le CMakeLists.txt via :
```bash
if(RC2D_BUILD_EXAMPLES_APPLE_CODE_SIGNING)
  set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${APP_IOSMACOS_DEVELOPMENT_TEAM_ID} # Remplacez par votre Team ID Apple
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${APP_IOS_CODE_SIGN_IDENTITY} # Remplacez par votre identité de signature
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual" # Style de signature (Manual ou Automatic)
    XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --strict --force --timestamp --verbose" # Options supplémentaires pour la signature
    XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ${APP_IOS_PROVISIONING_PROFILE_NAME} # Remplacez par le nom de votre profil de provisionnement
    XCODE_ATTRIBUTE_PROVISIONING_PROFILE ${APP_IOS_PROVISIONING_PROFILE_UUID} # Remplacez par l'UUID de votre profil de provisionnement
  )
endif()
```

### 📋 Prérequis iOS (Apple Developer + Device)
Pour que le script `ios.sh` fonctionne, il faut que : 

#### 💻 Sur le Mac
- ✅ Certificat Apple Development installé sur le Mac
- ✅ Provisioning Profile de type Developpement valide installé sur le Mac
- ✅ La valeur de la variable `BUNDLE_ID` du script (`ios.sh`) corresponde exactement à celui signé (donc l'identifier exemple : com.crzgames.testexe)
- ✅ La valeur de la variable `PATTERN` du script (`ios.sh`) corresponde exactement à la target CMake donc au nom du `project()` dans le `CMakelists.txt`.

#### 📱 Sur l’iPhone
- ✅ L’iPhone soit enregistré dans ton compte Apple Developer (Devices → UDID ajouté)
- ✅ L’iPhone soit : 
  - Déverrouillé
  - Branché en USB
  - Accepté la popup quand on branche en usb "faire confiance à cette ordinateur"
  - Developer Mode activé
  - iOS version ≥ 18.0 (selon les prérequis RC2D)

### ▶️ Exécuter
```bash
# macOS Apple Silicon arm64
chmod +x ./build-scripts/build-deploy-mobile/ios.sh
./build-scripts/build-deploy-mobile/ios.sh
```

<br /><br />

## Project Example - Shaders
Pour la compilation hors ligne des shaders, il faut utilisé les scripts à disposition via :
```bash
# Unix
chmod +x ./examples/shaders/scripts/compile-shaders-unix.sh
cd ./examples/shaders/scripts/ # Il faut executer obligatoirement le script de compilation de shaders à partir du dossier "scripts"
./compile-shaders-unix.sh

# Windows
cd .\examples\shaders\scripts\ # Il faut executer obligatoirement le script de compilation de shaders à partir du dossier "scripts"
.\compile-shaders-windows.bat
```

<br /><br />

---

<br /><br />

## Production
### ⚙️➡️ Automatic Distribution Process (CI / CD)
#### Si c'est un nouveau projet suivez les instructions : 
1. Ajoutées les SECRETS_GITHUB pour :
   - ... TODO
   - PAT (crée un nouveau token si besoin sur le site de github puis dans le menu du "Profil" puis -> "Settings" -> "Developper Settings' -> 'Personnal Access Tokens' -> Tokens (classic))
