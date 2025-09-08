#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_graphics.h>

void rc2d_graphics_clear(void)
{
    if (rc2d_engine_state.renderer) 
    {
        // 1) On dessine dans la render target
        if (rc2d_engine_state.render_target)
        {
            //SDL_SetRenderTarget(rc2d_engine_state.renderer, rc2d_engine_state.render_target);
        }

        // 2) Clear avec écran noir
        SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 0, 0, 0, 255);
        SDL_RenderClear(rc2d_engine_state.renderer);
    }
}

void rc2d_graphics_present(void)
{
    if (rc2d_engine_state.renderer) 
    {
        // 1) On revient au backbuffer
        //SDL_SetRenderTarget(rc2d_engine_state.renderer, NULL);

        // 2) Un seul draw call final
        //SDL_RenderTexture(rc2d_engine_state.renderer,
        //    rc2d_engine_state.render_target,
        //    NULL, // src = texture entière
        //    NULL  // dst = laissé à SDL (logical presentation applique letterbox/intscale)
        //);

        // 3) Présentation à l'écran
        SDL_RenderPresent(rc2d_engine_state.renderer);
    }
}