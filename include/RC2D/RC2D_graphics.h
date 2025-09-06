#ifndef RC2D_GRAPHICS_H
#define RC2D_GRAPHICS_H

#include <SDL3/SDL_render.h>

/* Configuration pour les définitions de fonctions C, même en C++ */
#ifdef __cplusplus
extern "C" {
#endif

void rc2d_graphics_clear(void);
void rc2d_graphics_present(void);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_GRAPHICS_H