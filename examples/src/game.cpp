#include <mygame/game.h>
#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>

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

/* ========================================================================= */
/*                              RESSOURCES                                   */
/* ========================================================================= */
static RC2D_TP_Atlas g_elite27_atlas = {0};

static const char* s_elite27_names[] = {
    "1.png","2.png","3.png","4.png","5.png","6.png","7.png","8.png"
};

static void Ocean_UpdateUniforms(SDL_Renderer* renderer, int out_w, int out_h, double dt)
{
    g_time_accum += dt;

    // time
    g_ocean_u.params0[0] = (float)g_time_accum;

    // NE PAS toucher à params0[1..3] si tu ne les animes pas
    // g_ocean_u.params0[1] = strength;
    // g_ocean_u.params0[2] = px_amp;
    // g_ocean_u.params0[3] = tiling;

    // resolution
    g_ocean_u.params1[0] = (float)out_w;
    g_ocean_u.params1[1] = (float)out_h;

    // NE PAS remettre à 0 : speed et causticIntensity
    // g_ocean_u.params1[2] = speed;            // si tu veux l’animer, ok
    // g_ocean_u.params1[3] = causticIntensity; // surtout pas 0 chaque frame !

    SDL_SetGPURenderStateFragmentUniforms(g_ocean_state, 0, &g_ocean_u, sizeof(g_ocean_u));
}

/* ========================================================================= */
/*                                UNLOAD                                     */
/* ========================================================================= */
void rc2d_unload(void)
{
    rc2d_graphics_freeImage(&tile_ocean_image);

    rc2d_tp_freeAtlas(&g_elite27_atlas);

    if (g_ocean_state) 
    {
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
    g_ocean_u.params0[1] = 0.6f;   // strength (0.4..0.8 pour un menu)
    g_ocean_u.params0[2] = 30.0f;  // px_amp : ~18 px visibles
    g_ocean_u.params0[3] = 3.0f;   // tiling : 6 répétitions

    g_ocean_u.params1[0] = 1280.0f; // width
    g_ocean_u.params1[1] = 720.0f;  // height
    g_ocean_u.params1[2] = 0.60f;   // speed (0.0..1.0)
    g_ocean_u.params1[3] = 0.25f; // reflet/Fresnel

    /**
     * 1) Créer un sampler en REPEAT
     */
    SDL_GPUSamplerCreateInfo s = {0};
    s.min_filter = SDL_GPU_FILTER_LINEAR;
    s.mag_filter = SDL_GPU_FILTER_LINEAR;
    s.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    s.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    SDL_GPUSampler* repeatSampler = SDL_CreateGPUSampler(rc2d_engine_state.gpu_device, &s);

    /**
     * 2) Loader l'image de la texture de la tile d'eau, puis récupérer la texture GPU
     */
    tile_ocean_image = rc2d_graphics_loadImageFromStorage("assets/images/tile-water.png", RC2D_STORAGE_TITLE);
    SDL_PropertiesID props = SDL_GetTextureProperties(tile_ocean_image.sdl_texture);
    if (!props) 
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_GetTextureProperties failed: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* textureGPUWater = (SDL_GPUTexture*)SDL_GetPointerProperty(
        props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL
    );
    if (!textureGPUWater) 
    {
        RC2D_log(RC2D_LOG_ERROR, "(1)No SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER on this texture");
        return;
    }

    /**
     * 3) Créer l’état GPU avec le shader et le sampler
     */    
    SDL_GPUTextureSamplerBinding sb[1] = {0};
    sb[0].texture = textureGPUWater;
    sb[0].sampler = repeatSampler;

    SDL_GPURenderStateCreateInfo rs = {0};
    rs.fragment_shader      = g_ocean_fragment_shader;
    rs.num_sampler_bindings = 1;              // <-- plus que 1
    rs.sampler_bindings     = sb;
    g_ocean_state = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs);
    if (!g_ocean_state) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateGPURenderState failed: %s", SDL_GetError());
    }

    /**
     * 4) Initialiser les uniforms
     */
    if (!SDL_SetGPURenderStateFragmentUniforms(g_ocean_state, 0, &g_ocean_u, sizeof(g_ocean_u))) {
        RC2D_log(RC2D_LOG_ERROR, "Set uniforms failed: %s", SDL_GetError());
    }

    /**
     * 5) Charger l’atlas de textures
     */
    g_elite27_atlas = rc2d_tp_loadAtlasFromStorage("assets/atlas/elite24/elite24.json", RC2D_STORAGE_TITLE);
    if (!g_elite27_atlas.atlas_image.sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load elite27 atlas");
    }
}

/* ========================================================================= */
/*                                UPDATE                                     */
/* ========================================================================= */
void rc2d_update(double dt)
{
    int lw = 0, lh = 0; SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &lw, &lh, &mode);
    SDL_FRect dst = { 0.0f, 0.0f, (float)lw, (float)lh };

    Ocean_UpdateUniforms(rc2d_engine_state.renderer, lw, lh, dt);

    // Update scene
}

/* ========================================================================= */
/*                                 DRAW                                      */
/* ========================================================================= */
void rc2d_draw(void)
{
    int lw = 0, lh = 0; SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &lw, &lh, &mode);
    SDL_FRect dst = { 0.0f, 0.0f, (float)lw, (float)lh };

    // DRAW SCENE
    if (tile_ocean_image.sdl_texture && g_ocean_state) 
    {
        // 1) activer l’état GPU custom
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, g_ocean_state);

        // 2) dessiner ta texture (le renderer lie automatiquement t0/s0 à cette texture)
        SDL_RenderTexture(rc2d_engine_state.renderer, tile_ocean_image.sdl_texture, NULL, &dst);

        // 3) désactiver l’état pour le reste du HUD
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, NULL);
    }
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