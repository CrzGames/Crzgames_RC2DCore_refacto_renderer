#pragma once

#if defined(_WIN32)
  #if defined(RC2D_BUILDING_SHARED)
    #define RC2D_API __declspec(dllexport)
  #else
    #define RC2D_API
  #endif
  #define RC2D_CALL __cdecl
#else
  #define RC2D_API __attribute__((visibility("default")))
  #define RC2D_CALL
#endif
