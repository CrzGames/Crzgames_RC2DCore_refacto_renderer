#ifndef RC2D_EXPORT_H
#define RC2D_EXPORT_H

#include <RC2D/RC2D_platform_defines.h> // Required for : RC2D_PLATFORM_*

#if defined(RC2D_PLATFORM_WINDOWS)
  #if defined(RC2D_BUILDING_SHARED)
    #define RC2D_DECLSPEC __declspec(dllexport)
  #else
    #define RC2D_DECLSPEC __declspec(dllimport)
  #endif
  #define RC2D_CALL __cdecl
#else
  #define RC2D_DECLSPEC __attribute__((visibility("default")))
  #define RC2D_CALL
#endif

#endif // RC2D_EXPORT_H
