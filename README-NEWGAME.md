# 🎮 RC2D Example (Créer un nouveau jeu)

RC2D fournit un projet d’exemple **cross-platform**.  
Quand tu crées un nouveau jeu, la majorité des changements à faire concernent :

- ✅ **Android** (package / Activity / libs / nom affiché)
- ✅ **Icônes** (une seule image → génération automatique pour toutes les plateformes)
- ✅ **Métadonnées globales CMake** 

<br /><br />

---

<br /><br />

# 🤖 Android

## 1️⃣ Déplacer / renommer le package Java (dossiers)

### Chemin actuel :
```
android-project/app/src/main/java/com/crzgames/testexe/
```

### Exemple nouveau package :
```
android-project/app/src/main/java/com/mycompany/mygame/
```

> ⚠️ Important : les dossiers doivent correspondre exactement au package (même structure).

<br />

## 2️⃣ Mettre à jour le package dans `MyGame.java`

### Fichier :
```
android-project/app/src/main/java/com/crzgames/testexe/MyGame.java
```

### Remplace :
```java
package com.crzgames.testexe;
```

### Par :
```java
package com.mycompany.mygame;
```

<br />

## 3️⃣ Mettre à jour Gradle : `applicationId` + `namespace`

### Fichier :
```
android-project/app/build.gradle
```

### Modifie :
- `applicationId`
- `namespace`

### Exemple :
```gradle
android {
  namespace "com.mycompany.mygame"
  defaultConfig {
    applicationId "com.mycompany.mygame"
  }
}
```

### 🔎 Explication

- namespace → utilisé par Android Gradle Plugin
- applicationId → identifiant final APK / Play Store

Ils doivent correspondre exactement au package Java.

<br />

## 4️⃣ Modifier AndroidManifest.xml

Fichier :

android-project/app/src/main/AndroidManifest.xml

Modifier cette ligne :

<activity android:name="com.crzgames.testexe.MyGame"

Par :

<activity android:name="com.mycompany.mygame.MyGame"

### 🔎 Pourquoi ?

Android doit savoir quelle classe lancer au démarrage.

Si cette valeur ne correspond pas :
- Crash immédiat au lancement
- ClassNotFoundException

<br />

## 4️⃣ Changer le nom affiché du jeu

Android ne permet pas d’utiliser uniquement `SDL_SetWindowTitle` pour le nom système de l’app.

### Fichier :
```
android-project/app/src/main/res/values/strings.xml
```

C’est ici que tu modifies le nom du jeu.

<br />

## 4️⃣ Ajouter une nouvelle dépendance `.so` + la charger (si besoin)

Si tu ajoutes une lib dynamique supplémentaire :

### 1️⃣ Déclarer le chemin dans `build.gradle`

Toujours dans :
```
android-project/app/build.gradle
```

Ajoute les dossiers dans :

```
jniLibs.srcDirs
```

Cela permet d’ajouter des chemins qui contiennent des `.so`.

### 2️⃣ Charger la lib dans `MyGame.java`

Dans `MyGame.java` :

```java
@Override
protected String[] getLibraries() {
  return new String[] {
    "SDL3",
    "SDL3_image",
    "SDL3_mixer",
    "SDL3_ttf",
    "avcodec",
    "avdevice",
    "avfilter",
    "avformat",
    "avutil",
    "swresample",
    "swscale",
    "onnxruntime",
    "main"
  };
}
```

### ✅ Règle importante :

- Ajoute ta nouvelle lib dans ce tableau
- Respecte l’ordre :

1. `SDL3` en premier  
2. `SDL3_*` juste après  
3. Tes libs externes ensuite  
4. `"main"` toujours en dernier  

<br /><br />

---

<br /><br />

# 🖼️ Icônes — 1 image → génération automatique toutes plateformes

RC2D centralise les icônes via une image source unique.

## Fichier source :
```
icons/app-icon-default.png
```

## ✅ Contraintes :
- 1024x1024  
- Format PNG  

## 🚀 Générer les icônes

### macOS / Linux :
```bash
chmod +x ./build-scripts/generate-icons/generate-icons-unix.sh
./build-scripts/generate-icons/generate-icons-unix.sh
```

### Windows :
```bat
.\build-scripts\generate-icons\generate-icons-windows.bat
```

Une fois exécuté, tout est régénéré automatiquement pour toutes les plateformes supportées  
(icônes app / bundle / ressources).

<br /><br />

---

<br /><br />

# 🧾 Métadonnées globales (CMake)

Dans CMakeLists.txt tu as :

APP_VERSION  
APP_COMPANY_NAME  
APP_GAME_DESCRIPTION  
APP_LEGAL_COPYRIGHT  
APP_IOSMACOS_BUILD_VERSION  
APP_IOSMACOS_IDENTIFIER  

Exemple :

if(DEFINED ENV{APP_IOSMACOS_IDENTIFIER})
    set(APP_IOSMACOS_IDENTIFIER "$ENV{APP_IOSMACOS_IDENTIFIER}")
else()
    set(APP_IOSMACOS_IDENTIFIER "com.crzgames.testexe")
endif()
