#include <mygame/game.h>
#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>
#include <SDL3/SDL.h>

/* ===== Gouttières autour de la MAP (logic coords, safe & overscan aware) ===== */
typedef struct RC2D_UIGutters {
    float left, top, right, bottom; /* px logiques ou % si percent=true */
    bool  percent;
} RC2D_UIGutters;

/* Ton besoin : peu en haut/bas, beaucoup à gauche/droite */
static RC2D_UIGutters g_gui_gutters = {
    420.0f,   /* left   : minimap / panel gauche */
    16.0f,    /* top    : barre boutons top */
    520.0f,   /* right  : chat / events / panel droit */
    56.0f,    /* bottom : barre boutons bas */
    false     /* percent = false (pixels logiques) */
};

static SDL_FRect g_ocean_dst = SDL_FRect{0,0,0,0};

static SDL_FRect rc2d_get_available_rect_with_gutters(RC2D_UIGutters G)
{
    SDL_FRect V = rc2d_engine_getVisibleSafeRectRender(); /* déjà OVERSCAN + safe */
    if (V.w <= 0.f || V.h <= 0.f) return SDL_FRect{0,0,0,0};

    float L = G.percent ? V.w * G.left   : G.left;
    float T = G.percent ? V.h * G.top    : G.top;
    float R = G.percent ? V.w * G.right  : G.right;
    float B = G.percent ? V.h * G.bottom : G.bottom;

    SDL_FRect A = SDL_FRect{ V.x + L, V.y + T, V.w - (L + R), V.h - (T + B) };
    if (A.w < 0.f) A.w = 0.f;
    if (A.h < 0.f) A.h = 0.f;
    return A;
}

/* Option ratio 16:9, ancré TOP-LEFT, clampé à la zone dispo */
static inline SDL_FRect ocean_layout_fit_ratio_top_left(SDL_FRect avail, float targetW, float targetH)
{
    if (avail.w <= 0.f || avail.h <= 0.f) return SDL_FRect{0,0,0,0};
    float s = SDL_min(avail.w / targetW, avail.h / targetH);
    if (s <= 0.f) return SDL_FRect{avail.x, avail.y, 0, 0};
    float w = targetW * s, h = targetH * s;
    return SDL_FRect{ avail.x, avail.y, w, h }; /* TOP-LEFT */
}

/* ========================================================================= */
/*                          GPU EFFECT: OCEAN                                */
/* ========================================================================= */

typedef struct OceanUniforms {
    float params0[4]; // time, strength, px_amp, tiling
    float params1[4]; // width, height, speed, unused
} OceanUniforms; // 32 bytes, aligné 16B

static RC2D_Image          tile_ocean_image = {0};
static RC2D_GPUShader*     g_ocean_fragment_shader = NULL;
static SDL_GPURenderState* g_ocean_state = NULL;
static OceanUniforms       g_ocean_u     = {0};
static double              g_time_accum  = 0.0;
static SDL_GPUSampler*     g_repeat_sampler = NULL;

/* ========================================================================= */
/*                              RESSOURCES                                   */
/* ========================================================================= */
static RC2D_TP_Atlas g_elite27_atlas = {0};

static const char* s_elite27_names[] = {
    "1.png","2.png","3.png","4.png","5.png","6.png","7.png","8.png"
};

static void Ocean_UpdateUniforms(double dt)
{
    g_time_accum += dt;
    g_ocean_u.params0[0] = (float)g_time_accum;
    g_ocean_u.params1[0] = g_ocean_dst.w;
    g_ocean_u.params1[1] = g_ocean_dst.h;
    SDL_SetGPURenderStateFragmentUniforms(g_ocean_state, 0, &g_ocean_u, sizeof(g_ocean_u));
}

/* ========================================================================= */
/*                                UNLOAD                                     */
/* ========================================================================= */
void rc2d_unload(void)
{
    rc2d_graphics_freeImage(&tile_ocean_image);
    rc2d_tp_freeAtlas(&g_elite27_atlas);

    if (g_ocean_state) {
        SDL_DestroyGPURenderState(g_ocean_state);
        g_ocean_state = NULL;
    }
    
    RC2D_log(RC2D_LOG_INFO, "My game is unloading...\n");
}

/* ========================================================================= */
/*                                 LOAD                                      */
/* ========================================================================= */
void rc2d_load(void)
{
    RC2D_log(RC2D_LOG_INFO, "My game is loading...\n");

    // Set window size to 1280x720 for testing
    rc2d_window_setSize(1280, 720);

    // Charger le shader depuis le stockage
    g_ocean_fragment_shader = rc2d_gpu_loadGraphicsShaderFromStorage("water.fragment", RC2D_STORAGE_TITLE);
    if (!g_ocean_fragment_shader) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ocean shader from assets/water.fragment: %s", SDL_GetError());
        return;
    }

    // Initialiser les uniforms
    g_ocean_u.params0[0] = 0.0f;   // time
    g_ocean_u.params0[1] = 0.6f;   // strength
    g_ocean_u.params0[2] = 30.0f;  // px_amp
    g_ocean_u.params0[3] = 3.0f;   // tiling

    g_ocean_u.params1[0] = 1280.0f; // width
    g_ocean_u.params1[1] = 720.0f;  // height
    g_ocean_u.params1[2] = 0.60f;   // speed
    g_ocean_u.params1[3] = 0.25f;   // reflet/Fresnel

    /* 1) Sampler REPEAT */
    SDL_GPUSamplerCreateInfo s = {0};
    s.min_filter = SDL_GPU_FILTER_LINEAR;
    s.mag_filter = SDL_GPU_FILTER_LINEAR;
    s.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    s.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    g_repeat_sampler = SDL_CreateGPUSampler(rc2d_engine_state.gpu_device, &s);

    /* 2) Texture GPU de la tile eau */
    tile_ocean_image = rc2d_graphics_loadImageFromStorage("assets/images/tile-water.png", RC2D_STORAGE_TITLE);
    SDL_PropertiesID props = SDL_GetTextureProperties(tile_ocean_image.sdl_texture);
    if (!props) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_GetTextureProperties failed: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* textureGPUWater = (SDL_GPUTexture*)SDL_GetPointerProperty(
        props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL
    );
    if (!textureGPUWater) {
        RC2D_log(RC2D_LOG_ERROR, "(1)No SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER on this texture");
        return;
    }

    /* 3) RenderState avec shader + sampler */
    SDL_GPUTextureSamplerBinding sb[1] = {0};
    sb[0].texture = textureGPUWater;
    sb[0].sampler = g_repeat_sampler;

    SDL_GPURenderStateCreateInfo rs = {0};
    rs.fragment_shader      = g_ocean_fragment_shader;
    rs.num_sampler_bindings = 1;
    rs.sampler_bindings     = sb;
    g_ocean_state = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs);
    if (!g_ocean_state) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateGPURenderState failed: %s", SDL_GetError());
    }

    /* 4) Uniforms init */
    if (!SDL_SetGPURenderStateFragmentUniforms(g_ocean_state, 0, &g_ocean_u, sizeof(g_ocean_u))) {
        RC2D_log(RC2D_LOG_ERROR, "Set uniforms failed: %s", SDL_GetError());
    }

    /* 5) Atlas */
    g_elite27_atlas = rc2d_tp_loadAtlasFromStorage("assets/atlas/elite24/elite24.json", RC2D_STORAGE_TITLE);
    if (!g_elite27_atlas.atlas_image.sdl_texture) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load elite27 atlas");
    }
}

/* ========================================================================= */
/*                                UPDATE                                     */
/* ========================================================================= */
void rc2d_update(double dt)
{
    SDL_FRect avail = rc2d_get_available_rect_with_gutters(g_gui_gutters);

    /* MAP/OCEAN top-left en 16:9 dans l’espace dispo */
    g_ocean_dst = ocean_layout_fit_ratio_top_left(avail, 1920.f, 1080.f);

    /* --- Uniforms shader : résolution = taille locale DU RECT --- */
    g_time_accum            += dt;
    g_ocean_u.params0[0]     = (float)g_time_accum;  /* time */
    g_ocean_u.params1[0]     = g_ocean_dst.w;        /* width locale */
    g_ocean_u.params1[1]     = g_ocean_dst.h;        /* height locale */

    SDL_SetGPURenderStateFragmentUniforms(g_ocean_state, 0, &g_ocean_u, sizeof(g_ocean_u));
}

/* ========================================================================= */
/*                                 DRAW                                      */
/* ========================================================================= */
void rc2d_draw(void)
{
    if (tile_ocean_image.sdl_texture && g_ocean_state && g_ocean_dst.w > 0.f && g_ocean_dst.h > 0.f)
    {
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, g_ocean_state);
        SDL_RenderTexture(rc2d_engine_state.renderer, tile_ocean_image.sdl_texture, NULL, &g_ocean_dst);
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, NULL);
    }

    /* ... dessine ta GUI dans les zones "gouttières" autour de g_ocean_dst ... */
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
