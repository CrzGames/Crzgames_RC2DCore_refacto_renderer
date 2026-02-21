TODO 1 :
- Compiler ffmpeg pour SteamRT4 manquant dans Crzgames_Libraries <br />
  (Conseille : ajouté au projet Crzgames_Builder_Libraries, pour compiler ffmpeg sur : Windows(x64/arm64), macOS(arm64), Linux(x64/arm64), iOS(arm64-iphoneos), Android(arm32/arm64), SteamRT4(x64/arm64)).
- CMakelists.txt : Pour le moment pour SteamRT4 pour ffmpeg, on utilise celle de : Linux x64/arm64 classique en attendant.



TODO 2 :
- Dans Crzgames_Builder_Libraries, certaines libraries dynamique (.so) casse les symlink, parce qu'il faut que ce sois déployer sur les release github via .tar.gz ou flag de .zip pour conserver les symlink.