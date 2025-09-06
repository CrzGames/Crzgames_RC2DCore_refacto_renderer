#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_graphics.h>

void rc2d_graphics_clear(void)
{
    if (rc2d_engine_state.renderer) 
    {
        SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 0, 0, 0, 255); // Couleur de fond noire
        SDL_RenderClear(rc2d_engine_state.renderer);
    }
}

void rc2d_graphics_present(void)
{
    if (rc2d_engine_state.renderer) 
    {
        SDL_RenderPresent(rc2d_engine_state.renderer);
    }
}