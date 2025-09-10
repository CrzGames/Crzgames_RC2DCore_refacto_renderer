#ifndef GAME_H
#define GAME_H

#include <RC2D/RC2D_engine.h>

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

void rc2d_unload(void);
void rc2d_load(void);
void rc2d_update(double dt);
void rc2d_draw(void);
void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // GAME_H