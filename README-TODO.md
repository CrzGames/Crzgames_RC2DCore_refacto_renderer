TODO 1 :
- Compiler ffmpeg pour SteamRT4 manquant dans Crzgames_Libraries <br />
  (Conseille : ajouté au projet Crzgames_Builder_Libraries, pour compiler ffmpeg sur : Windows(x64/arm64), macOS(arm64), Linux(x64/arm64), iOS(arm64-iphoneos), Android(arm32/arm64), SteamRT4(x64/arm64)).
- CMakelists.txt : Pour le moment pour SteamRT4 pour ffmpeg, on utilise celle de : Linux x64/arm64 classique en attendant.



TODO 2 :
- Dans Crzgames_Builder_Libraries, certaines libraries dynamique (.so) casse les symlink, parce qu'il faut que ce sois déployer sur les release github via .tar.gz ou flag de .zip pour conserver les symlink. (notamment onnxruntime pour macos et SDL3_shadercross pour SteamRT4 et Linux)



TODO 3 :
- iOS/ffmpeg: La librarie ffmpeg récupérer est en version min os : 18.5.0, on attends de build via les sources pour ffmpeg pour toute les plateformes, pour le gérer en 18.0.0 poue iOS



TODO 4 :
- Fixer tout les warning pour GCC / CLANG / CL



TODO 5 :
- Ajouter un clang format, l'équivalent d'un prettier pour C/C++.



TODO 6 : 
- Générer la documentation à partir des fichiers include, via doxygen ou autres (similaire à SDL3).



TODO 7 : 
- Finir la CI/CD, juste concernant l'upload des artifact / déploiement des artifact dans les release github sinon le build..etc OK.



TODO 8 : 
Mettre : 

if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

au lieu de : 

if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
  # Multi-config generators
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
else()
  # Single-config generators (Makefiles, Ninja, etc.)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()



TODO 9 : 
- Finir dans le CMakelists.txt pour le SDK EOS / SDK SteamWorks qui est configuré que pour Windows pour le moment.
- Finir le module RC2D_eos.c (non fini, mais commencé).