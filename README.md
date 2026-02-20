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
├── 📁 build-scripts                  # Scripts de build personnalisés (.sh / .bat), puis les scripts utilise le CMakelists.txt
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
│   ├── 📁 external/lz4               # Dossier "external" qui contient des librairies link statiquement depuis leur fichier source directement et intégrer à RC2D.
├── 📁 platforms                      # Contient la base de donnée des gamecontrollersdl, des choses spécifique au plateforme pour le projet d'exemple
├── 📁 src                            # Code source interne de la bibliothèque RC2D (implémentations .c)
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

| Librairie | Version / Commit SHA utilisé par RC2D | Rôle dans RC2D | Statut / Intégration |
|------------|----------------------------------------|----------------|----------------------|
| **LZ4** | v1.10.0 | Compression ultra-rapide utilisée par `RC2D_data` | 🔒 Intégrée statiquement (sources embarquées dans RC2D) |
| **SDL3** | commit `be82f316c4745d4cf0f8c0a5e37f5390beed9542` | Gestion fenêtre, entrées, rendu GPU | ⭐ Obligatoire |
| **SDL3_image** | commit `8bd9f3d7f2d2bb59ce4331f13b77d65254cd8c7b` | Chargement d’images (PNG, SVG, APNG…) | ⭐ Obligatoire |
| **SDL3_ttf** | commit `053bbc89517471427748a082583c9eada55c07b5` | Rendu de polices TrueType | ⭐ Obligatoire |
| **SDL3_mixer** | commit `37b2f3325a0fb1e98ba265aa38826aa9e16624fb` | Gestion audio (WAV, OGG, OPUS…) | ⭐ Obligatoire |
| **cJSON** | v1.7.19 | Parsing JSON léger | 🔒 Statique – linké à la compilation de RC2D |
| **SDL3_shadercross** | commit `7b7365a86611b2a7b6462e521cf1c43a037d0970` | Transpilation shaders (HLSL → MSL / SPIR-V / DXIL / METALLIB…) | 🟡 Dev uniquement – `RC2D_GPU_SHADER_HOT_RELOAD_ENABLED` |
| **RCENet** | v1.4.0 | Communication réseau UDP (fork ENet) | 🟡 Optionnel – `RC2D_NET_MODULE_ENABLED` (ON par défaut) |
| **OpenSSL** | v3.6.1 | Hashing, chiffrement, crypto | 🟡 Optionnel – `RC2D_DATA_MODULE_ENABLED` (ON par défaut) |
| **ONNX Runtime** | v1.24.1 | Inférence IA (modèles ONNX) | 🔵 Optionnel – `RC2D_ONNX_MODULE_ENABLED` (OFF par défaut) |
| **FFmpeg** | v8.0.0 | Lecture vidéo (MP4, etc.) | 🟡 Optionnel – `RC2D_VIDEO_MODULE_ENABLED` (ON par défaut) |

<br /><br />

---

<br /><br />

## ⚙️ Setup Environment Development
1. Cloner le projet :
  ```bash
  git clone git@github.com:CrzGames/Crzgames_RC2D.git
  ```
2. (Optional) Download and Install Node.js >= 18.0.0 (pour lancer la documentation, pour Vitepress).
3. Steps by Platform :
  ```bash  
  # Windows :
  1. Requirements : Windows >= 10 (x64 or arm64)
  2. Download and Install Visual Studio == 2022 (MSVC >= v143 + Windows SDK >= 10) : https://visualstudio.microsoft.com/fr/downloads/
  3. Download and Install CMake >= 3.28.0 : https://cmake.org/download/ and add PATH ENVIRONMENT.


  # Linux :
  1. Requirements : glibc >= 2.35.0 (Exemple : Ubuntu >= 22.04 OR Debian >= 12.0), checker via : ldd --version
  2. Download and Install (gcc, g++, make..) :
     sudo apt update
     sudo apt install -y build-essential
  3. Download and Install CMake >= 3.28.0 : sudo apt install -y cmake
  4. Download and Install Patchelf : sudo apt install -y patchelf
  5. Download and Install dependencies dev for build :
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



  # SteamRT4 (Steam Linux / Steam Deck) :
  1. Utiliser un container via le tag de l image SteamRT4 voir : https://repo.steampowered.com/steamrt4/images/
  2. Download and Install (gcc, g++, make..) :
     sudo apt update
     sudo apt install -y build-essential
  3. Download and Install CMake >= 3.28.0 : sudo apt install -y cmake
  4. Download and Install Patchelf : sudo apt install -y patchelf
  5. Download and Install dependencies dev for build :
    sudo apt-get update
    sudo apt-get -y install build-essential git make \
    pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
    libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
    libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
    libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev \
    ibpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev
  6. Download and Install Patchelf : sudo apt install -y patchelf



  # macOS :
  1. Requirements : MacOS X >= 15.0.0
  2. Download and Install xCode >= 16.4.0
  3. Download and Install Command Line Tools : xcode-select --install
  4. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  5. Download and Install CMake >= 3.28.0 : brew install cmake



  # Android (only Linux) :
  1. Download and Install : Android Studio 2025.3.1 or newer
  2. Add environment variable: ANDROID_HOME for path SDK Android (SDK Manager path), sous Windows en général : C:\Users\Corentin\AppData\Local\Android\Sdk
  3. Ouvrir Android Studio et installer certains composants du SDK : Android v16.0, Android SDK v36.1.0, NDK v29.0.14206865, Android SDK Command Line v20, CMake v3.30.3.
  4. Download and Install CMake >= 3.28.0 and add PATH ENVIRONMENT.
  5. Download and Install Java JDK 17 LTS (Temurin) : https://adoptium.net/fr/temurin/releases?version=17&os=any&arch=any  (Pendant l installation du SDK Java de Temurin cocher la case pour ajouté automatiquement la variable d environnement JAVA_HOME)
  6. Si vous avez oublié de coché la case pour ajouter automatiquement la variable d environnement "JAVA_HOME", sous Windows en général : C:\Program Files\Eclipse Adoptium\jdk-17.0.18.8-hotspot
  7. Download and Install Patchelf : sudo apt install -y patchelf



  # iOS (only macOS) :
  1. Requirements : MacOS X >= 15.0.0
  2. Download and Install xCode >= 16.4.0
  3. Download and Install SDK iOS >= 18.0.0
  4. Download and Install Command Line Tools : xcode-select --install
  5. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  6. Download and Install cmake >= 3.28.0 : brew install cmake 
  ```
  
4. Avant toute compilation, exécute le script suivant :

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


# macOS (Apple Silicon arm64) - No Signed Bundle .app
chmod +x ./build-scripts/generate-project/ios-iphoneos-arm64-nosignedbundleapp.sh
./build-scripts/generate-project/ios-iphoneos-arm64-nosignedbundleapp.sh


# macOS (Apple Silicon arm64) - Signed Bundle .app
chmod +x ./build-scripts/generate-project/ios-iphoneos-arm64-signedbundleapp.sh
./build-scripts/generate-project/ios-iphoneos-arm64-signedbundleapp.sh
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


# Android (run in Linux)
chmod +x ./build-scripts/generate-project/android-linux.sh
./build-scripts/generate-project/android-linux.sh


# iOS (run in macOS) - No Signed Bundle .app
chmod +x ./build-scripts/generate-project/ios-iphoneos-arm64-nosignedbundleapp.sh
./build-scripts/generate-project/ios-iphoneos-arm64-nosignedbundleapp.sh


# iOS (run in macOS) - Signed Bundle .app
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

---

<br /><br />

## Production
### ⚙️➡️ Automatic Distribution Process (CI / CD)
#### Si c'est un nouveau projet suivez les instructions : 
1. Ajoutées les SECRETS_GITHUB pour :
   - ... TODO
   - PAT (crée un nouveau token si besoin sur le site de github puis dans le menu du "Profil" puis -> "Settings" -> "Developper Settings' -> 'Personnal Access Tokens' -> Tokens (classic))
