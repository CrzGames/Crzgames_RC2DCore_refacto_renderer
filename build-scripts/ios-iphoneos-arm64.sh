#!/bin/bash
set -e

echo -e "\e[32m\nGenerating Xcode project for iOS (iphoneos, arm64)...\e[0m"

cmake -S . -B build/ios/iphoneos \
  -G Xcode \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_SYSTEM_NAME=iOS

for cfg in Debug Release; do
  echo -e "\e[32m\nBuilding $cfg (iphoneos/arm64)...\e[0m"
  cmake --build build/ios/iphoneos --config $cfg -- \
    CODE_SIGNING_ALLOWED=NO
done

echo -e "\033[32m\nLib RC2D pour iOS (iphoneos/arm64) buildée en Debug et Release.\033[0m"
