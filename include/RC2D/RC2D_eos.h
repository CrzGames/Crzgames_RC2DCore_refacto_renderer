#ifndef RC2D_EOS_H
#define RC2D_EOS_H

#if RC2D_EOS_SDK_ENABLED

// Include Standard Libraries C
#include <stdbool.h>

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

bool rc2d_epic_unlockAchievement(const char* api_name);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_EOS_SDK_ENABLED

#endif // RC2D_EOS_H