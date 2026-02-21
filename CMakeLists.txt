# CMake 3.28.0 est la version minimale requise pour ce projet
cmake_minimum_required(VERSION 3.28.0)

# ============================================================
# Project Configuration
# ============================================================
# Nom du projet : rc2d
#
# Langages :
#   - C   : cœur du moteur (C99)
#   - CXX : requis uniquement pour :
#       • Steamworks (SDK en C++) si RC2D_STEAMWORKS_SDK_ENABLED=ON
#         → compile src/RC2D/RC2D_steamworks.cpp
#       • Les exemples du projet (sources en C++)
#
# Même si RC2D est majoritairement en C, le support C++ est
# nécessaire pour certaines intégrations et pour les exemples.
#
# Version actuelle : 1.0.0
# ============================================================
project(rc2d
  LANGUAGES C CXX
  VERSION 1.0.0
)

# ============================================================
# Language Standards
# ============================================================
# C   : RC2D core est écrit en C99.
#
# CXX : requis uniquement pour :
#       • Steamworks SDK si RC2D_STEAMWORKS_SDK_ENABLED=ON
#         → compile src/RC2D/RC2D_steamworks.cpp (SDK Steam en C++)
#       • Le projet d’exemple (sources en C++)
#
# Le moteur principal reste en C.
# ============================================================
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Build Options (Feature toggles)
# ============================================================
# Toutes les options RC2D_* qui permettent d'activer/désactiver
# des modules, features ou intégrations SDK.
# ============================================================

# Option pour activer le débogage mémoire, pour savoir les fuites de mémoire si il manque des free()
# CONSEILLE : Désactivé pour la production (RC2D_MEMORY_DEBUG_ENABLED=OFF).
option(RC2D_MEMORY_DEBUG_ENABLED "Enable RC2D memory debug tracking (realloc/malloc/free.. wrappers)" ON)

# Option pour activer la compilation des shaders en ligne (hot reload) pour SDL3_shadercross
# CONSEILLE : Désactivé pour la production (RC2D_GPU_SHADER_HOT_RELOAD_ENABLED=OFF).
option(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED "Enable RC2D GPU shader hot reload (using SDL3_shadercross)" OFF)

# Option pour activer le module RC2D_video (basé sur FFmpeg)
option(RC2D_VIDEO_MODULE_ENABLED "Enable RC2D_video (FFmpeg-based)" ON)

# Option pour activer le module RC2D_data (basé sur OpenSSL)
option(RC2D_DATA_MODULE_ENABLED "Enable RC2D_data (OpenSSL-based)" ON)

# Option pour activer le module RC2D_net (basé sur RCEnet, qui est lui même basé sur ENet)
option(RC2D_NET_MODULE_ENABLED "Enable RC2D_net (ENet-based)" ON)

# Option pour activer le module RC2D_onnx (basé sur ONNX Runtime)
option(RC2D_ONNX_MODULE_ENABLED "Enable RC2D_onnx (ONNX Runtime-based)" ON)

# Option pour le SDK Steamworks
option(RC2D_STEAMWORKS_SDK_ENABLED "Enable RC2D Steamworks SDK integration" OFF)

# Option pour le SDK EOS (Epic Online Services)
option(RC2D_EOS_SDK_ENABLED "Enable RC2D Epic Online Services (EOS) SDK integration" OFF)

# Option pour le niveau d'assertion
option(RC2D_ASSERT_LEVEL "Niveau des assertions (0=none, 1=release, 2=debug, 3=paranoid)" 3)

# Option pour construire les exemples
option(RC2D_BUILD_EXAMPLES "Build examples" ON)

# Option pour activer la signature du bundle de l’exemple sur Apple (iOS/macOS)
option(RC2D_BUILD_EXAMPLES_APPLE_CODE_SIGNING "Enable code signing for Apple examples (iOS/macOS)" OFF)

# Option Tests unitaires avec Criterion (TODO: Pas encore implémenté pour les tests)
option(RC2D_BUILD_TESTS "Build unit tests with Criterion" OFF)

# Option pour choisir entre statique et dynamique (TODO: Pas encore implémenté pour le mode shared)
option(RC2D_BUILDING_SHARED "Build shared libraries" OFF)

# ============================================================
# Output Directories
# ============================================================
# Gestion du dossier de sortie selon le générateur CMake :
# - Visual Studio / Xcode : multi-config (Debug/Release/...)
# - Ninja / Makefiles     : single-config
# ============================================================
if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
  # Multi-config generators
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
else()
  # Single-config generators (Makefiles, Ninja, etc.)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# ============================================================
# Platform Minimum Versions / Architectures
# ============================================================
# Définit les versions minimales supportées (Windows / Apple / Android)
# et les architectures cibles (ex: arm64 sur Apple).
# ============================================================
if(WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.10240 CACHE STRING "Windows 10 Minimum Version" FORCE)
elseif(APPLE)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS Architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "macOS Deployment Target" FORCE)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS Architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "18.0" CACHE STRING "iOS Deployment Target" FORCE)
  endif()
elseif(ANDROID)
  set(CMAKE_ANDROID_API 28 CACHE STRING "Android API Level" FORCE)
endif()

# ============================================================
# Compiler Warnings & Strictness
# ============================================================
# Flags d'avertissements et de conformité :
# - MSVC : /W4, /permissive- (C++ strict), désactivations ciblées
# - GCC/Clang : -Wall -Wextra -pedantic + exceptions ciblées
#
# Note :
# - Les flags C et CXX ne sont pas toujours identiques (ex: strict-prototypes)
# ============================================================
if(MSVC)
  # /W4 : Niveau d'avertissement 4 (equivalent a -Wextra)
  # /permissive- : Spécifie un mode de compilation strict pour suivre les standards C++ 
  # /wd4100 : Désactive l'avertissement pour les paramètres de fonction non utilisés (C4100)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /wd4100")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive- /wd4100")
else()
  # -Wall : Active tous les avertissements de base
  # -Wextra : Active des avertissements supplémentaires
  # -pedantic : Enforce strictement les standards C++
  # -Wno-unused-parameter : Désactive les avertissements pour les paramètres de fonction non utilisés
  # -Wno-error=strict-prototypes : Désactive les erreurs pour les prototypes de fonction stricts en C.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-error=strict-prototypes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter")

  # Clang a un warning spécifique pour les fichiers qui ne se terminent pas par une nouvelle ligne (newline at end of file).
  # -Wno-newline-eof : Désactive les avertissements pour les nouvelles lignes à la fin du fichier
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-newline-eof")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-newline-eof")
  endif()
endif()

# ============================================================
# Linux Tools Prerequisites
# ============================================================
# Linux / Android / SteamRT4 / Raspberry Pi, etc. :
# On vérifie que patchelf est installé pour pouvoir patcher les .so après la compilation,
# afin que les .so soient trouvées à l'exécution (RPATH = $ORIGIN).
# ============================================================
if(UNIX AND NOT APPLE)
  find_program(PATCHELF_EXECUTABLE NAMES patchelf)
  if(NOT PATCHELF_EXECUTABLE)
    message(FATAL_ERROR "patchelf non trouvé dans le PATH")
  endif()
endif()

# ============================================================
# SDL3_ttf / SDL3_image / SDL3_mixer — Mode "vendored"
# ============================================================
# On force CMake à utiliser les dépendances fournies dans le repo (dossier dependencies/)
# au lieu de chercher des libs système (apt, pacman, brew, etc.).
#
# Objectif :
# - Build reproductible (CI/CD) sur toutes les plateformes
# - Éviter les différences de versions entre machines
# ============================================================
set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)

# ============================================================
# SDL3_mixer — Configuration formats audio
# ============================================================
# SDL3_mixer active pas mal de codecs par défaut.
# Ici on désactive explicitement ceux dont on ne veut pas, pour :
# - Réduire le temps de compilation
# - Réduire les dépendances (libs externes)
# - Réduire la taille finale (binaire + libs)
#
# Formats conservés par défaut :
# - opus               (.opus)
# - vorbis/vorbisfile  (.ogg / .oga)
# - wave               (.wav)
# ============================================================
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
endif()
set(SDLMIXER_MP3 OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3_DRMP3 OFF CACHE BOOL "" FORCE)
set(SDLMIXER_AIFF OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VOC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_AU OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC_LIBFLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_FLAC_DRFLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD_XMP OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD_XMP_LITE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI_FLUIDSYNTH OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI_TIMIDITY OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_STB OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_TREMOR OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK_DSD OFF CACHE BOOL "" FORCE)

# ============================================================
# SDL3_image — Configuration formats image
# ============================================================
# SDL3_image active pas mal de formats par défaut.
# Ici on désactive explicitement ceux dont on ne veut pas, pour :
# - Réduire le temps de compilation
# - Réduire les dépendances (libs externes)
# - Réduire la taille finale (binaire + libs)
#
# Formats conservés par défaut :
# - png   (.png)
# - svg   (.svg)
# - apng  (.apng)
# ============================================================
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)

  # Pour APNG, on doit aussi forcer libpng et zlib en statique, car ils sont utilisés par SDL3_image pour le support APNG.
  set(SDLIMAGE_PNG_SHARED  OFF CACHE BOOL "" FORCE)
  set(PNG_SHARED OFF CACHE BOOL "" FORCE)
  set(PNG_STATIC ON  CACHE BOOL "" FORCE)
  set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(ZLIB_BUILD_STATIC ON  CACHE BOOL "" FORCE)
endif()
# Formats conservés
set(SDLIMAGE_PNG ON CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG_LIBPNG ON CACHE BOOL "" FORCE) # active APNG
set(SDLIMAGE_SVG ON CACHE BOOL "" FORCE)
# Formats désactivés
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BMP  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_GIF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JPG  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_LBM  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PCX  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNM  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_QOI  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TGA  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XCF  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XPM  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XV   OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_ANI  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL  OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB OFF CACHE BOOL "" FORCE)

# ============================================================
# SDL3 — Input / Controllers configuration
# ============================================================
# Objectif : garantir une compatibilité maximale des manettes, sur toutes les plateformes.
#
# Remarque :
# - SDL a plusieurs backends (HIDAPI, XInput, RawInput, etc.)
# - Ici on active ceux qui apportent le plus en pratique
# ============================================================

# Active le support global des joysticks (base nécessaire pour toute manette)
set(SDL_JOYSTICK ON CACHE BOOL "Enable joystick support" FORCE)
# Active l’API GameController (mapping standardisé A/B/X/Y, triggers, etc.)
# Permet d’avoir une disposition uniforme entre toutes les manettes
set(SDL_GAMECONTROLLER ON CACHE BOOL "Enable gamecontroller API" FORCE)
# Active HIDAPI (accès direct USB/Bluetooth aux manettes modernes)
# Améliore la compatibilité et évite de dépendre uniquement des drivers OS
set(SDL_JOYSTICK_HIDAPI ON CACHE BOOL "Enable HIDAPI joystick backend" FORCE)
if(WIN32) # Windows
  # Active XInput (Windows) pour un support optimal des manettes Xbox
  # Meilleure détection, vibrations plus fiables
  set(SDL_JOYSTICK_XINPUT ON CACHE BOOL "Enable XInput controller backend on Windows" FORCE)
  # Active RawInput (Windows)
  # Permet une détection plus bas niveau des périphériques USB
  # Utile comme fallback si XInput ne voit pas certains devices
  set(SDL_JOYSTICK_RAWINPUT ON CACHE BOOL "Enable RawInput joystick backend on Windows" FORCE)
endif()
# Active le support des vibrations / retour de force
# Fonctionne avec la plupart des manettes modernes
set(SDL_HAPTIC ON CACHE BOOL "Enable haptic (rumble/force feedback) support" FORCE)

# ============================================================
# iOS — Forcer les libs SDL en statique
# ============================================================
# iOS : les .dylib sont interdites sur l'App Store.
# On force donc SDL3 + SDL3_image + SDL3_mixer + SDL3_ttf à être statiques.
# ============================================================
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif()

# ============================================================
# cJSON — Configuration build
# ============================================================
# Objectif :
# - Build en statique uniquement
# - Pas de tests / utilitaires (inutile dans un framework)
# ============================================================
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Allow old subprojects" FORCE) # Autoriser les vieux subprojects de CJSON
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON  CACHE BOOL "" FORCE) # ignore BUILD_SHARED_LIBS global
set(CJSON_BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE) # force STATIC pour cJSON
set(ENABLE_CJSON_TEST   OFF CACHE BOOL "" FORCE) # Désactiver les tests
set(ENABLE_CJSON_UTILS  OFF CACHE BOOL "" FORCE) # Désactiver les outils

# ============================================================
# Dépendances vendored — add_subdirectory
# ============================================================
# On inclut SDL + modules, et cJSON depuis dependencies/.
# EXCLUDE_FROM_ALL : évite de builder ces targets si aucune target ne les référence.
# ============================================================
add_subdirectory(dependencies/SDL        EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/SDL_ttf    EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/SDL_image  EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/SDL_mixer  EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/cJSON      EXCLUDE_FROM_ALL)

# ============================================================
# Paths — Dépendances précompilées (Crzgames_Libraries)
# ============================================================
# DEPENDENCIES_PATH : racine de ton dossier dependencies/
# CRZGAMES_LIBRARIES: dossier où on ranges les libs précompilées (ffmpeg, openssl, rcenet, onnxruntime, sdl3shadercross, etc.) pour chaque plateforme.
# ============================================================
set(DEPENDENCIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
set(CRZGAMES_LIBRARIES "${DEPENDENCIES_PATH}/Crzgames_Libraries")

# ============================================================
# rc2d_configure_cjson
#
# Configure l’intégration de cJSON pour une target donnée.
#
# Objectif :
# - Exposer les headers cJSON via include path (PUBLIC)
# - Lier la lib statique cJSON à la target
#
# Pourquoi une fonction :
# - Évite la duplication (lib rc2d + exécutable d’exemple)
# - Centralise la manière de consommer cJSON
# ============================================================
function(rc2d_configure_cjson target_name)
  # Le header cJSON.h est dans dependencies/cJSON/cJSON.h
  target_include_directories(${target_name} PUBLIC
    "${DEPENDENCIES_PATH}/cJSON"
  )

  # Link la librairie cJSON
  target_link_libraries(${target_name} PUBLIC cjson)
endfunction()

# ============================================================
# rc2d_configure_ffmpeg
#
# Configure FFmpeg pour une target donnée.
#
# Objectif :
# - Ajouter include + lib dirs selon la plateforme
# - Lier les libs FFmpeg nécessaires (avcodec, avformat, etc.)
# - Copier les libs runtime (DLL/.so/.dylib) quand nécessaire
#
# Notes :
# - FFmpeg est optionnel via RC2D_VIDEO_MODULE_ENABLED
# ============================================================
function(rc2d_configure_ffmpeg target_name)
  if (RC2D_VIDEO_MODULE_ENABLED)
    if (WIN32) # Windows
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/include_ffmpeg/ffmpeg")

      # Rajouter le répertoire de la lib FFmpeg, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/ffmpeg/${RC2D_ARCH}")

      # Link les librairies FFmpeg
      target_link_libraries(${target_name} PUBLIC
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
      )

      # Copier aussi les DLLs dans le répertoire de l'exécutable
      file(GLOB_RECURSE FFMPEG_DLLS
        "${CRZGAMES_LIBRARIES}/windows/bin/ffmpeg/${RC2D_ARCH}/*.dll"
      )
      foreach(FILE IN LISTS FFMPEG_DLLS)
        get_filename_component(FILE_NAME "${FILE}" NAME)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${FILE}"
                  "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
        )
      endforeach()
    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/include_ffmpeg/ffmpeg")

      # Ajouter explicitement la lib .dylib à linker
      target_link_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/macos/lib/ffmpeg"
      )

      # Link la librairies FFmpeg
      target_link_libraries(${target_name} PUBLIC
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
      )

      # Frameworks macOS requis par FFmpeg et d'autres dépendances
      target_link_libraries(${target_name} PUBLIC
          "-framework Security"
          "-framework CoreFoundation"
          "-framework VideoToolbox"
          "-framework CoreMedia"
          "-framework CoreVideo"     
          "-framework AudioToolbox"  # for AudioConverter*/AudioFormat*
          bz2                        # for BZ2_*
          z                          # for zlib (deflate/inflate/compress)
          iconv                      # for iconv_*
      )
    elseif (APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS") # iOS (iPhone/iPad)
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/include_ffmpeg/ffmpeg")

      # Ajouter explicitement la lib .a à linker
      target_link_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/ios/lib/ffmpeg/arm64-iphoneos"
      )

      # Link la librairies FFmpeg
      target_link_libraries(${target_name} PUBLIC
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
      )

      # Frameworks iOS requis par FFmpeg et d'autres dépendances
      target_link_libraries(${target_name} PUBLIC 
          "-framework Security"
          "-framework CoreFoundation"
          "-framework VideoToolbox"
          "-framework CoreMedia"
          "-framework CoreVideo"     
          "-framework AudioToolbox"  # for AudioConverter*/AudioFormat*
          bz2                        # for BZ2_*
          z                          # for zlib (deflate/inflate/compress)
          iconv                      # for iconv_*
      )

    elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, SteamRT4, Raspberry PI 5 etc.
      if (RC2D_PLATFORM STREQUAL "STEAMRT4") # Steam Deck / Steam Linux
        # Link le dossier include de FFmpeg à la target
        # ==================================
        # TODO:
        # Dès qu'ont auras la librairie pour SteamRT4, il faudra juste changer 
        # ce path pour "${CRZGAMES_LIBRARIES}/steamrt4/include/include_ffmpeg/ffmpeg"
        # ==================================
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/include_ffmpeg/ffmpeg")

        # Rajouter le répertoire de la lib FFmpeg, pour le linker ensuite
        # ==================================
        # TODO:
        # Dès qu'ont auras la librairie pour SteamRT4, il faudra juste changer 
        # ce path pour "${CRZGAMES_LIBRARIES}/steamrt4/lib/ffmpeg/${RC2D_ARCH}"
        # ==================================
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/ffmpeg/${RC2D_ARCH}")

        # Link les librairies FFmpeg
        target_link_libraries(${target_name} PUBLIC
          avcodec
          avdevice
          avfilter
          avformat
          avutil
          swresample
          swscale
        )
      else() # Linux, Raspberry PI 5 etc.
        # Link le dossier include de FFmpeg à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/include_ffmpeg/ffmpeg")

        # Rajouter le répertoire de la lib FFmpeg, pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/ffmpeg/${RC2D_ARCH}")

        # Link les librairies FFmpeg
        target_link_libraries(${target_name} PUBLIC
          avcodec
          avdevice
          avfilter
          avformat
          avutil
          swresample
          swscale
        )
      endif()
    elseif (ANDROID)
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/include_ffmpeg/ffmpeg")

      # Rajouter le répertoire de la lib FFmpeg, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/ffmpeg/${ANDROID_ABI}")

      # Link les librairies FFmpeg
      target_link_libraries(${target_name} PUBLIC
        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale
      )
    endif()
  endif()
endfunction()

# ============================================================
# rc2d_configure_rcenet
#
# Configure RCEnet pour une target donnée.
#
# Objectif :
# - Ajouter include + lib dirs selon la plateforme
# - Lier rcenet
# - Ajouter les libs système nécessaires (Windows : ws2_32, winmm)
#
# Notes :
# - RCEnet est optionnel via RC2D_NET_MODULE_ENABLED
# ============================================================
function(rc2d_configure_rcenet target_name)
  if (RC2D_NET_MODULE_ENABLED)
    if (WIN32) # Windows
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/include_rcenet")

      # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/rcenet/${RC2D_ARCH}")

      # Link les librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
        ws2_32 # Bibliothèque native à Windows (pour la librairie Enet)
        winmm # Bibliothèque native à Windows (pour la librairie Enet)
      )

    elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/include_rcenet")

      # Ajouter explicitement la lib .dylib à linker
      target_link_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/macos/lib/rcenet"
      )

      # Link la librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )

    elseif (APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS") # iOS (iPhone/iPad)
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/include_rcenet")

      # Ajouter explicitement la lib .a à linker
      target_link_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/ios/lib/rcenet/arm64-iphoneos"
      )

      # Link la librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )

    elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, SteamRT4, Raspberry PI 5 etc.
      if (RC2D_PLATFORM STREQUAL "STEAMRT4") # Steam Deck / Steam Linux
        # Link le dossier include de RCEnet à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/include/include_rcenet")

        # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/lib/rcenet/${RC2D_ARCH}")

        # Link les librairies RCEnet
        target_link_libraries(${target_name} PUBLIC
          rcenet
        )
      else() # Linux, Raspberry PI 5 etc.
        # Link le dossier include de RCEnet à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/include_rcenet")

        # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/rcenet/${RC2D_ARCH}")

        # Link les librairies RCEnet
        target_link_libraries(${target_name} PUBLIC
          rcenet
        )
      endif()
    elseif (ANDROID)
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/include_rcenet")

      # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/rcenet/${ANDROID_ABI}")

      # Link les librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )
    endif()
  endif()
endfunction()

# ============================================================
# rc2d_configure_onnxruntime
#
# Configure ONNX Runtime (inférence IA) pour une target donnée.
#
# Objectif :
# - Ajouter les headers ONNX Runtime (include)
# - Ajouter le dossier des libs (link directories)
# - Lier la librairie onnxruntime
# - Copier les libs runtime si nécessaire (Windows DLL / macOS dylib / Linux so)
#
# Notes :
# - ONNX Runtime est optionnel via RC2D_ONNX_MODULE_ENABLED
# ============================================================
function(rc2d_configure_onnxruntime target_name)
  if(RC2D_ONNX_MODULE_ENABLED)
    if(WIN32) # Windows
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/include_onnxruntime/onnxruntime")

      # Rajouter le répertoire de la lib ONNX Runtime (onnxruntime_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/onnxruntime/${RC2D_ARCH}")

      # Link les librairies ONNX Runtime
      target_link_libraries(${target_name} PUBLIC
        onnxruntime
      )

      # Copier aussi les DLLs dans le répertoire de l'exécutable
      file(GLOB_RECURSE ONNX_DLLS
        "${CRZGAMES_LIBRARIES}/windows/bin/onnxruntime/${RC2D_ARCH}/*.dll"
      )
      foreach(FILE IN LISTS ONNX_DLLS)
        get_filename_component(FILE_NAME "${FILE}" NAME)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${FILE}"
                  "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
        )
      endforeach()
    elseif (ANDROID) # Android
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/include_onnxruntime/onnxruntime")

      # Rajouter le répertoire de la lib ONNX Runtime, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/onnxruntime/${ANDROID_ABI}")

      # Link les librairies ONNX Runtime
      target_link_libraries(${target_name} PUBLIC
        onnxruntime
      )
    elseif (APPLE) # Apple (macOS, iOS)
      if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
        # Link le dossier include de ONNX Runtime à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/include_onnxruntime/onnxruntime")

        # Ajouter explicitement la lib .dylib à linker
        target_link_directories(${target_name} PUBLIC
          "${CRZGAMES_LIBRARIES}/macos/lib/onnxruntime"
        )

        # Link la librairies ONNX Runtime
        target_link_libraries(${target_name} PUBLIC
          onnxruntime
          "-framework CoreML"
        )

        # Copier aussi les libraries dynamique dans le répertoire de l'exécutable
        file(GLOB_RECURSE ONNX_DYLIBS
          "${CRZGAMES_LIBRARIES}/macos/lib/onnxruntime/*.dylib"
        )
        foreach(FILE IN LISTS ONNX_DYLIBS)
          get_filename_component(FILE_NAME "${FILE}" NAME)
          add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${FILE}"
                    "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
          )
        endforeach()
      elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS") # iOS (iPhone/iPad)
        # Link le dossier include de ONNX Runtime à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/include_onnxruntime/onnxruntime")

        # Ajouter explicitement la lib .a à linker
        target_link_directories(${target_name} PUBLIC
          "${CRZGAMES_LIBRARIES}/ios/lib/onnxruntime/arm64-iphoneos"
        )

        # Link la librairies ONNX Runtime
        target_link_libraries(${target_name} PUBLIC
          onnxruntime
          "-framework CoreML"
        )
      endif()
    elseif (RC2D_PLATFORM STREQUAL "STEAMRT4") # Steam Deck / Steam Linux
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/include/include_onnxruntime/onnxruntime")

      # Rajouter le répertoire de la lib ONNX Runtime (onnxruntime_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/lib/onnxruntime/${RC2D_ARCH}")

      # Link les librairies ONNX Runtime
      target_link_libraries(${target_name} PUBLIC
        onnxruntime
      )

      # Copier aussi les libraries dynamique dans le répertoire de l'exécutable
      file(GLOB_RECURSE ONNX_SO
        "${CRZGAMES_LIBRARIES}/steamrt4/lib/onnxruntime/${RC2D_ARCH}/*.so"
        "${CRZGAMES_LIBRARIES}/steamrt4/lib/onnxruntime/${RC2D_ARCH}/*.so.*"
      )
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND cp -a ${ONNX_SO} "$<TARGET_FILE_DIR:${target_name}>/"
        COMMENT "Copie des bibliothèques ONNX Runtime avec conservation des liens symboliques"
        VERBATIM
      )
    elseif (UNIX AND NOT APPLE AND NOT ANDROID AND NOT (RC2D_PLATFORM STREQUAL "STEAMRT4")) # Linux, Raspberry PI 5 etc.
        # Link le dossier include de ONNX Runtime à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/include_onnxruntime/onnxruntime")

        # Rajouter le répertoire de la lib ONNX Runtime, pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/onnxruntime/${RC2D_ARCH}")

        # Link les librairies ONNX Runtime
        target_link_libraries(${target_name} PUBLIC
          onnxruntime
        )

        # Copier aussi les libraries dynamique dans le répertoire de l'exécutable
        file(GLOB_RECURSE ONNX_SO
          "${CRZGAMES_LIBRARIES}/linux/lib/onnxruntime/${RC2D_ARCH}/*.so"
          "${CRZGAMES_LIBRARIES}/linux/lib/onnxruntime/${RC2D_ARCH}/*.so.*"
        )
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND cp -a ${ONNX_SO} "$<TARGET_FILE_DIR:${target_name}>/"
          COMMENT "Copie des bibliothèques ONNX Runtime avec conservation des liens symboliques"
          VERBATIM
        )
    endif()
  endif()
endfunction()

# ============================================================
# rc2d_configure_shadercross
#
# Configure SDL3_shadercross pour une target donnée.
#
# Objectifs :
# 1) Toujours : copier l’outil CLI "shadercross" + ses dépendances
#    dans examples/shaders/tools (compilation offline possible)
#
# 2) Optionnel (RC2D_GPU_SHADER_HOT_RELOAD_ENABLED=ON) :
#    - inclure les headers
#    - lier SDL3_shadercross
#    - copier les libs runtime à côté de la target
# ============================================================
function(rc2d_configure_shadercross target_name)
  if (APPLE)
    # Repertoire cible des binaires de shadercross pour "examples/shaders/tools"
    set(SHADERCROSS_BINARY_DIR "${CRZGAMES_LIBRARIES}/macos/bin/SDL3_shadercross")

    # Répertoire cible pour les outils de shaders, pour la compilation hors ligne des shaders
    set(SHADER_TOOLS_DIR "${PROJECT_SOURCE_DIR}/examples/shaders/tools")

    # Crée le dossier "examples/shaders/tools" AVANT toute copie
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_TOOLS_DIR}"
    )

    # Copier le binaires de shadercross dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_BINARY
      "${SHADERCROSS_BINARY_DIR}/*"
    )
    foreach(FILE IN LISTS SHADERCROSS_BINARY)
      get_filename_component(FILE_NAME "${FILE}" NAME)
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FILE}"
                "${SHADER_TOOLS_DIR}/${FILE_NAME}"
      )
    endforeach()

    # Copier aussi les librarieis dynamique (.dylib) dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_DYLIB
      "${CRZGAMES_LIBRARIES}/macos/lib/SDL3_shadercross/*.dylib"
    )
    foreach(FILE IN LISTS SHADERCROSS_DYLIB)
      get_filename_component(FILE_NAME "${FILE}" NAME)
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FILE}"
                "${SHADER_TOOLS_DIR}/${FILE_NAME}"
      )
    endforeach()

    # Rendre shadercross exécutable (important s’il est copié depuis une archive)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND chmod +x "${SHADER_TOOLS_DIR}/shadercross"
      COMMENT "Rendre shadercross exécutable"
      VERBATIM
    )

    # Ajouter @loader_path comme RPATH a l'executable "shadercross" pour que les .dylib soient trouvées dans le même dossier
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND install_name_tool -add_rpath @loader_path "${SHADER_TOOLS_DIR}/shadercross"
      COMMENT "Ajout de @loader_path comme RPATH à shadercross"
      VERBATIM
    )

    # Si RC2D_GPU_SHADER_HOT_RELOAD_ENABLED est activé, on link la lib shadercross
    if(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # Link le dossier include de shadercross à la target
      target_include_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/macos/include/include_sdl3shadercross"
      )

      # Récupérer le dossier de la lib shadercross
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/lib/SDL3_shadercross")

      # Link la lib shadercross
      target_link_libraries(${target_name} PUBLIC
        SDL3_shadercross
      )

      # Copier aussi les DLLs dans le répertoire de l'exécutable
      file(GLOB_RECURSE SHADERCROSS_DLLS
        "${CRZGAMES_LIBRARIES}/macos/lib/SDL3_shadercross/*.dylib"
      )
      foreach(FILE IN LISTS SHADERCROSS_DLLS)
        get_filename_component(FILE_NAME "${FILE}" NAME)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${FILE}"
                  "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
        )
      endforeach()
    endif()
  elseif(WIN32) # Windows
    # Repertoire cible des dll + binaire de shadercross pour "examples/shaders/tools"
    set(SHADERCROSS_DLL_BINARY_DIR "${CRZGAMES_LIBRARIES}/windows/bin/SDL3_shadercross/${RC2D_ARCH}")

    # Répertoire cible pour les outils de shaders, pour la compilation hors ligne des shaders
    set(SHADER_TOOLS_DIR "${PROJECT_SOURCE_DIR}/examples/shaders/tools")

    # Crée le dossier "examples/shaders/tools" AVANT toute copie
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_TOOLS_DIR}"
    )

    # Copier les DLLs et binaires de shadercross dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_DLLS_BINARY
      "${SHADERCROSS_DLL_BINARY_DIR}/*.dll"
      "${SHADERCROSS_DLL_BINARY_DIR}/*.exe"
    )
    foreach(FILE IN LISTS SHADERCROSS_DLLS_BINARY)
      get_filename_component(FILE_NAME "${FILE}" NAME)
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FILE}"
                "${SHADER_TOOLS_DIR}/${FILE_NAME}"
      )
    endforeach()

    # Si RC2D_GPU_SHADER_HOT_RELOAD_ENABLED est activé, on link la lib shadercross
    if(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED)
      # Link le dossier include de shadercross à la target
      target_include_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/windows/include/include_sdl3shadercross"
      )

      # Récupérer le dossier de la lib shadercross
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/SDL3_shadercross/${RC2D_ARCH}")

      # Link la lib shadercross
      target_link_libraries(${target_name} PUBLIC
        SDL3_shadercross
      )

      # Copier aussi les DLLs dans le répertoire de l'exécutable
      file(GLOB_RECURSE SHADERCROSS_DLLS
        "${CRZGAMES_LIBRARIES}/windows/bin/SDL3_shadercross/${RC2D_ARCH}/*.dll"
      )
      foreach(FILE IN LISTS SHADERCROSS_DLLS)
        get_filename_component(FILE_NAME "${FILE}" NAME)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${FILE}"
                  "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
        )
      endforeach()
    endif()
  elseif(RC2D_PLATFORM STREQUAL "STEAMRT4") # Steam Deck / Steam Linux
    # Repertoire cible du binaire de shadercross pour "examples/shaders/tools"
    set(SHADERCROSS_BINARY_DIR "${CRZGAMES_LIBRARIES}/steamrt4/bin/SDL3_shadercross/${RC2D_ARCH}")
    
    # Repertoire cible des .so + binaire de shadercross pour "examples/shaders/tools"
    set(SHADER_TOOLS_DIR "${PROJECT_SOURCE_DIR}/examples/shaders/tools")

    # Crée le dossier "examples/shaders/tools" AVANT toute copie
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_TOOLS_DIR}"
    )

    # Copier le binaire de shadercross dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_BINARY
      "${SHADERCROSS_BINARY_DIR}/*"
    )
    foreach(FILE IN LISTS SHADERCROSS_BINARY)
      get_filename_component(FILE_NAME "${FILE}" NAME)
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FILE}"
                "${SHADER_TOOLS_DIR}/${FILE_NAME}"
      )
    endforeach()

    # Copier aussi les .so dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_SO
      "${CRZGAMES_LIBRARIES}/steamrt4/lib/SDL3_shadercross/${RC2D_ARCH}/*.so"
      "${CRZGAMES_LIBRARIES}/steamrt4/lib/SDL3_shadercross/${RC2D_ARCH}/*.so.*"
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND cp -a ${SHADERCROSS_SO} "${SHADER_TOOLS_DIR}/"
      COMMENT "Copie des bibliothèques SDL3_shadercross avec conservation des liens symboliques"
      VERBATIM
    )

    # Rendre shadercross exécutable (important s’il est copié depuis une archive)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND chmod +x "${SHADER_TOOLS_DIR}/shadercross"
      COMMENT "Rendre shadercross exécutable"
      VERBATIM
    )

    # Ajouter $ORIGIN comme RPATH a l'executable "shadercross" pour que les .so soient trouvées dans le même dossier
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "\$ORIGIN" "${SHADER_TOOLS_DIR}/shadercross"
      COMMENT "Ajout de $ORIGIN comme RPATH à shadercross"
      VERBATIM
    )

    if(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED)
      target_include_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/steamrt4/include/include_sdl3shadercross"
      )

      # Récupérer le dossier de la lib shadercross
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/lib/SDL3_shadercross/${RC2D_ARCH}")

      # Link la lib shadercross
      target_link_libraries(${target_name} PUBLIC
        SDL3_shadercross
      )

      # Copier aussi les .so dans le répertoire de l'exécutable
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND cp -a ${SHADERCROSS_SO} "$<TARGET_FILE_DIR:${target_name}>/"
        COMMENT "Copie des bibliothèques SDL3_shadercross avec conservation des liens symboliques"
        VERBATIM
      )
    endif()
  elseif(UNIX AND NOT APPLE) # Linux, Android, Raspberry PI 5 etc.
    if(ANDROID)
      # Set RC2D_ARCH à x64, puisque de manière générale, on compile un projet Android sur 
      # une machine de développement x64, et que les outils de shadercross sont compilés pour la plateforme de développement (et pas pour Android).
      # Mais on'en n'a besoin que pour la partie copie de shadercross dans "examples/shaders/tools", 
      # qui permet la compilation offline des shaders, et pas pour le lien de shadercross à la target.
      set(RC2D_ARCH "x64")
    endif()
    
    # Repertoire cible du binaire de shadercross pour "examples/shaders/tools"
    set(SHADERCROSS_BINARY_DIR "${CRZGAMES_LIBRARIES}/linux/bin/SDL3_shadercross/${RC2D_ARCH}")

    # Repertoire cible des .so + binaire de shadercross pour "examples/shaders/tools"
    set(SHADER_TOOLS_DIR "${PROJECT_SOURCE_DIR}/examples/shaders/tools")

    # Crée le dossier "examples/shaders/tools" AVANT toute copie
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_TOOLS_DIR}"
    )

    # Copier le binaire de shadercross dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_BINARY
      "${SHADERCROSS_BINARY_DIR}/*"
    )
    foreach(FILE IN LISTS SHADERCROSS_BINARY)
      get_filename_component(FILE_NAME "${FILE}" NAME)
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FILE}"
                "${SHADER_TOOLS_DIR}/${FILE_NAME}"
      )
    endforeach()

    # Copier aussi les .so dans le répertoire "examples/shaders/tools"
    file(GLOB_RECURSE SHADERCROSS_SO
      "${CRZGAMES_LIBRARIES}/linux/lib/SDL3_shadercross/${RC2D_ARCH}/*.so"
      "${CRZGAMES_LIBRARIES}/linux/lib/SDL3_shadercross/${RC2D_ARCH}/*.so.*"
    )
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND cp -a ${SHADERCROSS_SO} "${SHADER_TOOLS_DIR}/"
      COMMENT "Copie des bibliothèques SDL3_shadercross avec conservation des liens symboliques"
      VERBATIM
    )

    # Rendre shadercross exécutable (important s’il est copié depuis une archive)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND chmod +x "${SHADER_TOOLS_DIR}/shadercross"
      COMMENT "Rendre shadercross exécutable"
      VERBATIM
    )

    # Ajouter $ORIGIN comme RPATH a l'executable "shadercross" pour que les .so soient trouvées dans le même dossier
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "\$ORIGIN" "${SHADER_TOOLS_DIR}/shadercross"
      COMMENT "Ajout de $ORIGIN comme RPATH à shadercross"
      VERBATIM
    )

    if(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED AND NOT ANDROID)
      # Link le dossier include de shadercross à la target
      target_include_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/linux/include/include_sdl3shadercross"
      )

      # Récupérer le dossier de la lib shadercross
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/SDL3_shadercross/${RC2D_ARCH}")

      # Link la lib shadercross
      target_link_libraries(${target_name} PUBLIC
        SDL3_shadercross
      )

      # Copier aussi les .so dans le répertoire de l'exécutable
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND cp -a ${SHADERCROSS_SO} "$<TARGET_FILE_DIR:${target_name}>/"
        COMMENT "Copie des bibliothèques SDL3_shadercross avec conservation des liens symboliques"
        VERBATIM
      )
    endif()
  endif()
endfunction()

# Configuration de OpenSSL
function(rc2d_configure_openssl target_name)
  if(RC2D_DATA_MODULE_ENABLED)
    if(APPLE) # Apple (macOS, iOS)
      if(CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
        # Link le dossier include de OpenSSL à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/include_openssl")

        # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/lib/OpenSSL")

        # Link les librairies OpenSSL
        target_link_libraries(${target_name} PUBLIC
          ssl    # OpenSSL
          crypto # OpenSSL
        )
      elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS") # iOS (iPhone/iPad)
        # Link le dossier include de OpenSSL à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/include_openssl")

        # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/lib/OpenSSL/arm64-iphoneos")

        # Link les librairies OpenSSL
        target_link_libraries(${target_name} PUBLIC
          ssl    # OpenSSL
          crypto # OpenSSL
        )
      endif()
    elseif (ANDROID) # Android
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/include_openssl")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/openssl/${ANDROID_ABI}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        ssl    # OpenSSL
        crypto # OpenSSL
      )
    elseif(WIN32) # Windows
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/include_openssl")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/OpenSSL/${RC2D_ARCH}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        libssl    # OpenSSL
        libcrypto # OpenSSL
        Crypt32   # OpenSSL CryptoAPI (Librarie native de Windows, obligatoire)
        ws2_32    # OpenSSL Winsock2  (Librarie native de Windows, obligatoire)
      )
    elseif(RC2D_PLATFORM STREQUAL "STEAMRT4") # Steam Deck / Steam Linux
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/include/include_openssl")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamrt4/lib/OpenSSL/${RC2D_ARCH}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        ssl    # OpenSSL
        crypto # OpenSSL
      )
    elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Raspberry PI 5 etc.
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/include_openssl")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/OpenSSL/${RC2D_ARCH}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        ssl    # OpenSSL
        crypto # OpenSSL
      )
    endif()
  endif()
endfunction()

# Fonction pour linker le sdk steamworks
function(rc2d_configure_steamworks target_name)
  if(RC2D_STEAMWORKS_SDK_ENABLED)
    # Link le dossier include du SDK Steamworks à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamworks-sdk/include")

    if(WIN32) # Windows
      # Rajouter le répertoire de la lib Steamworks, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/steamworks-sdk/lib/windows/${RC2D_ARCH}")

      # Link la librairie Steamworks
      target_link_libraries(${target_name} PUBLIC
        steam_api64 # Pour x64
      )

      # Copier aussi les DLLs dans le répertoire de l'exécutable
      file(GLOB_RECURSE STEAMWORKS_DLLS
        "${CRZGAMES_LIBRARIES}/steamworks-sdk/bin/windows/${RC2D_ARCH}/*.dll"
      )

      foreach(FILE IN LISTS STEAMWORKS_DLLS)
        get_filename_component(FILE_NAME "${FILE}" NAME)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${FILE}"
                  "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
        )
      endforeach()
    endif()
  endif()
endfunction()

# Fonction pour le SDK EOS (Epic Online Services)
function(rc2d_configure_eos_sdk target_name)
  if(RC2D_EOS_SDK_ENABLED)
    # Link le dossier include du SDK EOS à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/eos-sdk/include")

    if(WIN32) # Windows
      # Rajouter le répertoire de la lib EOS, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/eos-sdk/lib/windows/${RC2D_ARCH}")

      # Link la librairie EOS
      target_link_libraries(${target_name} PUBLIC
        EOSSDK-Win64-Shipping # Pour x64
      )

      # Copier aussi les DLLs dans le répertoire de l'exécutable
      file(GLOB_RECURSE EOS_DLLS
        "${CRZGAMES_LIBRARIES}/eos-sdk/bin/windows/${RC2D_ARCH}/*.dll"
      )

      foreach(FILE IN LISTS EOS_DLLS)
        get_filename_component(FILE_NAME "${FILE}" NAME)
        add_custom_command(TARGET ${target_name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${FILE}"
                  "$<TARGET_FILE_DIR:${target_name}>/${FILE_NAME}"
        )
      endforeach()
    endif()
  endif()
endfunction()

# ============================================================
# rc2d_link_linux_system_libraries
#
# Lie les bibliothèques système nécessaires au runtime Linux.
#
# Objectif :
# - Ajouter des dépendances POSIX classiques qui manquent souvent
#   si certaines libs sont linkées "optionnellement".
#
# Détails :
# - m       : libm (fonctions mathématiques)
# - pthread : threads POSIX
# - dl      : dlopen / dlsym (chargement dynamique)
#
# Pourquoi "-Wl,--no-as-needed" ?
# - Sur certains linkers, une lib peut être ignorée si aucun symbole
#   n'est vu comme utilisé à ce stade du link.
# - "--no-as-needed" force le linker à garder ces libs.
# - Puis "--as-needed" remet le comportement normal ensuite.
# ============================================================
function(rc2d_link_linux_system_libraries target_name)
  if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(${target_name} PUBLIC
      "-Wl,--no-as-needed"
      m
      pthread
      dl
      "-Wl,--as-needed"
    )
  endif()
endfunction()

# ============================================================
# rc2d_include_headers
#
# Ajoute les include paths publics de RC2D.
#
# Objectif :
# - Rendre accessibles les headers publics de RC2D (include/)
# - Inclure aussi include/external si tu exposes des headers tiers
#   (ou des wrappers) à l’utilisateur.
#
# PUBLIC :
# - Toute target qui link RC2D récupère automatiquement ces includes.
# ============================================================
function(rc2d_include_headers target_name)
  target_include_directories(${target_name} PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/external"
  )
endfunction()

# ============================================================
# rc2d_link_apple_frameworks
#
# Ajoute les frameworks Apple nécessaires.
#
# Pourquoi :
# - Quand SDL / SDL_image / SDL_mixer / SDL_ttf sont statiques,
#   ils tirent des symboles de frameworks Apple qui ne sont plus
#   automatiquement résolus.
# ============================================================
function(rc2d_link_apple_frameworks target_name)
  # SDL_Image bug: https://github.com/libsdl-org/SDL_image/issues/506
  # + Notamment pour iOS les libs ci-dessous sont nécessaires
  # vu qu'on utilise les libs static de SDL3, SDL3_image, SDL3_mixer et SDL3_ttf
  if (APPLE AND NOT BUILD_SHARED_LIBS)
    target_link_libraries(${target_name} PUBLIC 
      "-framework ImageIO"
      "-framework CoreServices"
      "-framework CoreText"
      "-framework CoreGraphics"
      "-framework CoreFoundation"
      "-framework AVFoundation"
      "-framework AudioToolbox"
      "-framework CoreBluetooth" 
      "-framework CoreMotion"
      "-framework CoreGraphics"
      "-framework CoreHaptics"
      "-framework GameController"
      "-framework Metal"
      "-framework UIKit"
      "-framework Foundation"
      "-framework OpenGLES"
      "-framework QuartzCore"
    )
  endif()
endfunction()

# ============================================================
# rc2d_target_compile_definitions
#
# Centralise toutes les compile definitions RC2D_* appliquées
# à une target donnée.
#
# Objectif :
# - Exposer dans le code C/C++ des flags cohérents :
#   • RC2D_EOS_SDK_ENABLED (0/1)
#   • RC2D_STEAMWORKS_SDK_ENABLED (0/1)
#   • RC2D_BUILDING_SHARED (0/1)
#   • RC2D_VIDEO_MODULE_ENABLED (0/1)
#   • RC2D_GPU_SHADER_HOT_RELOAD_ENABLED (0/1)
#   • RC2D_ASSERT_LEVEL (0..3)
#   • RC2D_DATA_MODULE_ENABLED (0/1)
#   • RC2D_NET_MODULE_ENABLED (0/1)
#   • RC2D_ONNX_MODULE_ENABLED (0/1)
#   • RC2D_MEMORY_DEBUG_ENABLED (0/1)
# ============================================================
function(rc2d_target_compile_definitions target_name)
  if(RC2D_EOS_SDK_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_EOS_SDK_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_EOS_SDK_ENABLED=0)
  endif()

  if(RC2D_STEAMWORKS_SDK_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_STEAMWORKS_SDK_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_STEAMWORKS_SDK_ENABLED=0)
  endif()

  if(RC2D_BUILDING_SHARED)
    target_compile_definitions(${target_name} PUBLIC RC2D_BUILDING_SHARED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_BUILDING_SHARED=0)
  endif()

  if(RC2D_VIDEO_MODULE_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_VIDEO_MODULE_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_VIDEO_MODULE_ENABLED=0)
  endif()

  if(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_GPU_SHADER_HOT_RELOAD_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_GPU_SHADER_HOT_RELOAD_ENABLED=0)
  endif()

  target_compile_definitions(${target_name} PUBLIC RC2D_ASSERT_LEVEL=${RC2D_ASSERT_LEVEL})

  if(RC2D_DATA_MODULE_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_DATA_MODULE_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_DATA_MODULE_ENABLED=0)
  endif()

  if(RC2D_NET_MODULE_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_NET_MODULE_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_NET_MODULE_ENABLED=0)
  endif()

  if(RC2D_ONNX_MODULE_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_ONNX_MODULE_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_ONNX_MODULE_ENABLED=0)
  endif()

  if (RC2D_MEMORY_DEBUG_ENABLED)
    target_compile_definitions(${target_name} PUBLIC RC2D_MEMORY_DEBUG_ENABLED=1)
  else()
    target_compile_definitions(${target_name} PUBLIC RC2D_MEMORY_DEBUG_ENABLED=0)
  endif()
endfunction()

# ============================================================
# Sources & Headers (RC2D)
# ============================================================
# Récupère automatiquement tous les fichiers sources C du moteur,
# puis ajoute éventuellement des sources C++ si certaines intégrations
# sont activées (notamment : Steamworks SDK).
# ============================================================

# Sources du projet RC2D (moteur core en C)
file(GLOB_RECURSE SOURCES
  "${PROJECT_SOURCE_DIR}/src/*.c"
)
# Ajouter le module Steamworks si activé :
# - Steamworks SDK est en C++ → on compile RC2D_steamworks.cpp uniquement si nécessaire
if(RC2D_STEAMWORKS_SDK_ENABLED)
  list(APPEND SOURCES src/RC2D/RC2D_steamworks.cpp)
endif()

# Headers publics du projet RC2D (API exposée)
file(GLOB_RECURSE HEADERS
  "${PROJECT_SOURCE_DIR}/include/*.h"
)

# ============================================================
# Library Target (static / shared)
# ============================================================
# Crée la target principale du moteur RC2D.
#
# Deux modes :
# - RC2D_BUILDING_SHARED=ON  → lib partagée (.dll/.so/.dylib)
# - RC2D_BUILDING_SHARED=OFF → lib statique  (.lib/.a)
#
# Note :
# - En mode statique, on change le nom de sortie en "*_static"
#   pour éviter les collisions de nom avec la version partagée/dynamique.
# ============================================================
if(RC2D_BUILDING_SHARED)
  add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
else()
  add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
  # Rajouter le suffixe "*_static" au nom de la bibliothèque statique, pour éviter les conflits
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_static")
endif()

# ============================================================
# Compile Definitions (RC2D_* flags)
# ============================================================
# Applique à la target RC2D toutes les compile definitions
# (modules activés/désactivés, niveau d’assertion, etc.)
# ============================================================
rc2d_target_compile_definitions(${PROJECT_NAME})

# ============================================================
# Include Directories (Public API)
# ============================================================
# Expose les headers publics (include/) et les éventuels headers externes
# (include/external) à tous les projets qui linkent RC2D.
# ============================================================
rc2d_include_headers(${PROJECT_NAME})

# ============================================================
# Common Dependencies (SDL3, SDL3_image, SDL3_mixer, SDL3_ttf)
# ============================================================
# Dépendances communes à toutes les plateformes :
# - SDL3
# - SDL3_image / SDL3_ttf / SDL3_mixer
#
# PUBLIC :
# - Toute target qui link RC2D récupère aussi ces dépendances,
#   ce qui est généralement souhaité pour une lib "engine".
# ============================================================
target_link_libraries(${PROJECT_NAME} PUBLIC
  SDL3_mixer::SDL3_mixer
  SDL3_image::SDL3_image
  SDL3_ttf::SDL3_ttf
  SDL3::SDL3
)

# FFmpeg (module RC2D_video, optionnel)
rc2d_configure_ffmpeg(${PROJECT_NAME})

# ONNX Runtime (module RC2D_onnx, optionnel)
rc2d_configure_onnxruntime(${PROJECT_NAME})

# OpenSSL (module RC2D_data, optionnel)
rc2d_configure_openssl(${PROJECT_NAME})

# SDL3_shadercross (outil shadercross CLI + hot reload optionnel)
rc2d_configure_shadercross(${PROJECT_NAME})

# Libs système Linux (m/pthread/dl) pour runtime POSIX
rc2d_link_linux_system_libraries(${PROJECT_NAME})

# RCEnet / ENet (module RC2D_net, optionnel)
rc2d_configure_rcenet(${PROJECT_NAME})

# Frameworks Apple nécessaires si build statique (SDL & co)
rc2d_link_apple_frameworks(${PROJECT_NAME})

# cJSON (parsing JSON léger côté runtime)
rc2d_configure_cjson(${PROJECT_NAME})

# Steamworks SDK (module RC2D_steamworks, optionnel)
rc2d_configure_steamworks(${PROJECT_NAME})

# Epic Online Services SDK (module RC2D_eos, optionnel)
rc2d_configure_eos_sdk(${PROJECT_NAME})










# ============================================================
#
# Example Target (exécutable de démonstration)
#
# ============================================================
if(RC2D_BUILD_EXAMPLES)
  # ============================================================
  # Nom de la target de l’exécutable d’exemple
  # ============================================================
  if(ANDROID)
    set(RC2D_EXAMPLE_TARGET_NAME main)
  else()
    set(RC2D_EXAMPLE_TARGET_NAME rc2d_example)
  endif()

  # ============================================================
  # Application Metadata
  #
  # Définit les métadonnées de l'application (version, nom de l'entreprise, description, droits d'auteur, etc.)
  # en utilisant des variables d'environnement si elles sont définies, ou des valeurs par défaut sinon.
  #
  # C'est utiliser pour générer dynamiquement le fichier de ressources Windows (.rc) ou les Info.plist pour Apple, 
  # afin d'inclure ces métadonnées dans l'exécutable final.
  # ============================================================
  if(DEFINED ENV{APP_VERSION})
    set(APP_VERSION "$ENV{APP_VERSION}" CACHE STRING "Version de l'application")
  else()
    set(APP_VERSION "1.0.0" CACHE STRING "Version de l'application")
  endif()

  # Découper la version en parties
  string(REPLACE "." ";" VERSION_LIST ${APP_VERSION})
  list(GET VERSION_LIST 0 APP_VERSION_MAJOR)
  list(GET VERSION_LIST 1 APP_VERSION_MINOR)
  list(GET VERSION_LIST 2 APP_VERSION_PATCH)

  if(DEFINED ENV{APP_COMPANY_NAME})
    set(APP_COMPANY_NAME "$ENV{APP_COMPANY_NAME}" CACHE STRING "Nom de l'entreprise")
  else()
    set(APP_COMPANY_NAME "CrzGames" CACHE STRING "Nom de l'entreprise")
  endif()

  if(DEFINED ENV{APP_GAME_DESCRIPTION})
    set(APP_GAME_DESCRIPTION "$ENV{APP_GAME_DESCRIPTION}" CACHE STRING "Game description")
  else()
    set(APP_GAME_DESCRIPTION "MyGame - MMORPG" CACHE STRING "Game description")
  endif()

  if(DEFINED ENV{APP_LEGAL_COPYRIGHT})
    set(APP_LEGAL_COPYRIGHT "$ENV{APP_LEGAL_COPYRIGHT}" CACHE STRING "Droits d'auteur")
  else()
    set(APP_LEGAL_COPYRIGHT "Copyright 2025 CrzGames" CACHE STRING "Droits d'auteur")
  endif()

  if(DEFINED ENV{APP_IOSMACOS_BUILD_VERSION})
    set(APP_IOSMACOS_BUILD_VERSION "$ENV{APP_IOSMACOS_BUILD_VERSION}" CACHE STRING "Build version")
  else()
    set(APP_IOSMACOS_BUILD_VERSION "1" CACHE STRING "Build version")
  endif()

  if(DEFINED ENV{APP_IOSMACOS_IDENTIFIER})
    set(APP_IOSMACOS_IDENTIFIER "$ENV{APP_IOSMACOS_IDENTIFIER}" CACHE STRING "Identifiant de l'application")
  else()
    set(APP_IOSMACOS_IDENTIFIER "com.crzgames.testexe" CACHE STRING "Identifiant de l'application")
  endif()

  # ============================================================
  # Fichiers source de l’exemple
  #
  # Récupère automatiquement tous les fichiers sources .cpp de manière récursive 
  # dans "examples/src" pour les compiler dans la target de l’exemple.
  # ============================================================
  file(GLOB_RECURSE RC2D_EXAMPLE_SOURCES
    "${PROJECT_SOURCE_DIR}/examples/src/*.cpp"
  )

  # ============================================================
  # Fichiers d’en-tête (headers) de l’exemple
  #
  # Récupère automatiquement tous les fichiers d’en-tête .h de manière récursive 
  # dans "examples/include" pour les inclure dans la target de l’exemple.
  # ============================================================
  file(GLOB_RECURSE RC2D_EXAMPLE_HEADERS
    "${PROJECT_SOURCE_DIR}/examples/include/*.h"
  )

  # ============================================================
  # Création de la target exécutable pour l’exemple
  #
  # Pour une application Android, il faut impérativement que la target soit une bibliothèque partagée (SHARED) 
  # et que son nom soit "main" pour que le système Android puisse la reconnaître comme point d'entrée de l'application.
  # 
  # Pour les autres plateformes, on peut utiliser le nom de target "rc2d_example" et créer une target d'exécutable classique.
  #
  # ============================================================
  if(ANDROID)
    add_library(${RC2D_EXAMPLE_TARGET_NAME} SHARED 
      ${RC2D_EXAMPLE_SOURCES} 
      ${RC2D_EXAMPLE_HEADERS}
    )
  else()
    add_executable(${RC2D_EXAMPLE_TARGET_NAME} 
      ${RC2D_EXAMPLE_SOURCES} 
      ${RC2D_EXAMPLE_HEADERS}
    )
  endif()

  # ============================================================
  # Include directories pour l’exemple
  # 
  # Ajoute le dossier "examples/include" aux include paths privés de la target de l’exemple.
  # ============================================================
  target_include_directories(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/examples/include"
  )

  # ============================================================
  # Linker l’exemple avec la bibliothèque RC2D
  #
  # Lie la target de l’exemple à la bibliothèque RC2D pour pouvoir utiliser le moteur dans l’exemple.
  # ============================================================
  target_link_libraries(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
    ${PROJECT_NAME} # RC2D
  )

  # ============================================================
  # Configurations spécifiques à chaque plateforme pour l’exécutable de l’exemple
  # ============================================================
  if (WIN32)
    # Définir l'executable comme une application Windows (pour éviter l'affichage de la console quand on ouvre l'executable) en mode Release
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES 
      WIN32_EXECUTABLE $<CONFIG:Release>
    )

    # Pour MSVC spécifiquement :
    if(MSVC)
      # ------------------------------------------------------------
      # Définition du chemin du fichier .rc généré
      # ------------------------------------------------------------
      # CMAKE_CURRENT_BINARY_DIR :
      #   → Répertoire de build courant (dossier généré par CMake).
      #   → On évite volontairement d’écrire dans le source tree pour :
      #       - ne pas polluer le repo Git
      #       - éviter les conflits
      #       - séparer clairement sources et artefacts générés
      #
      # resource.rc sera généré à partir de resource.rc.in
      #
      set(GENERATED_RC "${CMAKE_CURRENT_BINARY_DIR}/resource.rc")

      # ------------------------------------------------------------
      # Génération du fichier resource.rc à partir du template .rc.in
      # ------------------------------------------------------------
      # configure_file :
      #   → Remplace les variables CMake (@VAR@) dans le fichier .in
      #   → Génère un fichier concret utilisé par le compilateur
      #
      # @ONLY :
      #   → Remplace uniquement les variables au format @VAR@
      #   → Évite toute substitution accidentelle du style ${VAR}
      #
      # Exemple :
      #   resource.rc.in contient :
      #     VALUE "FileVersion", "@APP_VERSION@"
      #
      #   CMake remplace @APP_VERSION@ par la valeur définie dans le CMakeLists
      #
      configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/resource.rc.in"
        "${GENERATED_RC}"
        @ONLY
      )

      # ------------------------------------------------------------
      # Indique explicitement à CMake que ce fichier est un fichier
      # de ressources Windows (.rc)
      # ------------------------------------------------------------
      # LANGUAGE RC :
      #   → Force CMake à utiliser le compilateur de ressources Windows (rc.exe)
      #   → Important surtout si le fichier est généré
      #
      # Cela permet :
      #   - d’intégrer les métadonnées (version, société, copyright)
      #   - d’intégrer l’icône de l’exécutable
      #   - d’inclure les informations visibles dans les propriétés Windows
      #
      set_source_files_properties("${GENERATED_RC}" PROPERTIES LANGUAGE RC)

      # Et enfin, on ajoute ce fichier de ressources à la target de l’exemple pour qu’il soit compilé et lié à l’exécutable final.
      target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE "${GENERATED_RC}")

      # Définir le projet de démarrage dans Visual Studio pour que l’exécutable de l’exemple soit celui qui se lance par défaut quand on appuie sur "Play" (F5)
      set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${RC2D_EXAMPLE_TARGET_NAME})

      # Définir le répertoire de travail pour le débogueur dans Visual Studio
      # Cela permet de lancer l'exécutable avec le bon répertoire de travail (working directory)
      set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>"
      )
    endif()
  elseif (APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Ajouter le fichier d'icône de l'application dans les ressources de la target de l’exemple
    target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/icons/macos/app-icon.icns"
    )
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/icons/macos/app-icon.icns" PROPERTIES 
      MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Ajouter les assets génériques (images, sons, etc.) dans les ressources du bundle pour qu'ils soient accessibles à l'exécution
    target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
      "${PROJECT_SOURCE_DIR}/examples/assets"
    )
    set_source_files_properties("${PROJECT_SOURCE_DIR}/examples/assets" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Configurez le bundle (.app)
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME ${RC2D_EXAMPLE_TARGET_NAME} # Nom de l'application (à modifier pour chaque application)
      MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IOSMACOS_IDENTIFIER} # L'identifiant de votre application (via identifiers sur le compte apple developer), à modifier pour chaque application
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION} # Version utilisateur lisible (à modifier pour chaque application)
      MACOSX_BUNDLE_BUNDLE_VERSION ${APP_IOSMACOS_BUILD_VERSION} # Version de build unique (à modifier pour chaque application)
      MACOSX_BUNDLE_COPYRIGHT ${APP_LEGAL_COPYRIGHT} # Droits d'auteur (à modifier pour chaque application)
      MACOSX_BUNDLE_ICON_FILE "app-icon.icns"
      MACOSX_BUNDLE_INFO_STRING ${APP_GAME_DESCRIPTION} # Description de l'application (à modifier pour chaque application)
    )

    # --- Code signing (optionnel) ---
    if(RC2D_BUILD_EXAMPLES_APPLE_CODE_SIGNING)
      # SIGNATURE ACTIVÉE
      set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "U3D28WJ8DV" # Remplacez par votre Team ID Apple
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application: Corentin Recanzone (U3D28WJ8DV)" # Remplacez par votre identité de signature
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual" # Style de signature (Manual ou Automatic)
        XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--options=runtime --strict --deep --force --timestamp --verbose" # Options supplémentaires pour la signature
      )
    endif()

    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      # ---------------------------------------------------------------------------
      # macOS — RPATH pour les dépendances embarquées dans le bundle (.app)
      # Pour que les .dylib embarquées soient trouvées à l’exécution, il faut configurer le RPATH
      # pour indiquer à dyld où chercher ces bibliothèques dynamiques.
      # ---------------------------------------------------------------------------
      #
      # Contexte :
      # - Sur macOS, l’exécutable d’un bundle est ici :
      #     MyApp.app/Contents/MacOS/MyApp
      # - Les bibliothèques dynamiques que l’on embarque (SDL3, SDL3_image, SDL3_ttf,
      #   SDL3_mixer, onnxruntime, SDL3_shadercross, etc.) doivent être copiées dans :
      #     MyApp.app/Contents/Frameworks/
      #
      # Objectif :
      # - Faire en sorte que dyld (le loader macOS) trouve automatiquement ces .dylib
      #   à l’exécution, sans dépendre d’un chemin absolu de la machine de build,
      #   ni de libs installées sur le système.
      #
      # ---------------------------------------------------------------------------

      # INSTALL_RPATH :
      # - Ajoute un chemin de recherche "runtime" dans l’exécutable.
      # - "@executable_path" = dossier contenant l’exécutable du bundle :
      #     .../MyApp.app/Contents/MacOS
      # - Donc "@executable_path/../Frameworks" pointe vers :
      #     .../MyApp.app/Contents/Frameworks
      # - C’est la convention standard pour résoudre les .dylib embarquées.
      INSTALL_RPATH "@executable_path/../Frameworks"

      # BUILD_WITH_INSTALL_RPATH :
      # - Applique ce RPATH dès la phase de build, pas uniquement après un "cmake --install".
      # - Utile car on exécute généralement l’app directement depuis le dossier de build
      #   (et pas via une étape d’installation).
      BUILD_WITH_INSTALL_RPATH TRUE

      # MACOSX_RPATH :
      # - Active le mécanisme RPATH sur macOS (utilisation de LC_RPATH / @rpath).
      # - Requis/attendu pour que "@rpath" et les chemins ci-dessus soient pris en compte.
      MACOSX_RPATH TRUE
    )

    # Ajoute une étape de post-build pour copier les fichiers nécessaires dans le bundle .app et modifier le fichier Info.plist
    add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD      
      # Active le support des périphériques d'entrée indirects (souris, trackpad, clavier, manettes)
      COMMAND /usr/libexec/PlistBuddy -c "Add :UIApplicationSupportsIndirectInputEvents bool true" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${RC2D_EXAMPLE_TARGET_NAME}.app/Contents/Info.plist"
      
      # Description obligatoire pour l'utilisation du Bluetooth (CoreBluetooth)
      COMMAND /usr/libexec/PlistBuddy -c "Add :NSBluetoothAlwaysUsageDescription string 'MyApp souhaite rester connecté aux contrôleurs de jeu et aux manettes de jeu Bluetooth à proximité, même lorsque vous n'utilisez pas l'application.'" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${RC2D_EXAMPLE_TARGET_NAME}.app/Contents/Info.plist"
      
      # Ajoute les clés pour autoriser le High DPI
      COMMAND /usr/libexec/PlistBuddy -c "Add :NSHighResolutionCapable bool true" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${RC2D_EXAMPLE_TARGET_NAME}.app/Contents/Info.plist"

      # Ajoute la clé ITSAppUsesNonExemptEncryption pour le chiffrement
      COMMAND /usr/libexec/PlistBuddy -c "Add :ITSAppUsesNonExemptEncryption bool false" "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${RC2D_EXAMPLE_TARGET_NAME}.app/Contents/Info.plist"
      COMMENT "Adding keys to Info.plist"
    )
  elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # Ressource : Storyboard de lancement et Asset Catalog pour les icônes (obligatoire sur iOS)
    set(IOS_ASSETCATALOG "${CMAKE_CURRENT_SOURCE_DIR}/icons/ios/Assets.xcassets")
    set(IOS_LAUNCH_STORYBOARD "${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/LaunchScreen.storyboard")

    # Ajout à la target des ressources spécifiques à iOS (Asset Catalog et Storyboard de lancement)
    target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
      "${IOS_ASSETCATALOG}"
      "${IOS_LAUNCH_STORYBOARD}"
    )

    # Configurez les propriétés des fichiers de ressources pour qu'ils soient correctement traités par Xcode
    set_source_files_properties("${IOS_ASSETCATALOG}" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
      XCODE_EXPLICIT_FILE_TYPE "folder.assetcatalog"
    )
    set_source_files_properties("${IOS_LAUNCH_STORYBOARD}" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
      XCODE_EXPLICIT_FILE_TYPE "file.storyboard"
    )

    # Ajouter les assets génériques (images, sons, etc.) dans les ressources du bundle pour qu'ils soient accessibles à l'exécution
    target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
      "${PROJECT_SOURCE_DIR}/examples/assets"
    )
    set_source_files_properties("${PROJECT_SOURCE_DIR}/examples/assets" PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Configurez le bundle (.app)
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME ${RC2D_EXAMPLE_TARGET_NAME} # Nom de l'application (à modifier pour chaque application)
      MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IOSMACOS_IDENTIFIER} # L'identifiant de votre application (via identifiers sur le compte apple developer) à modifier pour chaque application
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION} # Version utilisateur lisible (à modifier pour chaque application)
      MACOSX_BUNDLE_BUNDLE_VERSION ${APP_IOSMACOS_BUILD_VERSION} # Version de build unique (à modifier pour chaque application)
      MACOSX_BUNDLE_COPYRIGHT ${APP_LEGAL_COPYRIGHT} # Droits d'auteur
      MACOSX_BUNDLE_INFO_STRING ${APP_GAME_DESCRIPTION} # Description de l'application (à modifier pour chaque application)
      XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon" # Nom de l'icône de l'application
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # 1 pour iPhone, 2 pour iPad
    )

    # --- Code signing (optionnel) ---
    if(RC2D_BUILD_EXAMPLES_APPLE_CODE_SIGNING)
      set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "U3D28WJ8DV" # Remplacez par votre Team ID Apple
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development: Corentin Recanzone (2X96T6AM4K)" # Remplacez par votre identité de signature
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual" # Style de signature (Manual ou Automatic)
        XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --strict --force --timestamp --verbose" # Options supplémentaires pour la signature
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "TestExe" # Remplacez par le nom de votre profil de provisionnement
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE "d5115451-c438-4aa6-9c68-dee8a87a36e8" # Remplacez par l'UUID de votre profil de provisionnement
      )
    endif()

    # Ajoute une étape de post-build pour ajouter des clés au fichier Info.plist
    add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
      # Active le support des périphériques d'entrée indirects (souris, trackpad, clavier, manettes)
      COMMAND /usr/libexec/PlistBuddy -c "Add :UIApplicationSupportsIndirectInputEvents bool true" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding keys mouse, trackpad, keyboard, game controllers support to Info.plist"

      # Description obligatoire pour l'utilisation du Bluetooth (CoreBluetooth)
      COMMAND /usr/libexec/PlistBuddy -c "Add :NSBluetoothAlwaysUsageDescription string 'MyApp souhaite rester connecté aux contrôleurs de jeu et aux manettes de jeu Bluetooth à proximité, même lorsque vous n'utilisez pas l'application.'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding keys for Bluetooth usage description to Info.plist"

      # Ajoute les clés pour autoriser le High DPI
      COMMAND /usr/libexec/PlistBuddy -c "Add :NSHighResolutionCapable bool true" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding key High DPI to Info.plist"

      # Ajoute les clés pour autoriser les orientations supportées
      COMMAND /usr/libexec/PlistBuddy -c "Add :UISupportedInterfaceOrientations array" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMAND /usr/libexec/PlistBuddy -c "Add :UISupportedInterfaceOrientations:0 string 'UIInterfaceOrientationPortrait'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMAND /usr/libexec/PlistBuddy -c "Add :UISupportedInterfaceOrientations:1 string 'UIInterfaceOrientationPortraitUpsideDown'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMAND /usr/libexec/PlistBuddy -c "Add :UISupportedInterfaceOrientations:2 string 'UIInterfaceOrientationLandscapeLeft'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMAND /usr/libexec/PlistBuddy -c "Add :UISupportedInterfaceOrientations:3 string 'UIInterfaceOrientationLandscapeRight'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding supported interface orientations to Info.plist"

      # Ajoute la clé ITSAppUsesNonExemptEncryption pour le chiffrement
      COMMAND /usr/libexec/PlistBuddy -c "Add :ITSAppUsesNonExemptEncryption bool false" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding ITSAppUsesNonExemptEncryption key to Info.plist"

      # Ajoute la clé UILaunchStoryboardName pour le storyboard de lancement
      COMMAND /usr/libexec/PlistBuddy -c "Add :UILaunchStoryboardName string 'LaunchScreen'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding launch storyboard name to Info.plist"
    )
  elseif(UNIX AND NOT APPLE AND NOT ANDROID AND NOT (RC2D_PLATFORM STREQUAL "STEAMRT4"))
    # Pour l'executable "classique" sur Linux (on ne parle pas de l'AppImage ici, mais rc2d_example)
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      BUILD_RPATH "\$ORIGIN;\$ORIGIN/usr/lib"
      INSTALL_RPATH "\$ORIGIN;\$ORIGIN/usr/lib"
      BUILD_WITH_INSTALL_RPATH TRUE
    )

    # ------------------------------------------------------------
    # Arch AppImage depuis RC2D_ARCH
    # ------------------------------------------------------------
    if(RC2D_ARCH STREQUAL "x64")
      set(APPIMAGE_ARCH "x86_64")
    elseif(RC2D_ARCH STREQUAL "arm64")
      set(APPIMAGE_ARCH "aarch64")
    else()
      message(FATAL_ERROR "RC2D_ARCH invalide: '${RC2D_ARCH}' (attendu: x64 ou arm64)")
    endif()

    # ------------------------------------------------------------
    # APPDIR : isolé par arch + build_type (évite collisions)
    # On le met dans le build tree, pas dans le source tree.
    # Exemple :
    #   build/linux/x64/Debug/AppDir
    #   build/linux/x64/Release/AppDir
    #   build/linux/arm64/Debug/AppDir
    # ------------------------------------------------------------
    set(APPDIR "${CMAKE_BINARY_DIR}/AppDir")
    set(APPIMAGE_OUTPUT "${CMAKE_BINARY_DIR}/${RC2D_EXAMPLE_TARGET_NAME}.AppImage")

    install(CODE
      "include(\"${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux/appimage.cmake\")
      make_appimage(
        EXE \"\$<TARGET_FILE:${RC2D_EXAMPLE_TARGET_NAME}>\"
        NAME \"${RC2D_EXAMPLE_TARGET_NAME}\"
        ICON \"${PROJECT_SOURCE_DIR}/icons/linux/app-icon.png\"
        DIR_ICON \"${PROJECT_SOURCE_DIR}/icons/linux/app-icon.png\"
        OUTPUT_NAME \"${APPIMAGE_OUTPUT}\"
        APP_VERSION \"${APP_VERSION}\"
        APP_ARCH \"${APPIMAGE_ARCH}\"
        APPDIR \"${APPDIR}\"

        # 👇 Copie des assets, shaders dans AppDir/ (à côté de l’exe)
        ASSETS
          \"${CMAKE_BINARY_DIR}/assets\"
          \"${CMAKE_BINARY_DIR}/shaders\"
      )"
      COMPONENT Runtime
    )
  elseif(RC2D_PLATFORM STREQUAL "STEAMRT4")
    # ============================================================
    # SteamRT4 — Configure RPATH to $ORIGIN
    # ============================================================
    # Explication technique :
    # - "$ORIGIN" est une variable spéciale du loader ELF.
    #   Elle représente le dossier contenant l'exécutable.
    #
    #   Exemple de structure :
    #     ./rc2d_example
    #     ./libonnxruntime.so
    #     ./libavcodec.so
    #
    #   Avec RPATH = $ORIGIN, le loader cherchera :
    #     ./libonnxruntime.so
    #
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      BUILD_RPATH "\$ORIGIN"
      INSTALL_RPATH "\$ORIGIN"
      BUILD_WITH_INSTALL_RPATH TRUE
    )
  endif()

  # ============================================================
  # Copie des ressources (dossiers : shaders, assets) dans le dossier de sortie de l'exécutable
  # Infos :
  # Android copie automatiquement les ressources dans le dossier assets/ de l'APK, donc pas besoin de les copier manuellement.
  # Sur Apple (macOS/iOS), les ressources sont intégrées dans le bundle .app, donc pas besoin de les copier manuellement.
  # ============================================================
  if(NOT ANDROID AND NOT APPLE)
    # ---------------------------------------------------------------------------
    # Shaders runtime :
    # On ne copie QUE :
    #   - examples/shaders/compiled
    #   - examples/shaders/reflection
    #
    # tools/, scripts/ et src/ restent dans le repo :
    # ---------------------------------------------------------------------------

    # Créer le dossier shaders/ à côté de l'exe (même si compiled/reflection n'existent pas)
    add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/shaders"
      COMMENT "Prépare le dossier shaders/ à côté de ${RC2D_EXAMPLE_TARGET_NAME}"
      VERBATIM
    )

    # Copier shaders/compiled/ si présent
    if(EXISTS "${PROJECT_SOURCE_DIR}/examples/shaders/compiled")
      add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/shaders/compiled"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/examples/shaders/compiled"
                "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/shaders/compiled"
        COMMENT "Copie shaders/compiled à côté de ${RC2D_EXAMPLE_TARGET_NAME}"
        VERBATIM
      )
    endif()

    # Copier shaders/reflection/ si présent
    if(EXISTS "${PROJECT_SOURCE_DIR}/examples/shaders/reflection")
      add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/shaders/reflection"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/examples/shaders/reflection"
                "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/shaders/reflection"
        COMMENT "Copie shaders/reflection à côté de ${RC2D_EXAMPLE_TARGET_NAME}"
        VERBATIM
      )
    endif()

    # Assets génériques (images, sons, etc.) :
    add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/assets"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${PROJECT_SOURCE_DIR}/examples/assets"
              "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/assets"
      COMMENT "Copie du dossier assets dans le dossier de build de ${RC2D_EXAMPLE_TARGET_NAME}"
      VERBATIM
    )
  endif()

  # ============================================================
  # Linux / SteamRT4 / Apple (macOS/iOS) — Rendre l'exécutable exécutable via chmod
  # ============================================================ 
  if((UNIX OR APPLE) AND NOT ANDROID)
    add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
      COMMAND chmod +x "$<TARGET_FILE:${RC2D_EXAMPLE_TARGET_NAME}>"
      COMMENT "Rendre ${RC2D_EXAMPLE_TARGET_NAME} exécutable"
      VERBATIM
    )
  endif()
endif()











# Pour les tests unitaires RC2D
if(RC2D_BUILD_TESTS)
  enable_testing()

  # Ajouter les fichiers source des tests unitaires
  file(GLOB_RECURSE RC2D_TEST_SOURCES
    "${PROJECT_SOURCE_DIR}/tests/src/*.c"
  )

  # Ajouter les fichiers include des tests unitaires
  file(GLOB_RECURSE RC2D_TEST_HEADERS
    "${PROJECT_SOURCE_DIR}/tests/include/*.h"
  )

  # Créer un exécutable pour les tests
  add_executable(rc2d_tests 
    ${RC2D_TEST_SOURCES} ${RC2D_TEST_HEADERS}
  )
  
  # Linker Criterion selon la plateforme
  if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS

  elseif(WIN32) # Windows
  
  elseif(UNIX AND NOT APPLE AND NOT ANDROID) # Linux

  elseif(RC2D_PLATFORM STREQUAL "STEAMRT4") # SteamRT4

  endif()

  # Link RC2D statique ou dynamique selon le choix de l'utilisateur
  target_link_libraries(rc2d_tests PRIVATE
    ${PROJECT_NAME} # RC2D
  )

  # Permet de lancer les tests avec la commande "ctest" intégrée dans CMake
  add_test(NAME RC2D_AllTests COMMAND rc2d_tests)
endif()