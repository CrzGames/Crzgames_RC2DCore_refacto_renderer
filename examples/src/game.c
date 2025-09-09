#include <mygame/game.h>

#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_video.h>
#include <RC2D/RC2D_logger.h>

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>

#ifndef AV_TIME_BASE
#include <libavutil/avutil.h>
#endif

/* ========================================================================= */
/*                              RESSOURCES                                   */
/* ========================================================================= */

static SDL_Texture*        g_ocean_tile_texture   = NULL;
static SDL_Texture*        g_ocean_navire_texture = NULL;
static SDL_GPURenderState* g_ocean_render_state   = NULL;
static RC2D_GPUShader*     g_ocean_fragment_shader = NULL;
static SDL_GPUSampler*     g_ocean_sampler        = NULL;

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

/* Récupère la durée totale (secondes) depuis FFmpeg, ou <=0 si inconnue */
static inline double video_total_seconds(const RC2D_Video* v)
{
    if (!v || !v->format_ctx) return -1.0;
    if (v->format_ctx->duration <= 0 || v->format_ctx->duration == AV_NOPTS_VALUE)
        return -1.0;
    return (double)v->format_ctx->duration / (double)AV_TIME_BASE;
}

/* Temps courant (secondes) — on utilise l’horloge du lecteur */
static inline double video_current_seconds(const RC2D_Video* v)
{
    if (!v) return 0.0;
    return (v->clock_time < 0.0) ? 0.0 : v->clock_time;
}

/* ========================================================================= */
/*                                UNLOAD                                     */
/* ========================================================================= */

void rc2d_unload(void)
{
    rc2d_video_close(&g_splash_studio);
    rc2d_video_close(&g_splash_game);

    if (g_ocean_render_state) {
        SDL_DestroyGPURenderState(g_ocean_render_state);
        g_ocean_render_state = NULL;
    }
    if (g_ocean_fragment_shader) {
        SDL_ReleaseGPUShader(rc2d_engine_state.gpu_device, (SDL_GPUShader*)g_ocean_fragment_shader);
        g_ocean_fragment_shader = NULL;
    }
    if (g_ocean_tile_texture) {
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
    }
    if (g_ocean_navire_texture) {
        SDL_DestroyTexture(g_ocean_navire_texture);
        g_ocean_navire_texture = NULL;
    }
    if (g_ocean_sampler) {
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

    rc2d_window_setFullscreen(true, RC2D_FULLSCREEN_EXCLUSIVE, true);

    const char *base_path = SDL_GetBasePath();
    char full_path[512];

    /* 1) Ouvrir la vidéo du Studio */
    SDL_snprintf(full_path, sizeof(full_path), "%sSplashScreen_Studio_1080p.mp4", base_path);
    if (rc2d_video_open(&g_splash_studio, full_path) != 0) {
        RC2D_log(RC2D_LOG_WARN, "Studio splash failed to open, skipping directly to game splash.");
        /* Tente d'ouvrir la 2e directement */
        SDL_snprintf(full_path, sizeof(full_path), "%sSplashScreen_SeaTyrants_1080p.mp4", base_path);
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

    /* 2) Ressources jeu (inchangé) */
    SDL_snprintf(full_path, sizeof(full_path), "%stile.png", base_path);
    g_ocean_tile_texture = IMG_LoadTexture(rc2d_engine_state.renderer, full_path);
    if (!g_ocean_tile_texture) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load tile.png: %s", SDL_GetError());
        return;
    }

    SDL_snprintf(full_path, sizeof(full_path), "%snavire.png", base_path);
    g_ocean_navire_texture = IMG_LoadTexture(rc2d_engine_state.renderer, full_path);
    if (!g_ocean_navire_texture) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load navire.png: %s", SDL_GetError());
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        return;
    }

    // Sound
    /*SDL_snprintf(full_path, sizeof(full_path), "%ssound1.mp3", base_path);
    MIX_Audio* audio = rc2d_audio_load(full_path, true);
    MIX_Track* track = rc2d_track_create();

    rc2d_track_setAudio(track, audio);
    rc2d_track_setGain(track, 1.0f);
    rc2d_track_play(track, 0);*/

    // Shader GPU
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
    if (!g_ocean_sampler) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to create sampler: %s", SDL_GetError());
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        SDL_DestroyTexture(g_ocean_navire_texture);
        g_ocean_navire_texture = NULL;
        return;
    }

    g_ocean_fragment_shader = rc2d_gpu_loadGraphicsShader("water.fragment");
    if (!g_ocean_fragment_shader) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load water.fragment shader: %s", SDL_GetError());
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        SDL_DestroyTexture(g_ocean_navire_texture);
        g_ocean_navire_texture = NULL;
        SDL_ReleaseGPUSampler(rc2d_engine_state.gpu_device, g_ocean_sampler);
        g_ocean_sampler = NULL;
        return;
    }

    SDL_GPUTextureSamplerBinding sampler_binding = {
        .texture = g_ocean_tile_texture,
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
    if (!g_ocean_render_state) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateGPURenderState failed: %s", SDL_GetError());
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        SDL_DestroyTexture(g_ocean_navire_texture);
        g_ocean_navire_texture = NULL;
        SDL_ReleaseGPUSampler(rc2d_engine_state.gpu_device, g_ocean_sampler);
        g_ocean_sampler = NULL;
        SDL_ReleaseGPUShader(rc2d_engine_state.gpu_device, (SDL_GPUShader*)g_ocean_fragment_shader);
        g_ocean_fragment_shader = NULL;
        return;
    }

    RC2D_log(RC2D_LOG_INFO, "Render state created successfully!");
}

/* ========================================================================= */
/*                                UPDATE                                     */
/* ========================================================================= */

void rc2d_update(double dt)
{
    if (!g_splash_active) {
        /* Update gameplay ici si besoin */
        return;
    }

    switch (g_splash_state) {
    case SPLASH_STUDIO: {
        int r = rc2d_video_update(&g_splash_studio, dt);
        if (r <= 0) {
            /* Ouverture de la 2e vidéo immédiatement après la fin */
            const char *base_path = SDL_GetBasePath();
            char full_path[512];
            SDL_snprintf(full_path, sizeof(full_path), "%sSplashScreen_SeaTyrants_1080p.mp4", base_path);

            if (rc2d_video_open(&g_splash_game, full_path) != 0) {
                RC2D_log(RC2D_LOG_WARN, "Game splash failed, skipping to game.");
                rc2d_video_close(&g_splash_studio);
                g_splash_state  = SPLASH_DONE;
                g_splash_active = false;
                return;
            }
            /* On peut fermer la 1re vidéo, on passe à la 2e (qui fera son fade-in 6s) */
            rc2d_video_close(&g_splash_studio);
            g_splash_state = SPLASH_GAME;
        }
    } break;

    case SPLASH_GAME: {
        int r = rc2d_video_update(&g_splash_game, dt);
        if (r <= 0) {
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
    if (g_splash_active) {
        if (g_splash_state == SPLASH_STUDIO) {
            /* 1) Dessiner la vidéo du studio */
            rc2d_video_draw(&g_splash_studio);

            /* 2) Calculer l’alpha du voile noir sur les 6 dernières secondes */
            const double total = video_total_seconds(&g_splash_studio);   /* total (s) */
            const double now   = video_current_seconds(&g_splash_studio); /* courant (s) */

            if (total > 0.0) {
                const double remaining = total - now;
                if (remaining <= g_fade_seconds) {
                    double a = 1.0 - (remaining / g_fade_seconds); /* 0->1 quand on approche de la fin */
                    draw_fullscreen_black_with_alpha(rc2d_engine_state.renderer, a);
                }
            }
        }
        else if (g_splash_state == SPLASH_GAME) {
            /* 1) Dessiner la vidéo du jeu */
            rc2d_video_draw(&g_splash_game);

            /* 2) Calculer l’alpha du voile noir sur les 6 premières secondes (fade-in) */
            const double now = video_current_seconds(&g_splash_game);
            if (now < g_fade_seconds) {
                double a = 1.0 - (now / g_fade_seconds); /* 1->0 sur 6s */
                draw_fullscreen_black_with_alpha(rc2d_engine_state.renderer, a);
            }
        }
        /* SPLASH_DONE n’affiche rien ici */
    } else {
        /* --- Rendu jeu (exemple simple) --- */
        float texW, texH;
        SDL_GetTextureSize(g_ocean_navire_texture, &texW, &texH);
        SDL_FRect dstrect = { 0.0f, 0.0f, texW, texH };
        SDL_RenderTexture(rc2d_engine_state.renderer, g_ocean_navire_texture, NULL, &dstrect);
    }
}
