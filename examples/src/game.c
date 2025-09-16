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
static RC2D_Image          tileCausticImage = {0};
static RC2D_GPUShader*     g_ocean_fragment_shader = NULL;
static SDL_GPURenderState* g_ocean_state = NULL;
static OceanUniforms       g_ocean_u     = {0};
static double              g_time_accum  = 0.0;

/* ========================================================================= */
/*                              RESSOURCES                                   */
/* ========================================================================= */
static RC2D_Video g_login_bg_video; /* background-login.mp4 (silencieuse idéalement) */

/* === TexturePacker: elite27 === */
static RC2D_TP_Atlas g_elite27_atlas = {0};

static const char* s_elite27_names[] = {
    "1.png","2.png","3.png","4.png","5.png","6.png","7.png","8.png"
};
static const int   s_elite27_count = 8;
static double      s_elite27_time  = 0.0;
static float       s_elite27_fps   = 2.0f;   /* vitesse de preview */
static float       s_elite27_x     = 50.0f;   /* position d’affichage */
static float       s_elite27_y     = 50.0f;

/* ========================================================================= */
/*                              RESSOURCES UI                                */
/* ========================================================================= */
static RC2D_UIImage g_logo_ui       = {0};
static RC2D_UIImage g_input_email_ui = {0};
static RC2D_UIImage g_input_pass_ui  = {0};
static RC2D_UIImage g_button_login_ui = {0};

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
/*                              RESSOURCES AUDIO                             */
/* ========================================================================= */
static MIX_Audio* g_menu_music  = NULL;   // assets/sounds/sound_menu.mp3
static MIX_Track* g_menu_track  = NULL;   // piste de lecture
static bool       g_menu_started = false; // évite de rejouer chaque frame

/* ========================================================================= */
/*                       FADE SCENE LOGIN SCREEN                             */
/* ========================================================================= */
static float g_login_fade_alpha = 1.0f; // commence noir opaque (1.0 = 100% noir)
static const float g_login_fade_speed = 0.5f; // vitesse du fade (alpha/sec)

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
    rc2d_video_close(&g_splash_studio);
    rc2d_video_close(&g_splash_game);
    rc2d_video_close(&g_login_bg_video);

    rc2d_graphics_freeImage(&tile_ocean_image);
    rc2d_graphics_freeImage(&g_logo_ui.image);
    rc2d_graphics_freeImageData(&g_logo_ui.imageData);
    rc2d_graphics_freeImage(&g_input_email_ui.image);
    rc2d_graphics_freeImageData(&g_input_email_ui.imageData);
    rc2d_graphics_freeImage(&g_input_pass_ui.image);
    rc2d_graphics_freeImageData(&g_input_pass_ui.imageData);
    rc2d_graphics_freeImage(&g_button_login_ui.image);
    rc2d_graphics_freeImageData(&g_button_login_ui.imageData);
    rc2d_graphics_freeImage(&tileCausticImage);

    rc2d_tp_freeAtlas(&g_elite27_atlas);

    // Audio
    if (g_menu_track) 
    {
        rc2d_track_stop(g_menu_track);
        rc2d_track_destroy(g_menu_track);
        g_menu_track = NULL;
    }
    if (g_menu_music) 
    {
        rc2d_audio_destroy(g_menu_music);
        g_menu_music = NULL;
    }
    g_menu_started = false;

    if (g_ocean_fragment_shader) 
    {
        SDL_ReleaseGPUShader(rc2d_engine_state.gpu_device, (SDL_GPUShader*)g_ocean_fragment_shader);
        g_ocean_fragment_shader = NULL;
    }

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

    rc2d_window_setSize(1280, 720);
    //rc2d_window_setFullscreen(true, RC2D_FULLSCREEN_EXCLUSIVE, true);

    // --- Logo ---
    g_logo_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/logost-login.png", RC2D_STORAGE_TITLE);
    g_logo_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/logost-login.png", RC2D_STORAGE_TITLE);
    g_logo_ui.anchor      = RC2D_UI_ANCHOR_TOP_CENTER;
    g_logo_ui.margin_mode = RC2D_UI_MARGIN_PERCENT;
    g_logo_ui.margin_x    = 0.0f;
    g_logo_ui.margin_y    = 0.005f; // marge depuis le haut
    g_logo_ui.visible     = true;
    g_logo_ui.hittable    = false;

    // --- Input Email ---
    g_input_email_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/input-email-login.png", RC2D_STORAGE_TITLE);
    g_input_email_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/input-email-login.png", RC2D_STORAGE_TITLE);
    g_input_email_ui.anchor      = RC2D_UI_ANCHOR_TOP_CENTER;
    g_input_email_ui.margin_mode = RC2D_UI_MARGIN_PERCENT;
    g_input_email_ui.margin_x    = 0.0f;
    g_input_email_ui.margin_y    = 0.45f; // placé au-dessus du centre
    g_input_email_ui.visible     = true;
    g_input_email_ui.hittable    = true;

    // --- Input password ---
    g_input_pass_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/input-password-login.png", RC2D_STORAGE_TITLE);
    g_input_pass_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/input-password-login.png", RC2D_STORAGE_TITLE);
    g_input_pass_ui.anchor      = RC2D_UI_ANCHOR_TOP_CENTER;
    g_input_pass_ui.margin_mode = RC2D_UI_MARGIN_PERCENT;
    g_input_pass_ui.margin_x    = 0.f;
    g_input_pass_ui.margin_y    = 0.55f; // juste en dessous du champ email
    g_input_pass_ui.visible     = true;
    g_input_pass_ui.hittable    = true;

    // --- Bouton login (clé) ---
    g_button_login_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/button-login.png", RC2D_STORAGE_TITLE);
    g_button_login_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/button-login.png", RC2D_STORAGE_TITLE);
    g_button_login_ui.anchor      = RC2D_UI_ANCHOR_TOP_CENTER;
    g_button_login_ui.margin_mode = RC2D_UI_MARGIN_PERCENT;
    g_button_login_ui.margin_x    = 0.f;
    g_button_login_ui.margin_y    = 0.65f; // encore en dessous
    g_button_login_ui.visible     = true;
    g_button_login_ui.hittable    = true;

    /**
     * Sounds
     */
    g_menu_music = rc2d_audio_loadAudioFromStorage("assets/sounds/sound_menu.mp3", RC2D_STORAGE_TITLE, /*predecode=*/true);
    if (!g_menu_music) 
    {
        RC2D_log(RC2D_LOG_WARN, "Failed to load menu music: %s", SDL_GetError());
    } 
    else 
    {
        g_menu_track = rc2d_track_create();
        if (!g_menu_track) 
        {
            RC2D_log(RC2D_LOG_WARN, "Failed to create audio track: %s", SDL_GetError());
        } 
        else 
        {
            if (!rc2d_track_setAudio(g_menu_track, g_menu_music)) 
            {
                RC2D_log(RC2D_LOG_WARN, "track_setAudio failed: %s", SDL_GetError());
            }
        }
    }
    g_menu_started = false;

    // Charger le shader depuis le stockage
    g_ocean_fragment_shader = rc2d_gpu_loadGraphicsShaderFromStorage("water.fragment", RC2D_STORAGE_TITLE);
    if (!g_ocean_fragment_shader) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ocean shader from assets/water.fragment: %s", SDL_GetError());
        return;
    }

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
     * 3) Loader l'image de la texture de la tile de caustics, puis récupérer la texture GPU
     */
    tileCausticImage = rc2d_graphics_loadImageFromStorage("assets/images/tile-caustic.png", RC2D_STORAGE_TITLE);
    props = SDL_GetTextureProperties(tileCausticImage.sdl_texture);
    if (!props) 
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_GetTextureProperties failed: %s", SDL_GetError());
        return;
    }
    SDL_GPUTexture* textureGPUCaustic = (SDL_GPUTexture*)SDL_GetPointerProperty(
        props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL
    );
    if (!textureGPUCaustic) 
    {
        RC2D_log(RC2D_LOG_ERROR, "(2)No SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER on this texture");
        return;
    }

    
    SDL_GPUTextureSamplerBinding sb[1] = {0};
    sb[0].texture = textureGPUWater;
    sb[0].sampler = repeatSampler;
    
    /*SDL_GPUTextureSamplerBinding sb[2] = {0};
    sb[0].texture = textureGPUWater;
    sb[0].sampler = repeatSampler;
    sb[1].texture = textureGPUCaustic;
    sb[1].sampler = repeatSampler;*/

    SDL_GPURenderStateCreateInfo rs = {0};
    rs.fragment_shader      = g_ocean_fragment_shader;
    rs.num_sampler_bindings = 1;              // <-- plus que 1
    rs.sampler_bindings     = sb;
    g_ocean_state = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs);
    if (!g_ocean_state) {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateGPURenderState failed: %s", SDL_GetError());
    }

    // Premier upload d’uniforms -> slot 0 (b0, space3)
    if (!SDL_SetGPURenderStateFragmentUniforms(g_ocean_state, 0, &g_ocean_u, sizeof(g_ocean_u))) {
        RC2D_log(RC2D_LOG_ERROR, "Set uniforms failed: %s", SDL_GetError());
    }


    // Charger l’atlas de textures
    g_elite27_atlas = rc2d_tp_loadAtlasFromStorage("assets/atlas/elite24/elite24.json", RC2D_STORAGE_TITLE);
    if (!g_elite27_atlas.atlas_image.sdl_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load elite27 atlas");
    }


    // Initialiser l'état des splashes sans charger les vidéos
    g_splash_state = SPLASH_STUDIO;
    g_splash_active = true;
}

/* ========================================================================= */
/*                                UPDATE                                     */
/* ========================================================================= */
void rc2d_update(double dt)
{
    int out_w, out_h;
    SDL_GetCurrentRenderOutputSize(rc2d_engine_state.renderer, &out_w, &out_h);
    SDL_FRect dst = {0.0f, 0.0f, (float)out_w, (float)out_h};

    s_elite27_time += dt;
    
    Ocean_UpdateUniforms(rc2d_engine_state.renderer, out_w, out_h, dt);

    if (!g_splash_active) 
    {
        /* On décrémente l’alpha du fade jusqu’à 0 */
        if (g_login_fade_alpha > 0.0f) 
        {
            g_login_fade_alpha -= (float)(g_login_fade_speed * dt);
            if (g_login_fade_alpha < 0.0f)
                g_login_fade_alpha = 0.0f;
        }

        // Démarrer la musique une seule fois quand on est sur l’écran login
        if (!g_menu_started && g_menu_track && g_menu_music) 
        {
            if (!rc2d_track_play(g_menu_track, -1)) 
            { // -1 = boucle infinie
                RC2D_log(RC2D_LOG_WARN, "track_play failed: %s", SDL_GetError());
            } 
            else 
            {
                g_menu_started = true;
            }
        }

        /* update de la vidéo de fond si chargée */
        if (!g_login_bg_video.format_ctx) 
        {
            if (rc2d_video_openFromStorage(&g_login_bg_video, "assets/videos/background-login.mp4", RC2D_STORAGE_TITLE) != 0) 
            {
                RC2D_log(RC2D_LOG_ERROR, "Studio splash failed to open, skipping to game splash");
                return;
            }

            rc2d_video_setLoop(&g_login_bg_video, 1); // boucle infinie
            RC2D_log(RC2D_LOG_INFO, "Background login video opened successfully");
        }
        else 
        {
            rc2d_video_update(&g_login_bg_video, dt);
        }

        return;
    }

    switch (g_splash_state) {
        case SPLASH_STUDIO: {
            // Charger la première vidéo si elle n'est pas encore ouverte
            if (!g_splash_studio.format_ctx) {
                RC2D_log(RC2D_LOG_INFO, "Loading studio splash video in update");
                if (rc2d_video_openFromStorage(&g_splash_studio, "assets/videos/SplashScreen_Studio_1080p.mp4", RC2D_STORAGE_TITLE) != 0) {
                    RC2D_log(RC2D_LOG_ERROR, "Studio splash failed to open, skipping to game splash");
                    g_splash_state = SPLASH_GAME;
                    return;
                }
                RC2D_log(RC2D_LOG_INFO, "Studio splash opened successfully");
            }

            int r = rc2d_video_update(&g_splash_studio, dt);
            if (r <= 0) 
            {
                // Charger la deuxième vidéo
                RC2D_log(RC2D_LOG_INFO, "Loading game splash video");
                if (rc2d_video_openFromStorage(&g_splash_game, "assets/videos/SplashScreen_SeaTyrants_1080p.mp4", RC2D_STORAGE_TITLE) != 0) {
                    RC2D_log(RC2D_LOG_WARN, "Game splash failed, skipping to game.");
                    rc2d_video_close(&g_splash_studio);
                    g_splash_state = SPLASH_DONE;
                    g_splash_active = false;
                    return;
                }

                // Fermer la première vidéo et passer à la suivante
                rc2d_video_close(&g_splash_studio);
                g_splash_state = SPLASH_GAME;
                RC2D_log(RC2D_LOG_INFO, "Transitioned to game splash");
            }
        } break;

        case SPLASH_GAME: {
            // Charger la deuxième vidéo si elle n'est pas encore ouverte
            if (!g_splash_game.format_ctx) {
                RC2D_log(RC2D_LOG_INFO, "Loading game splash video in update");
                if (rc2d_video_openFromStorage(&g_splash_game, "assets/videos/SplashScreen_SeaTyrants_1080p.mp4", RC2D_STORAGE_TITLE) != 0) {
                    RC2D_log(RC2D_LOG_WARN, "Game splash failed, skipping to game.");
                    g_splash_state = SPLASH_DONE;
                    g_splash_active = false;
                    return;
                }
                RC2D_log(RC2D_LOG_INFO, "Game splash opened successfully");
            }

            int r = rc2d_video_update(&g_splash_game, dt);
            if (r <= 0) 
            {
                g_splash_state = SPLASH_DONE;
                g_splash_active = false;
                rc2d_video_close(&g_splash_game);
                RC2D_log(RC2D_LOG_INFO, "Both splash videos finished, switching to game");
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
    int lw = 0, lh = 0; SDL_RendererLogicalPresentation mode;
    SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &lw, &lh, &mode);
    SDL_FRect dst = { 0.0f, 0.0f, (float)lw, (float)lh };

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
        /* --- Rendu du jeu --- */
        rc2d_video_draw(&g_login_bg_video);

        // UI LOGIN
        rc2d_ui_drawImage(&g_logo_ui);
        rc2d_ui_drawImage(&g_input_email_ui);
        rc2d_ui_drawImage(&g_input_pass_ui);
        rc2d_ui_drawImage(&g_button_login_ui);

        // --- Dessiner le fade noir au-dessus si alpha > 0 ---
        if (g_login_fade_alpha > 0.0f) 
        {
            SDL_SetRenderDrawBlendMode(rc2d_engine_state.renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 0, 0, 0, (Uint8)(g_login_fade_alpha * 255));
            SDL_FRect full = {0, 0, (float)lw, (float)lh};
            SDL_RenderFillRect(rc2d_engine_state.renderer, &full);
            SDL_SetRenderDrawBlendMode(rc2d_engine_state.renderer, SDL_BLENDMODE_NONE);
        }
    }

    if (tile_ocean_image.sdl_texture && g_ocean_state) 
    {
        // 1) activer l’état GPU custom
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, g_ocean_state);

        // 2) dessiner ta texture (le renderer lie automatiquement t0/s0 à cette texture)
        SDL_RenderTexture(rc2d_engine_state.renderer, tile_ocean_image.sdl_texture, NULL, &dst);

        // 3) désactiver l’état pour le reste du HUD
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, NULL);
    }

    /* ================== PREVIEW TexturePacker elite27 ================== */
    if (g_elite27_atlas.atlas_image.sdl_texture) 
    {
        /* index courant à s_elite27_fps */
        int idx = (int)(s_elite27_time * (double)s_elite27_fps) % s_elite27_count;
        const char* fname = s_elite27_names[idx];

        /* dessine la sous-image RAW à (x,y) (aucun spriteSourceSize appliqué) */
        rc2d_tp_drawFrameByName(&g_elite27_atlas, fname,
                                s_elite27_x, s_elite27_y,
                                /*angle*/ 0.0,
                                /*scale*/ 1.0f, 1.0f,
                                /*offset*/ -1.0f, -1.0f,
                                /*flip*/ false, false);

        /* récupérer la frame pour connaitre w/h, tracer un cadre rouge de debug */
        const RC2D_TP_Frame* f = rc2d_tp_getFrame(&g_elite27_atlas, fname);
        if (f)
        {
            /* cadre = zone blit réelle (RAW) */
            SDL_FRect r = { s_elite27_x, s_elite27_y, f->frame.w, f->frame.h };
        }
    }
}

/* ========================================================================= */
/*                              MOUSE INPUT                                  */
/* ========================================================================= */
void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n", x, y, button, clicks, mouseID);

    if (button == RC2D_MOUSE_BUTTON_LEFT) 
    {
        if (rc2d_collision_pointInUIImagePixelPerfect(&g_input_email_ui, x, y)) {
            RC2D_log(RC2D_LOG_INFO, "Clicked in EMAIL input box\n");
            // TODO: focus login
        }
        else if (rc2d_collision_pointInUIImagePixelPerfect(&g_input_pass_ui, x, y)) {
            RC2D_log(RC2D_LOG_INFO, "Clicked in PASSWORD input box\n");
            // TODO: focus password
        }
        else if (rc2d_collision_pointInUIImagePixelPerfect(&g_button_login_ui, x, y)) {
            RC2D_log(RC2D_LOG_INFO, "Clicked LOGIN CONNEXION button!\n");
            // TODO: trigger login
        }
    }
}