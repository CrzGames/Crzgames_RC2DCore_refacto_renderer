#include <mygame/game.h>
#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>
#include <SDL3/SDL.h>

/* =========================================================================
   MAP LAYOUT (placement de la carte / océan dans la zone visible)
   -------------------------------------------------------------------------
   Objectif :
   - On récupère la "zone sûre visible" via rc2d_engine_getVisibleSafeRectRender(),
     c’est-à-dire la portion garantie visible (safe area ∩ overscan).
   - On enlève des marges ("insets") pour laisser de la place à l’interface
     (minimap, chat, boutons…).
   - Contrairement au HUD, ici la MAP est un fond de jeu → pas de gestion
     de ratio, SDL Logical Presentation s’en occupe déjà.
   ========================================================================= */

/**
 * \brief Marges autour de la zone visible.
 * - Si percent==false : valeurs en pixels logiques.
 * - Si percent==true  : valeurs proportionnelles (0.0–1.0).
 */
typedef struct MapInsets {
    float left;    /**< marge gauche   */
    float top;     /**< marge haut     */
    float right;   /**< marge droite   */
    float bottom;  /**< marge bas      */
    bool  percent; /**< true = %age, false = pixels logiques */
} MapInsets;

/* Exemple de configuration :
   - 200px à gauche/droite (pour minimap, chat…)
   - 50px en haut/bas (barres de boutons)
 */
static MapInsets mapInsets = {
    200.0f,  // left
    50.0f,   // top
    200.0f,  // right
    50.0f,   // bottom
    false    // interprétation en pixels logiques
};

/**
 * \brief Rectangle final de la MAP (dans l’espace logique rendu).
 * 
 * Mis à jour à chaque frame dans `rc2d_update()`, utilisé pour :
 * - dessiner l’océan,
 * - placer d’autres entités “dans le monde”.
 */
static SDL_FRect mapRect = {0,0,0,0};

/**
 * \brief Calcule un rectangle en retirant les insets d’une zone donnée.
 * 
 * @param visibleSafe zone visible et sûre (safe-area ∩ overscan)
 * @param insets marges à appliquer
 * @return rectangle final (jamais négatif)
 */
static inline SDL_FRect computeMapRect(const SDL_FRect visibleSafe, const MapInsets insets)
{
    // Conversion : pixels logiques OU %age de la zone
    const float L = insets.percent ? visibleSafe.w * insets.left   : insets.left;
    const float T = insets.percent ? visibleSafe.h * insets.top    : insets.top;
    const float R = insets.percent ? visibleSafe.w * insets.right  : insets.right;
    const float B = insets.percent ? visibleSafe.h * insets.bottom : insets.bottom;

    SDL_FRect out;
    out.x = visibleSafe.x + L;
    out.y = visibleSafe.y + T;
    out.w = visibleSafe.w - (L + R);
    out.h = visibleSafe.h - (T + B);

    // Clamp pour éviter des valeurs négatives
    if (out.w < 0.f) out.w = 0.f;
    if (out.h < 0.f) out.h = 0.f;
    return out;
}

/* =========================================================================
   GPU EFFECT: OCEAN
   -------------------------------------------------------------------------
   Le fond de la MAP est un océan animé via un shader.
   Uniforms :
   - params0 : [ time, strength, px_amp, tiling ]
   - params1 : [ width, height, speed, extra ]
   ========================================================================= */

typedef struct OceanUniforms {
    float params0[4]; /**< time, strength, px_amp, tiling   */
    float params1[4]; /**< width, height, speed, extra(use) */
} OceanUniforms;

static RC2D_Image          oceanTile        = {0};   /**< texture de base (tile eau) */
static RC2D_GPUShader*     oceanShader      = NULL;  /**< fragment shader eau        */
static SDL_GPURenderState* oceanRenderState = NULL;  /**< pipeline rendu eau         */
static SDL_GPUSampler*     repeatSampler    = NULL;  /**< sampler REPEAT             */
static OceanUniforms       oceanU           = {0};   /**< uniforms shader            */
static double              timeSeconds      = 0.0;   /**< horloge locale             */

/**
 * \brief Met à jour les uniforms du shader océan.
 * 
 * @param dt temps écoulé depuis la dernière frame (secondes)
 */
static void updateOceanUniforms(double dt)
{
    timeSeconds += dt;

    oceanU.params0[0] = (float)timeSeconds; // temps animé
    oceanU.params1[0] = mapRect.w;          // largeur zone MAP
    oceanU.params1[1] = mapRect.h;          // hauteur zone MAP

    SDL_SetGPURenderStateFragmentUniforms(oceanRenderState, 0, &oceanU, sizeof(oceanU));
}

/* ========================================================================= */
/*                                UNLOAD                                     */
/* ========================================================================= */
void rc2d_unload(void)
{
    rc2d_graphics_freeImage(&oceanTile);

    if (oceanRenderState) 
    {
        SDL_DestroyGPURenderState(oceanRenderState);
        oceanRenderState = NULL;
    }
}

/* ========================================================================= */
/*                                 LOAD                                      */
/* ========================================================================= */
void rc2d_load(void)
{
    // Fenêtre de test (peut être enlevée si tu as déjà un rc2d_window_setSize ailleurs)
    rc2d_window_setSize(1280, 720);

    // 1) Charger le shader
    oceanShader = rc2d_gpu_loadGraphicsShaderFromStorage("water.fragment", RC2D_STORAGE_TITLE);
    if (!oceanShader) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ocean shader: %s", SDL_GetError());
        return;
    }

    // 2) Initialiser les uniforms
    oceanU.params0[0] = 0.0f;   // time
    oceanU.params0[1] = 0.6f;   // strength
    oceanU.params0[2] = 30.0f;  // px_amp
    oceanU.params0[3] = 3.0f;   // tiling
    oceanU.params1[2] = 0.60f;  // speed
    oceanU.params1[3] = 0.25f;  // extra: reflet/Fresnel

    // 3) Créer un sampler REPEAT
    SDL_GPUSamplerCreateInfo s = {0};
    s.min_filter    = SDL_GPU_FILTER_LINEAR;
    s.mag_filter    = SDL_GPU_FILTER_LINEAR;
    s.mipmap_mode   = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    s.address_mode_u= SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_v= SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_w= SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    repeatSampler = SDL_CreateGPUSampler(rc2d_engine_state.gpu_device, &s);

    // 4) Charger la texture tile
    oceanTile = rc2d_graphics_loadImageFromStorage("assets/images/tile-water.png", RC2D_STORAGE_TITLE);
    SDL_PropertiesID props = SDL_GetTextureProperties(oceanTile.sdl_texture);
    SDL_GPUTexture* texGPU = (SDL_GPUTexture*)SDL_GetPointerProperty(
        props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL
    );

    // 5) Construire l’état GPU (shader + sampler)
    SDL_GPUTextureSamplerBinding sb[1] = {0};
    sb[0].texture = texGPU;
    sb[0].sampler = repeatSampler;

    SDL_GPURenderStateCreateInfo rs = {0};
    rs.fragment_shader      = oceanShader;
    rs.num_sampler_bindings = 1;
    rs.sampler_bindings     = sb;
    oceanRenderState = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs);
}

/* ========================================================================= */
/*                            UPDATE                                         */
/* ========================================================================= */
void rc2d_update(double dt)
{
    // Zone visible (safe + overscan corrigé)
    const SDL_FRect visibleSafe = rc2d_engine_getVisibleSafeRectRender();

    // Calcul ou se situera la MAP dans la zone visible par rapport aux insets
    mapRect = computeMapRect(visibleSafe, mapInsets);

    // Mise à jour des uniforms océan
    updateOceanUniforms(dt);
}

/* ========================================================================= */
/*                                 DRAW                                      */
/* ========================================================================= */
void rc2d_draw(void)
{
    // 1) Dessiner l’océan dans la zone MAP
    if (oceanTile.sdl_texture && oceanRenderState && mapRect.w > 0.f && mapRect.h > 0.f)
    {
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, oceanRenderState);
        SDL_RenderTexture(rc2d_engine_state.renderer, oceanTile.sdl_texture, NULL, &mapRect);
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, NULL);
    }

    // Ensuite : dessiner l’UI par-dessus dans les marges
}

/* ========================================================================= */
/*                              MOUSE INPUT                                  */
/* ========================================================================= */
void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n", x, y, button, clicks, mouseID);
}

/* ========================================================================= */
/*                              KEYBOARD INPUT                               */
/* ========================================================================= */
void rc2d_keypressed(const char *key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID)
{
    RC2D_log(RC2D_LOG_INFO, "Key pressed: key=%s, scancode=%d, keycode=%d, mod=%d, isrepeat=%d, keyboardID=%d\n", key, scancode, keycode, mod, isrepeat, keyboardID);
}
