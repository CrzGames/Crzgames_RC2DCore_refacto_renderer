#include <mygame/game.h>

#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>

#ifndef AV_TIME_BASE
#include <libavutil/avutil.h>
#endif

/* ========================================================================= */
/*                              RESSOURCES                                   */
/* ========================================================================= */

static SDL_GPURenderState* g_ocean_render_state   = NULL;
static RC2D_GPUShader*     g_ocean_fragment_shader = NULL;
static SDL_GPUSampler*     g_ocean_sampler        = NULL;
static RC2D_Image          tile_ocean_image = {0};
static RC2D_Image          minimap_image = {0};
static RC2D_Image          background_login_image = {0};

/* ========================================================================= */
/*                            SPLASH SYSTEME                                 */
/* ========================================================================= */

static RC2D_Video          g_splash_studio;     /* 1ère vidéo : Studio */
static RC2D_Video          g_splash_game;       /* 2ème vidéo : Jeu    */
static bool                g_splash_active = true;

typedef enum SplashState {
    SPLASH_STUDIO = 0,   /* on joue la 1ère vidéo (fondu OUT 6s avant la fin) */
    SPLASH_GAME,         /* on joue la 2e vidéo   (fondu IN  sur 6s)         */
    SPLASH_DONE          /* terminé → jeu */
} SplashState;

static SplashState         g_splash_state = SPLASH_STUDIO;

/* Durée du fondu (secondes) */
static const double        g_fade_seconds = 1.5;

/* ========================================================================= */
/*                         HELPERS / PETITES UTILS                           */
/* ========================================================================= */

static inline double clamp01(double x)
{
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

/* alpha 0..1 vers 0..255, avec blend */
static inline void draw_fullscreen_black_with_alpha(SDL_Renderer* r, double a01)
{
    int w=0, h=0; SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(r, &w, &h, &mode);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)(clamp01(a01) * 255.0));
    SDL_FRect full = {0.0f, 0.0f, (float)w, (float)h};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ========================================================================= */
/*                                UNLOAD                                     */
/* ========================================================================= */

void rc2d_unload(void)
{
    rc2d_video_close(&g_splash_studio);
    rc2d_video_close(&g_splash_game);

    rc2d_graphics_freeImage(&minimap_image);
    rc2d_graphics_freeImage(&background_login_image);
    rc2d_graphics_freeImage(&tile_ocean_image);

    if (g_ocean_render_state) 
    {
        SDL_DestroyGPURenderState(g_ocean_render_state);
        g_ocean_render_state = NULL;
    }
    if (g_ocean_fragment_shader) 
    {
        SDL_ReleaseGPUShader(rc2d_engine_state.gpu_device, (SDL_GPUShader*)g_ocean_fragment_shader);
        g_ocean_fragment_shader = NULL;
    }
    if (g_ocean_sampler) 
    {
        SDL_ReleaseGPUSampler(rc2d_engine_state.gpu_device, g_ocean_sampler);
        g_ocean_sampler = NULL;
    }

    RC2D_log(RC2D_LOG_INFO, "My game is unloading...\n");
}

/* ========================================================================= */
/*                                 LOAD                                      */
/* ========================================================================= */

void rc2d_load(void)
{
    RC2D_log(RC2D_LOG_INFO, "My game is loading...\n");

    //rc2d_window_setSize(1280, 720);
    //rc2d_window_setFullscreen(true, RC2D_FULLSCREEN_EXCLUSIVE, true);

    const char *base_path = SDL_GetBasePath();
    char full_path[512];

    // splash videos
    SDL_snprintf(full_path, sizeof(full_path), "%sassets/videos/SplashScreen_Studio_1080p.mp4", base_path);
    if (rc2d_video_open(&g_splash_studio, full_path) != 0) {
        RC2D_log(RC2D_LOG_WARN, "Studio splash failed to open, skipping directly to game splash.");
        /* Tente d'ouvrir la 2e directement */
        SDL_snprintf(full_path, sizeof(full_path), "%sassets/videos/SplashScreen_SeaTyrants_1080p.mp4", base_path);
        if (rc2d_video_open(&g_splash_game, full_path) != 0) {
            RC2D_log(RC2D_LOG_WARN, "Game splash failed to open, skipping to gameplay.");
            g_splash_state  = SPLASH_DONE;
            g_splash_active = false;
        } else {
            g_splash_state  = SPLASH_GAME; /* on jouera un fade-in automatique sur 6s */
            g_splash_active = true;
        }
    } else {
        g_splash_state  = SPLASH_STUDIO;
        g_splash_active = true;
    }    

    // minimap
    minimap_image = rc2d_graphics_newImageFromStorage("assets/images/minimap.png", RC2D_STORAGE_TITLE);

    // background login
    background_login_image = rc2d_graphics_newImageFromStorage("assets/images/background-login.png", RC2D_STORAGE_TITLE);

    // tile ocean
    tile_ocean_image = rc2d_graphics_newImageFromStorage("assets/images/tile.png", RC2D_STORAGE_TITLE);

    // Load shader, sampler, render state
    SDL_GPUSamplerCreateInfo sampler_info = {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .props = 0
    };
    g_ocean_sampler = SDL_CreateGPUSampler(rc2d_engine_state.gpu_device, &sampler_info);
    if (!g_ocean_sampler) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to create sampler: %s", SDL_GetError());
        return;
    }

    g_ocean_fragment_shader = rc2d_gpu_loadGraphicsShader("water.fragment");
    if (!g_ocean_fragment_shader) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load water.fragment shader: %s", SDL_GetError());
        return;
    }

    SDL_GPUTextureSamplerBinding sampler_binding = {
        .texture = tile_ocean_image.sdl_texture,
        .sampler = g_ocean_sampler
    };
    SDL_GPURenderStateCreateInfo rs_info = {
        .fragment_shader = (SDL_GPUShader*)g_ocean_fragment_shader,
        .num_sampler_bindings = 1,
        .sampler_bindings = &sampler_binding,
        .num_storage_textures = 0,
        .storage_textures = NULL,
        .num_storage_buffers = 0,
        .storage_buffers = NULL,
        .props = 0
    };

    g_ocean_render_state = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs_info);
    if (!g_ocean_render_state) 
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateGPURenderState failed: %s", SDL_GetError());
        return;
    }

    RC2D_log(RC2D_LOG_INFO, "Render state created successfully!");
}

/* ========================================================================= */
/*                                UPDATE                                     */
/* ========================================================================= */

void rc2d_update(double dt)
{
    if (!g_splash_active) 
    {
        /* Update gameplay ici si besoin */
        return;
    }

    switch (g_splash_state) {
        case SPLASH_STUDIO: {
            int r = rc2d_video_update(&g_splash_studio, dt);
            if (r <= 0) 
            {
                /* On peut fermer la 1re vidéo, on passe à la 2e (qui fera son fade-in 6s) */
                rc2d_video_close(&g_splash_studio);
                g_splash_state = SPLASH_GAME;
            }
        } break;

        case SPLASH_GAME: {
            int r = rc2d_video_update(&g_splash_game, dt);
            if (r <= 0) 
            {
                g_splash_state  = SPLASH_DONE;
                g_splash_active = false;
                rc2d_video_close(&g_splash_game);
                RC2D_log(RC2D_LOG_INFO, "Both splash videos finished, switching to game\n");
            }
        } break;

        case SPLASH_DONE:
            /* Update gameplay ici */
            break;
    }
}

/* ========================================================================= */
/*                                 DRAW                                      */
/* ========================================================================= */

void rc2d_draw(void)
{
    if (g_splash_active) 
    {
        if (g_splash_state == SPLASH_STUDIO) 
        {
            /* 1) Dessiner la vidéo du studio */
            rc2d_video_draw(&g_splash_studio);

            /* 2) Calculer l’alpha du voile noir sur les 6 dernières secondes */
            const double total = rc2d_video_totalSeconds(&g_splash_studio);   /* total (s) */
            const double now   = rc2d_video_currentSeconds(&g_splash_studio); /* courant (s) */

            if (total > 0.0) 
            {
                const double remaining = total - now;
                if (remaining <= g_fade_seconds) 
                {
                    double a = 1.0 - (remaining / g_fade_seconds); /* 0->1 quand on approche de la fin */
                    draw_fullscreen_black_with_alpha(rc2d_engine_state.renderer, a);
                }
            }
        }
        else if (g_splash_state == SPLASH_GAME) 
        {
            /* 1) Dessiner la vidéo du jeu */
            rc2d_video_draw(&g_splash_game);

            /* 2) Calculer l’alpha du voile noir sur les 6 premières secondes (fade-in) */
            const double now = rc2d_video_currentSeconds(&g_splash_game);
            if (now < g_fade_seconds) 
            {
                double a = 1.0 - (now / g_fade_seconds); /* 1->0 sur 6s */
                draw_fullscreen_black_with_alpha(rc2d_engine_state.renderer, a);
            }
        }
    } 
    else 
    {
        /* --- Rendu jeu : fond de login plein écran logique --- */
        if (background_login_image.sdl_texture) 
        {
            int lw = 0, lh = 0; SDL_RendererLogicalPresentation mode;
            SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &lw, &lh, &mode);

            SDL_FRect dst = { 0.0f, 0.0f, (float)lw, (float)lh };
            /* src = NULL -> texture entière ; dst = plein cadre logique */
            SDL_RenderTexture(rc2d_engine_state.renderer, background_login_image.sdl_texture, NULL, &dst);
        }

        // minimap en bas-droite, marge 20 px logiques
        if (minimap_image.sdl_texture) {
            rc2d_ui_drawImageAnchoredPixels(
                minimap_image,
                RC2D_UI_ANCHOR_BOTTOM_RIGHT,
                20.0f,  // margin_x_pixels (depuis la droite)
                20.0f   // margin_y_pixels (depuis le bas)
            );
            rc2d_ui_drawImageAnchoredPercentage(
                minimap_image,
                RC2D_UI_ANCHOR_BOTTOM_RIGHT,
                0.20f,  // 20% de la largeur de la zone sûre depuis la droite
                0.20f   // 20% de la hauteur de la zone sûre depuis le bas
            );
        }
    }
}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n", x, y, button, clicks, mouseID);
}