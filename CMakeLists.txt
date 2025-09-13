# CMake 3.25.0 est la version minimale requise pour ce projet
cmake_minimum_required(VERSION 3.25.0)

# Projet 
project(rc2d LANGUAGES C CXX)

# Découper la version en parties
#string(REPLACE "." ";" VERSION_LIST ${APP_VERSION})
#list(GET VERSION_LIST 0 APP_VERSION_MAJOR)
#list(GET VERSION_LIST 1 APP_VERSION_MINOR)
#list(GET VERSION_LIST 2 APP_VERSION_PATCH)

# Spécifier la version C standard
set(CMAKE_C_STANDARD 99)
# Cela signifie que le compilateur doit respecter la version C standard spécifiée
set(CMAKE_C_STANDARD_REQUIRED True)

# Option pour activer le débogage mémoire, pour savoir les fuites de mémoire si il manque des free()
option(RC2D_MEMORY_DEBUG_ENABLED "Enable RC2D memory debug tracking (realloc/malloc/free.. wrappers)" ON)

# Option pour activer la compilation des shaders en ligne (hot reload) pour SDL3_shadercross
if(ANDROID OR CMAKE_OSX_SYSROOT MATCHES "iphoneos")
  option(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED "Enable RC2D GPU shader hot reload (using SDL3_shadercross)" OFF)
else()
  option(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED "Enable RC2D GPU shader hot reload (using SDL3_shadercross)" ON)
endif()

# Option pour activer le module RC2D_video (basé sur FFmpeg)
option(RC2D_VIDEO_MODULE_ENABLED "Enable RC2D_video (FFmpeg-based)" ON)

# Option pour activer le module RC2D_data (basé sur OpenSSL)
option(RC2D_DATA_MODULE_ENABLED "Enable RC2D_data (OpenSSL-based)" ON)

# Option pour activer le module RC2D_net (via RCEnet, fork de ENet)
option(RC2D_NET_MODULE_ENABLED "Enable RC2D_net (ENet-based)" ON)

# Option pour activer le module RC2D_onnx (via ONNX Runtime)
option(RC2D_ONNX_MODULE_ENABLED "Enable RC2D_onnx (ONNX Runtime-based)" OFF)

# Option pour le niveau d'assertion
option(RC2D_ASSERT_LEVEL "Niveau des assertions (0=none, 1=release, 2=debug, 3=paranoid)" 3)

# Option Tests unitaires avec Criterion
option(RC2D_BUILD_TESTS "Build unit tests with Criterion" OFF)

# Option pour construire les exemples
option(RC2D_BUILD_EXAMPLES "Build examples" ON)

# Option pour choisir entre statique et dynamique
option(RC2D_BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Dossier de sortie
if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
  # Multi-config generators
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
else()
  # Single-config generators (Makefiles, Ninja, etc.)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# Configuration multi-plateforme avec versions minimales
if(WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.10240 CACHE STRING "Windows 10 Minimum Version" FORCE)
elseif(APPLE)
  if(CMAKE_OSX_SYSROOT MATCHES "macosx")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS Architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "macOS Deployment Target" FORCE)
  elseif(CMAKE_OSX_SYSROOT MATCHES "iphoneos")
    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS Architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "18.0" CACHE STRING "iOS Deployment Target" FORCE)
  endif()
elseif(ANDROID)
  set(CMAKE_ANDROID_API 28 CACHE STRING "Android API Level" FORCE)
endif()

# Configuration des drapeaux du compilateur
if(MSVC)
  # /W3 : Niveau d'avertissement 3 (equivalent a -Wall)
  # /W4 : Niveau d'avertissement 4 (equivalent a -Wextra)
  # /permissive- : Spécifie un mode de compilation strict pour suivre les standards C++ 
  # /wd4100 : Désactive l'avertissement pour les paramètres de fonction non utilisés (C4100)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /W4 /permissive- /wd4100")
else()
  # -Wall : Active tous les avertissements de base
  # -Wextra : Active des avertissements supplémentaires
  # -pedantic : Enforce strictement les standards C++
  # -Wno-unused-parameter : Désactive les avertissements pour les paramètres de fonction non utilisés
  # -Wno-error=strict-prototypes : Désactive les erreurs pour les prototypes de fonction stricts
  # -Wno-newline-eof : Désactive les avertissements pour les nouvelles lignes à la fin du fichier
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-error=strict-prototypes -Wno-newline-eof")
endif()

if(UNIX AND NOT APPLE AND NOT ANDROID)
  # Active les backends vidéo nécessaires pour Linux (Steam Deck, Distribution Linux, Raspberry PI 5 etc.)
  set(SDL_WAYLAND ON CACHE BOOL "Enable Wayland backend for SDL3" FORCE)
  set(SDL_X11 ON CACHE BOOL "Enable X11 backend for SDL3" FORCE)
  set(SDL_KMSDRM ON CACHE BOOL "Enable KMS/DRM backend for SDL3" FORCE)
  set(SDL_DUMMYVIDEO ON CACHE BOOL "Enable dummy video backend for SDL3" FORCE)

  find_program(PATCHELF_EXECUTABLE NAMES patchelf)
  if(NOT PATCHELF_EXECUTABLE)
    message(FATAL_ERROR "patchelf non trouvé dans le PATH")
  endif()
endif()

# SDL3_ttf, SDL3_image, SDL3_mixer : 
# Permet de dire à CMake de ne pas chercher les dépendances dans le système,
# mais de les utiliser à partir du répertoire "dependencies", dont également les submodules
# des repositories de SDL3_ttf, SDL3_mixer, SDL3_image
set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDLMIXER_VENDORED ON CACHE BOOL "" FORCE)

# SDL3_mixer : Désactiver explicitement tous les formats audio qui sont activé par défault.
# Il restera ces formats activé par défault : WAV, OGG et MP3.
if(APPLE AND CMAKE_OSX_SYSROOT MATCHES "iphoneos")
  # Spécifiquement pour iOS, on utilise minimp3 pour le MP3 qui est en statique, par rapport à mpg123.
  # et on build en statique pour éviter les problèmes de dépendances dynamiques sur iOS
  set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
  set(SDLMIXER_MP3_MPG123 OFF CACHE BOOL "" FORCE)
  set(SDLMIXER_MP3_MINIMP3 ON  CACHE BOOL "" FORCE)
endif()

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
set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_STB OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_TREMOR OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK_DSD OFF CACHE BOOL "" FORCE)

# SDL3_image : Désactiver explicitement tous les formats d'image qui sont activé par défault.
# Il restera ces formats activé par défault : STB, PNG et SVG
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BMP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_GIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JPG OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_LBM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PCX OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_PNM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_QOI OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TGA OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XCF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XPM OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_XV OFF CACHE BOOL "" FORCE)

# SDL3 :
# Il peut vous donner accès à beaucoup plus de contrôleurs de jeu,
# Mais il nécessite l'autorisation de l'utilisateur avant que votre application puisse communiquer avec le matériel Bluetooth
set(SDL_JOYSTICK_HIDAPI ON CACHE BOOL "" FORCE)

# On force la compilation en statique pour iOS (iPhone/iPad), pour SDL3, SDL3_ttf, SDL3_image, SDL3_mixer et leur sous-dépendances.
# Spécifiquement pour iOS, parce que les .dylib ne sont plus autorisées sur l'App Store.
# On le fait grâce à : set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "") pour toutes les dépendances.
if(APPLE AND CMAKE_OSX_SYSROOT MATCHES "iphoneos")
  set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
  set(SDL_SHARED OFF)
endif()

# SDL3, SDL3_ttf, SDL3_image, SDL3_mixer : 
# Permet de dire à CMake de ne pas chercher les dépendances dans le système,
# mais de les utiliser à partir du répertoire "dependencies", dont également les submodules
# des repositories de SDL3, SDL3_ttf, SDL3_mixer, SDL3_image
add_subdirectory(dependencies/SDL        EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/SDL_ttf    EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/SDL_image  EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/SDL_mixer  EXCLUDE_FROM_ALL)

# Chemin vers les dépendances
set(DEPENDENCIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
set(CRZGAMES_LIBRARIES "${DEPENDENCIES_PATH}/Crzgames_Libraries")

function(rc2d_configure_ffmpeg target_name)
  if (RC2D_VIDEO_MODULE_ENABLED)
    if (WIN32)
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/ffmpeg")

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
      
    elseif(APPLE AND CMAKE_OSX_SYSROOT MATCHES "macosx")
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/ffmpeg")

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
    elseif (APPLE AND CMAKE_OSX_SYSROOT MATCHES "iphoneos")
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/ffmpeg")

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

    elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Steam Deck, Raspberry PI 5 etc.
    
    elseif (ANDROID)
      # Link le dossier include de FFmpeg à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/ffmpeg")

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

function(rc2d_configure_rcenet target_name)
  if (RC2D_NET_MODULE_ENABLED)
    if (WIN32)
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/rcenet")

      # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/rcenet/${RC2D_ARCH}")

      # Link les librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
        ws2_32 # Bibliothèque native à Windows (pour la librairie Enet)
        winmm # Bibliothèque native à Windows (pour la librairie Enet)
      )

    elseif(APPLE AND CMAKE_OSX_SYSROOT MATCHES "macosx")
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/rcenet")

      # Ajouter explicitement la lib .dylib à linker
      target_link_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/macos/lib/rcenet"
      )

      # Link la librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )

    elseif (APPLE AND CMAKE_OSX_SYSROOT MATCHES "iphoneos")
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/rcenet")

      # Ajouter explicitement la lib .a à linker
      target_link_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/ios/lib/rcenet/arm64-iphoneos"
      )

      # Link la librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )

    elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Steam Deck, Raspberry PI 5 etc.
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/rcenet")

      # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/rcenet/${RC2D_ARCH}")

      # Link les librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )
    elseif (ANDROID)
      # Link le dossier include de RCEnet à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/rcenet")

      # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/rcenet/${ANDROID_ABI}")

      # Link les librairies RCEnet
      target_link_libraries(${target_name} PUBLIC
        rcenet
      )
    endif()
  endif()
endfunction()

function(rc2d_configure_onnxruntime target_name)
  if(RC2D_ONNX_MODULE_ENABLED)
    if(WIN32)
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include/onnxruntime")

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
    elseif (ANDROID)
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include/onnxruntime")

      # Rajouter le répertoire de la lib ONNX Runtime, pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/onnxruntime/${ANDROID_ABI}")

      # Link les librairies ONNX Runtime
      target_link_libraries(${target_name} PUBLIC
        onnxruntime
      )
    elseif (APPLE)
      if(CMAKE_OSX_SYSROOT MATCHES "macosx")
        # Link le dossier include de ONNX Runtime à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include/onnxruntime")

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
      elseif(CMAKE_OSX_SYSROOT MATCHES "iphoneos")
        # Link le dossier include de ONNX Runtime à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include/onnxruntime")

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
    elseif (RC2D_PLATFORM STREQUAL "SLR3-SNIPER") # Steam Deck / Steam Linux
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/slr3-sniper/include/onnxruntime")

      # Rajouter le répertoire de la lib ONNX Runtime (onnxruntime_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/onnxruntime/${RC2D_ARCH}")

      # Link les librairies ONNX Runtime
      target_link_libraries(${target_name} PUBLIC
        onnxruntime
      )

      # Copier aussi les libraries dynamique dans le répertoire de l'exécutable
      file(GLOB_RECURSE ONNX_SO
        "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/onnxruntime/${RC2D_ARCH}/*.so"
        "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/onnxruntime/${RC2D_ARCH}/*.so.*"
      )
      add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND cp -a ${ONNX_SO} "$<TARGET_FILE_DIR:${target_name}>/"
        COMMENT "Copie des bibliothèques ONNX Runtime avec conservation des liens symboliques"
        VERBATIM
      )
    elseif (RC2D_PLATFORM STREQUAL "Linux")
      # Link le dossier include de ONNX Runtime à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include/onnxruntime")

      # Rajouter le répertoire de la lib ONNX Runtime (onnxruntime_static), pour le linker ensuite
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
    if(RC2D_GPU_SHADER_HOT_RELOAD_ENABLED AND NOT CMAKE_OSX_SYSROOT MATCHES "iphoneos")
      # Link le dossier include de shadercross à la target
      target_include_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/macos/include"
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
  elseif(WIN32)
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
        "${CRZGAMES_LIBRARIES}/windows/include"
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
  elseif(RC2D_PLATFORM STREQUAL "SLR3-SNIPER") # Steam Deck / Steam Linux
    # Repertoire cible du binaire de shadercross pour "examples/shaders/tools"
    set(SHADERCROSS_BINARY_DIR "${CRZGAMES_LIBRARIES}/slr3-sniper/bin/SDL3_shadercross/${RC2D_ARCH}")
    
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
      "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/SDL3_shadercross/${RC2D_ARCH}/*.so"
      "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/SDL3_shadercross/${RC2D_ARCH}/*.so.*"
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
      target_include_directories(${target_name} PUBLIC
        "${CRZGAMES_LIBRARIES}/slr3-sniper/include"
      )

      # Récupérer le dossier de la lib shadercross
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/SDL3_shadercross/${RC2D_ARCH}")

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

  elseif(RC2D_PLATFORM STREQUAL "Linux")
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
        "${CRZGAMES_LIBRARIES}/linux/include"
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
    if(APPLE)
      if(CMAKE_OSX_SYSROOT MATCHES "macosx")
        # Link le dossier include de OpenSSL à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/include")

        # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/macos/lib/OpenSSL")

        # Link les librairies OpenSSL
        target_link_libraries(${target_name} PUBLIC
          ssl    # OpenSSL
          crypto # OpenSSL
        )
      elseif(CMAKE_OSX_SYSROOT MATCHES "iphoneos")
        # Link le dossier include de OpenSSL à la target
        target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/include")

        # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
        target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/ios/lib/OpenSSL/arm64-iphoneos")

        # Link les librairies OpenSSL
        target_link_libraries(${target_name} PUBLIC
          ssl    # OpenSSL
          crypto # OpenSSL
        )
      endif()
    elseif (ANDROID)
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/include")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/android/lib/openssl/${ANDROID_ABI}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        ssl    # OpenSSL
        crypto # OpenSSL
      )
    elseif(WIN32)
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/include")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/windows/lib/OpenSSL/${RC2D_ARCH}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        libssl    # OpenSSL
        libcrypto # OpenSSL
        Crypt32   # OpenSSL CryptoAPI (Librarie native de Windows, obligatoire)
        ws2_32    # OpenSSL Winsock2  (Librarie native de Windows, obligatoire)
      )
    elseif (RC2D_PLATFORM STREQUAL "Linux")
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/include")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/linux/lib/OpenSSL/${RC2D_ARCH}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        ssl    # OpenSSL
        crypto # OpenSSL
      )
    elseif(RC2D_PLATFORM STREQUAL "SLR3-SNIPER") # Steam Deck / Steam Linux
      # Link le dossier include de OpenSSL à la target
      target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/slr3-sniper/include")

      # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
      target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES}/slr3-sniper/lib/OpenSSL/${RC2D_ARCH}")

      # Link les librairies OpenSSL
      target_link_libraries(${target_name} PUBLIC
        ssl    # OpenSSL
        crypto # OpenSSL
      )
    endif()
  endif()
endfunction()

function(rc2d_force_link_linux target_name)
  if(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(${target_name} PUBLIC
      "-Wl,--no-as-needed" m pthread dl "-Wl,--as-needed"
    )
  endif()
endfunction()

# Headers de RC2D + les lib qui sont dans le répertoire "external"
function(rc2d_include_headers target_name)
  target_include_directories(${target_name} PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/external"
  )
endfunction()

function(rc2d_link_apple_frameworks target_name)
  # SDL_Image bug: https://github.com/libsdl-org/SDL_image/issues/506
  # + Notammment pour iOS les lib ci-dessous sont nécessaires vu qu'ont utilise les lib static de SDL3, SDL3_image, SDL3_mixer et SDL3_ttf
  if (APPLE AND NOT BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME} PUBLIC 
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

# Fonction pour les target_compile_definitions par target
function(rc2d_target_compile_definitions target_name)
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

# Sources du projet RC2D
file(GLOB_RECURSE SOURCES
  "${PROJECT_SOURCE_DIR}/src/*.c"
)

# Fichiers d'en-tête (headers) du projet RC2D
file(GLOB_RECURSE HEADERS
  "${PROJECT_SOURCE_DIR}/include/*.h"
)

# Choisir entre bibliothèque dynamique/partagées ou statique pour RC2D
if(RC2D_BUILD_SHARED_LIBS)
  add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
else()
  add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
  # Rajouter le suffixe "_static" au nom de la bibliothèque statique, pour éviter les conflits
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_static")
endif()

# Compiler les définitions pour la target RC2D
rc2d_target_compile_definitions(${PROJECT_NAME})

# Inclure les répertoires d'en-tête (headers) de RC2D et du répertoire "external"
rc2d_include_headers(${PROJECT_NAME})

# Link les dépendances communes à toutes les plateformes
target_link_libraries(${PROJECT_NAME} PUBLIC
  SDL3_mixer::SDL3_mixer
  SDL3_image::SDL3_image
  SDL3_ttf::SDL3_ttf
  SDL3::SDL3
)

# Link la dépendance ffmpeg si le module RC2D_video est activé
rc2d_configure_ffmpeg(${PROJECT_NAME})

# Link onnxruntime si le module RC2D_onnx est activé
rc2d_configure_onnxruntime(${PROJECT_NAME})

# Link la dépendance OpenSSL si le module RC2D_data est activé
rc2d_configure_openssl(${PROJECT_NAME})

# Linker SDL3_shadercross selon la plateforme
rc2d_configure_shadercross(${PROJECT_NAME})

# Linker les dépendances natives selon la plateforme
rc2d_force_link_linux(${PROJECT_NAME})

# Linker la dépendance RCEnet si le module RC2D_net est activé
rc2d_configure_rcenet(${PROJECT_NAME})

# Linker les frameworks Apple si nécessaire
rc2d_link_apple_frameworks(${PROJECT_NAME})

# Pour l'exemple RC2D
if(RC2D_BUILD_EXAMPLES)
  # Pour Android, on doit obligatoirement mettre "main" comme nom de target, quand c'est un exécutable
  # Pour les autres plateformes, on peut mettre "rc2d_example" comme nom de target
  if(ANDROID)
    set(RC2D_EXAMPLE_TARGET_NAME main)
  else()
    set(RC2D_EXAMPLE_TARGET_NAME rc2d_example)
  endif()

  # Variables d'environnement pour la configuration de l'application (version, nom de l'entreprise, etc.)
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

  # Pour macOS spécifiquement (bundle .app) : Identifier de l'application et version de construction
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

  # Fichiers de ressources (resource files) pour l'exécutable
  if(MSVC)
    # Generer le fichier .rc pour l'executable par rapport au fichier resource.rc.in
    # Pour Windows, le fichier resource.rc est utilisé pour définir les informations de version de l'application, le nom de l'entreprise, icone etc.
    configure_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/resource.rc.in 
      ${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/resource.rc
      @ONLY
    )
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/resource.rc" PROPERTIES LANGUAGE RC)

    # Ressource de l'executable : resource, fichiers .json, etc.
    set(RESOURCE_FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/resource.rc"
    )
  elseif(APPLE AND CMAKE_OSX_SYSROOT MATCHES "macosx")
    # Configurations spécifiques à macOS

    # Ressource de l'executable : icon, fichier .json.., exemple d'un .app : Contents/Ressources/..
    set(RESOURCE_FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/icons/macos/app-icon.icns"
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/assets"
    )
  elseif(APPLE AND CMAKE_OSX_SYSROOT MATCHES "iphoneos")
    # Configurations spécifiques à iOS

    # Ressource de l'executable : icon, fichier .json.. (notamment ce qui as dans assets)
    set(RESOURCE_FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/icons/ios/Assets.xcassets"
      "${CMAKE_CURRENT_SOURCE_DIR}/examples/assets"
    )
  endif()

  # Ajouter les fichiers source de l'exemple
  # Cela inclut tous les fichiers .c dans le répertoire "examples/src" et ses sous-répertoires
  file(GLOB_RECURSE RC2D_EXAMPLE_SOURCES
    "${PROJECT_SOURCE_DIR}/examples/src/*.c"
  )

  # Ajouter les fichiers inclus dans le répertoire "examples/include" et ses sous-répertoires
  file(GLOB_RECURSE RC2D_EXAMPLE_HEADERS
    "${PROJECT_SOURCE_DIR}/examples/include/*.h"
  )

  # Définir des flags de compilation pour le mode Release (NDEBUG = No Debug, donc Release)
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")

  # Créer un exécutable pour l'exemple
  if(ANDROID)
    # Pour Android, il faut obligatoirement mettre SHARED et "main" comme nom de target
    add_library(${RC2D_EXAMPLE_TARGET_NAME} SHARED 
      ${RC2D_EXAMPLE_SOURCES} ${RC2D_EXAMPLE_HEADERS}
    )
  else()
    add_executable(${RC2D_EXAMPLE_TARGET_NAME} 
      ${RC2D_EXAMPLE_SOURCES} ${RC2D_EXAMPLE_HEADERS}
      ${RESOURCE_FILES} # Fichier .rc doit être intégré dans l'executable (Windows uniquement)
    )
  endif()

  if (WIN32)
    # Définir le projet de démarrage dans Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${RC2D_EXAMPLE_TARGET_NAME})

    # Définir le répertoire de travail pour le débogueur dans Visual Studio
    # Cela permet de lancer l'exécutable avec le bon répertoire de travail (working directory)
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>"
    )
  endif()

  # Les fichiers d'en-tête (headers) de l'exemple sont dans le répertoire "examples/include"
  target_include_directories(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/examples/include"
  )

  # Link la dépendance ffmpeg si le module RC2D_video est activé
  rc2d_configure_ffmpeg(${RC2D_EXAMPLE_TARGET_NAME})

  # Linker la dépendance OpenSSL non commune à toutes les plateformes
  # Cela dépend de la plateforme, donc on le fait dans la fonction rc2d_configure_openssl
  rc2d_configure_openssl(${RC2D_EXAMPLE_TARGET_NAME})

  # Linker SDL3_shadercross selon la plateforme
  rc2d_configure_shadercross(${RC2D_EXAMPLE_TARGET_NAME})

  # Link onnxruntime si le module RC2D_onnx est activé
  rc2d_configure_onnxruntime(${RC2D_EXAMPLE_TARGET_NAME})

  # Link la dépendance RC2D statique ou dynamique selon le choix de l'utilisateur
  target_link_libraries(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE
    ${PROJECT_NAME} # RC2D
  )

  if(APPLE AND CMAKE_OSX_SYSROOT MATCHES "iphoneos")
    # Ajouter le dossier Assets.xcassets comme ressource
    set(ASSETS_ICONS "${CMAKE_CURRENT_SOURCE_DIR}/icons/ios/Assets.xcassets")
    target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE ${ASSETS_ICONS})
    set_source_files_properties(${ASSETS_ICONS} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Ajouter le fichier storyboard comme ressource
    set(STORYBOARD_FILE "${CMAKE_SOURCE_DIR}/platforms/ios/LaunchScreen.storyboard")
    target_sources(${RC2D_EXAMPLE_TARGET_NAME} PRIVATE ${STORYBOARD_FILE})
    set_source_files_properties(${STORYBOARD_FILE} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Configurez le bundle (.app)
    set_target_properties(${RC2D_EXAMPLE_TARGET_NAME} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_NAME ${RC2D_EXAMPLE_TARGET_NAME} # Nom de l'application (à modifier pour chaque application)
      MACOSX_BUNDLE_GUI_IDENTIFIER "com.crzgames.testexe" # L'identifiant de votre application (via identifiers sur le compte apple developer) à modifier pour chaque application
      MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0" # Version utilisateur lisible (à modifier pour chaque application)
      MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0" # Version de build unique (à modifier pour chaque application)
      MACOSX_BUNDLE_COPYRIGHT "Crgames" # Droits d'auteur
      MACOSX_BUNDLE_INFO_STRING "Description de l'application" # Description de l'application (à modifier pour chaque application)
      XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon" # Nom de l'icône de l'application
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # 1 pour iPhone, 2 pour iPad
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "U3D28WJ8DV" # Remplacez par votre Team ID Apple
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development: Corentin Recanzone (2X96T6AM4K)" # Remplacez par votre identité de signature
      XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual" # Style de signature (Manual ou Automatic)
      XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --strict --force --timestamp --verbose" # Options supplémentaires pour la signature
      XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "TestExe" # Remplacez par le nom de votre profil de provisionnement
      XCODE_ATTRIBUTE_PROVISIONING_PROFILE "d5115451-c438-4aa6-9c68-dee8a87a36e8" # Remplacez par l'UUID de votre profil de provisionnement
      RESOURCE "${RESOURCE_FILES}" # Fichiers de ressources à inclure dans le bundle .app
    )

    # Ajoute une étape de post-build pour ajouter des clés au fichier Info.plist
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      # Ajoute les clés pour le bluetooth
      COMMAND /usr/libexec/PlistBuddy -c "Add :UIApplicationSupportsIndirectInputEvents bool true" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding keys bluetooth to Info.plist"

      # Ajoute les clés pour les souris
      COMMAND /usr/libexec/PlistBuddy -c "Add :NSBluetoothPeripheralUsageDescription string 'MyApp souhaite rester connecté aux contrôleurs de jeu et aux manettes de jeu Bluetooth à proximité, même lorsque vous n'utilisez pas l'application.'" "$<TARGET_FILE_DIR:${RC2D_EXAMPLE_TARGET_NAME}>/Info.plist"
      COMMENT "Adding keys mouse to Info.plist"

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

      # Copie le storyboard de lancement dans le bundle .app
      COMMAND cp -a "${PROJECT_SOURCE_DIR}/platforms/ios/LaunchScreen.storyboard" "${CMAKE_BINARY_DIR}/$<CONFIG>/${RC2D_EXAMPLE_TARGET_NAME}.app/"
      COMMENT "Copying LaunchScreen.storyboard to bundle .app directory"
    )
  endif()

  # Copier le dossier shaders dans le dossier de sortie de rc2d_example (exécutable)
  add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:rc2d_example>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/examples/shaders"
            "$<TARGET_FILE_DIR:rc2d_example>/shaders"
    COMMENT "Copie du dossier shaders dans le dossier de build de rc2d_example"
  )

  # Copier le dossier assets dans le dossier de sortie de rc2d_example (exécutable)
  add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:rc2d_example>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/examples/assets"
            "$<TARGET_FILE_DIR:rc2d_example>/assets"
    COMMENT "Copie du dossier assets dans le dossier de build de rc2d_example"
  )

  if (UNIX OR APPLE AND NOT ANDROID)
    # Rendre rc2d_example exécutable (important s’il est copié depuis une archive)
    add_custom_command(TARGET ${RC2D_EXAMPLE_TARGET_NAME} POST_BUILD
      COMMAND chmod +x "$<TARGET_FILE_DIR:rc2d_example>/rc2d_example"
      COMMENT "Rendre rc2d_example exécutable"
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
  if(APPLE)
  elseif(WIN32)
  
  elseif(LINUX)

  endif()

  # Link RC2D statique ou dynamique selon le choix de l'utilisateur
  target_link_libraries(rc2d_tests PRIVATE
    ${PROJECT_NAME} # RC2D
  )

  # Permet de lancer les tests avec la commande "ctest" intégrée dans CMake
  add_test(NAME RC2D_AllTests COMMAND rc2d_tests)
endif()