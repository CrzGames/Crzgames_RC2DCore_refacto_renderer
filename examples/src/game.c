#include <mygame/game.h>
#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>

void rc2d_unload(void) 
{
    RC2D_log(RC2D_LOG_INFO, "My game is unloading...\n");
}

void rc2d_load(void) 
{
    RC2D_log(RC2D_LOG_INFO, "My game is loading...\n");
}

void rc2d_update(double dt) 
{

}

void rc2d_draw(void)
{
    SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 255, 0, 0, 255); // Couleur rouge
    SDL_FRect rect = {100, 100, 200, 200};
    SDL_RenderRect(rc2d_engine_state.renderer, &rect);
}