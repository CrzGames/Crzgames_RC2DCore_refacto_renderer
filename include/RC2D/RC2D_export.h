#ifndef RC2D_EXPORT_H
#define RC2D_EXPORT_H

#include <RC2D/RC2D_platform_defines.h> // Required for : RC2D_PLATFORM_*

/**
 * \brief Définitions pour l'exportation des symboles (fonctions) dans une bibliothèque partagée (DLL).
 * Cela permet de gérer la visibilité des fonctions lors de la compilation et du linkage.
*/
// Windows (MSVC, MinGW, etc.)
#if defined(RC2D_PLATFORM_WINDOWS)
#if defined(RC2D_BUILDING_SHARED)
#define RC2D_DECLSPEC __declspec(dllexport)
#endif // RC2D_BUILDING_SHARED
#if defined(RC2D_USING_SHARED)
#define RC2D_DECLSPEC __declspec(dllimport)
#endif // RC2D_USING_SHARED
#define RC2D_CALL __cdecl
#else
#define RC2D_DECLSPEC __attribute__((visibility("default")))
#define RC2D_CALL
#endif // RC2D_PLATFORM_WINDOWS

// macOS / Linux / Unix
#if defined(RC2D_PLATFORM_MACOS) || defined(RC2D_PLATFORM_LINUX) || defined(RC2D_PLATFORM_UNIX)
#if defined(RC2D_BUILDING_SHARED) || defined(RC2D_USING_SHARED)
#define RC2D_DECLSPEC __attribute__((visibility("default")))
#define RC2D_CALL
#endif // RC2D_BUILDING_SHARED || RC2D_USING_SHARED
#endif // RC2D_PLATFORM_MACOS || RC2D_PLATFORM_LINUX || RC2D_PLATFORM_UNIX

#endif // RC2D_EXPORT_H
