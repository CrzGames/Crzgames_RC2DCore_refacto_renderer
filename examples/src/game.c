#include <mygame/game.h>

#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>

/* ========================================================================= */
/*                              RESSOURCES                                   */
/* ========================================================================= */

static SDL_GPURenderState* g_ocean_render_state   = NULL;
static RC2D_GPUShader*     g_ocean_fragment_shader = NULL;
static SDL_GPUSampler*     g_ocean_sampler        = NULL;
static RC2D_Image          tile_ocean_image = {0};
static RC2D_Image          background_login_image = {0};

/* ========================================================================= */
/*                           RESSOURCES TEXTE / FONT                         */
/* ========================================================================= */

static RC2D_Font g_ui_font = {0};
static RC2D_Text g_title_text = {0};
static RC2D_Text g_hint_text  = {0};
static int g_title_w = 0, g_title_h = 0;


/* ========================================================================= */
/*                              RESSOURCES UI                                */
/* ========================================================================= */

static RC2D_UIImage g_logo_ui       = {0};
static RC2D_UIImage g_input_login_ui = {0};
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

/* ========================================================================= */
/*                                UNLOAD                                     */
/* ========================================================================= */

void rc2d_unload(void)
{
    rc2d_video_close(&g_splash_studio);
    rc2d_video_close(&g_splash_game);

    rc2d_graphics_freeImage(&background_login_image);
    rc2d_graphics_freeImage(&tile_ocean_image);
    rc2d_graphics_freeImage(&g_logo_ui.image);
    rc2d_graphics_freeImageData(&g_logo_ui.imageData);
    rc2d_graphics_freeImage(&g_input_login_ui.image);
    rc2d_graphics_freeImageData(&g_input_login_ui.imageData);
    rc2d_graphics_freeImage(&g_input_pass_ui.image);
    rc2d_graphics_freeImageData(&g_input_pass_ui.imageData);
    rc2d_graphics_freeImage(&g_button_login_ui.image);
    rc2d_graphics_freeImageData(&g_button_login_ui.imageData);

    rc2d_graphics_closeFont(&g_ui_font);

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

    rc2d_window_setSize(1280, 720);
    //rc2d_window_setFullscreen(true, RC2D_FULLSCREEN_EXCLUSIVE, true);

    const char *base_path = SDL_GetBasePath();
    char full_path[512];

    // splash videos
    SDL_snprintf(full_path, sizeof(full_path), "%sassets/videos/SplashScreen_Studio_1080p.mp4", base_path);
    if (rc2d_video_open(&g_splash_studio, full_path) != 0) 
    {
        RC2D_log(RC2D_LOG_WARN, "Studio splash failed to open, skipping directly to game splash.");
    } 
    else 
    {
        g_splash_state  = SPLASH_STUDIO;
        g_splash_active = true;
    }

    // --- Logo ---
    g_logo_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/logost-login.png", RC2D_STORAGE_TITLE);
    g_logo_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/logost-login.png", RC2D_STORAGE_TITLE);
    g_logo_ui.anchor      = RC2D_UI_ANCHOR_TOP_CENTER;
    g_logo_ui.margin_mode = RC2D_UI_MARGIN_PIXELS;
    g_logo_ui.margin_x    = 0.f;
    g_logo_ui.margin_y    = 40.f; // marge depuis le haut
    g_logo_ui.visible     = true;
    g_logo_ui.hittable    = false;

    // --- Input login ---
    g_input_login_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/input-login.png", RC2D_STORAGE_TITLE);
    g_input_login_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/input-login.png", RC2D_STORAGE_TITLE);
    g_input_login_ui.anchor      = RC2D_UI_ANCHOR_CENTER;
    g_input_login_ui.margin_mode = RC2D_UI_MARGIN_PIXELS;
    g_input_login_ui.margin_x    = 0.f;
    g_input_login_ui.margin_y    = -40.f; // placé au-dessus du centre
    g_input_login_ui.visible     = true;
    g_input_login_ui.hittable    = true;

    // --- Input password ---
    g_input_pass_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/input-login.png", RC2D_STORAGE_TITLE);
    g_input_pass_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/input-login.png", RC2D_STORAGE_TITLE);
    g_input_pass_ui.anchor      = RC2D_UI_ANCHOR_CENTER;
    g_input_pass_ui.margin_mode = RC2D_UI_MARGIN_PIXELS;
    g_input_pass_ui.margin_x    = 0.f;
    g_input_pass_ui.margin_y    = +50.f; // juste en dessous du champ login
    g_input_pass_ui.visible     = true;
    g_input_pass_ui.hittable    = true;

    // --- Bouton login (clé) ---
    g_button_login_ui.image     = rc2d_graphics_loadImageFromStorage("assets/images/button-login.png", RC2D_STORAGE_TITLE);
    g_button_login_ui.imageData = rc2d_graphics_loadImageDataFromStorage("assets/images/button-login.png", RC2D_STORAGE_TITLE);
    g_button_login_ui.anchor      = RC2D_UI_ANCHOR_CENTER;
    g_button_login_ui.margin_mode = RC2D_UI_MARGIN_PIXELS;
    g_button_login_ui.margin_x    = 0.f;
    g_button_login_ui.margin_y    = -10.f; // encore en dessous
    g_button_login_ui.visible     = true;
    g_button_login_ui.hittable    = true;

    /**
     * Sounds
    */
    g_menu_music = rc2d_audio_load("assets/sounds/sound_menu.mp3", /*predecode=*/true);
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

    // Ouvrir une police depuis le storage (ex: assets/fonts/Inter-Regular.ttf)
    g_ui_font = rc2d_graphics_openFontFromStorage("assets/fonts/SIXTY.ttf",
                                                  RC2D_STORAGE_TITLE,
                                                  32.0f /* fontSize initial */);
    if (!g_ui_font.sdl_font) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to open font, text rendering disabled.\n");
    } 
    else 
    {
        /* Configurer la police via les champs de RC2D_Font (struct-driven) */
        g_ui_font.style     = TTF_STYLE_BOLD;                 /* ex: bold */
        g_ui_font.alignment = TTF_HORIZONTAL_ALIGN_LEFT;      /* wrap: gauche */

        /* Appliquer ces réglages à la police SDL_ttf */
        if (!rc2d_graphics_setFontSize(&g_ui_font)) 
        {
            RC2D_log(RC2D_LOG_WARN, "rc2d_graphics_setFontSize failed.\n");
        }
        rc2d_graphics_setFontStyle(&g_ui_font);
        rc2d_graphics_setFontWrapAlignment(&g_ui_font);

        /* 3) Créer des textes persistants */
        g_title_text = rc2d_graphics_createText(&g_ui_font, "Sea Tyrants");
        if (!g_title_text.sdl_text) {
            RC2D_log(RC2D_LOG_ERROR, "createText(title) failed.\n");
        } else {
            /* Couleur via le champ color, puis appliquer */
            g_title_text.color = (RC2D_Color){255, 255, 255, 255};
            if (!rc2d_graphics_setTextColor(&g_title_text)) {
                RC2D_log(RC2D_LOG_WARN, "setTextColor(title) failed.\n");
            }
            /* Mesure de ce texte persistant */
            if (!rc2d_graphics_getTextSize(&g_title_text, &g_title_w, &g_title_h)) {
                RC2D_log(RC2D_LOG_WARN, "getTextSize(title) failed.\n");
            }
        }

        g_hint_text = rc2d_graphics_createText(&g_ui_font, "Tap to start — press Enter");
        if (!g_hint_text.sdl_text) {
            RC2D_log(RC2D_LOG_ERROR, "createText(hint) failed.\n");
        } else {
            g_hint_text.color = (RC2D_Color){255, 220, 120, 255};
            if (!rc2d_graphics_setTextColor(&g_hint_text)) {
                RC2D_log(RC2D_LOG_WARN, "setTextColor(hint) failed.\n");
            }
            /* Exemple de wrap: largeur max 600px */
            if (!rc2d_graphics_setTextWrapWidth(&g_hint_text, 600)) {
                RC2D_log(RC2D_LOG_WARN, "setTextWrapWidth(hint) failed.\n");
            }

            /* Exemple de MAJ de string “struct-driven” :
                - on remplace la chaîne en changeant text.string
                - puis on applique via rc2d_graphics_setTextString(&text) */
            g_hint_text.string = "Click LOGIN to continue";
            if (!rc2d_graphics_setTextString(&g_hint_text)) {
                RC2D_log(RC2D_LOG_WARN, "setTextString(hint) failed.\n");
            }
        }

        /* Exemple de mesure ad-hoc d’une chaîne arbitraire via la police (sans TTF_Text) */
        int w=0, h=0;
        if (!rc2d_graphics_getStringSize(&g_ui_font, "Measured with font", 18, &w, &h)) {
            RC2D_log(RC2D_LOG_WARN, "getStringSize(font,\"Measured with font\") failed.\n");
        } else {
            RC2D_log(RC2D_LOG_INFO, "Measure sample: %dx%d\n", w, h);
        }
    }

    // background login
    background_login_image = rc2d_graphics_loadImageFromStorage("assets/images/background-login.png", RC2D_STORAGE_TITLE);

    // tile ocean
    tile_ocean_image = rc2d_graphics_loadImageFromStorage("assets/images/tile.png", RC2D_STORAGE_TITLE);

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

        // ici update gameplay ou UI logique si besoin
        return;
    }

    switch (g_splash_state) {
        case SPLASH_STUDIO: {
            int r = rc2d_video_update(&g_splash_studio, dt);
            if (r <= 0) 
            {
                /* Ouverture de la 2e vidéo immédiatement après la fin */
                const char *base_path = SDL_GetBasePath();
                char full_path[512];
                SDL_snprintf(full_path, sizeof(full_path), "%sassets/videos/SplashScreen_SeaTyrants_1080p.mp4", base_path);

                if (rc2d_video_open(&g_splash_game, full_path) != 0) 
                {
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
        /* --- Rendu du jeu --- */
        if (background_login_image.sdl_texture) 
        {
            // BACKGROUND
            int lw = 0, lh = 0; SDL_RendererLogicalPresentation mode;
            SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &lw, &lh, &mode);

            SDL_FRect dst = { 0.0f, 0.0f, (float)lw, (float)lh };
            /* src = NULL -> texture entière ; dst = plein cadre logique */
            SDL_RenderTexture(rc2d_engine_state.renderer, background_login_image.sdl_texture, NULL, &dst);


            // UI
            rc2d_ui_drawImage(&g_logo_ui);
            rc2d_ui_drawImage(&g_input_login_ui);
            rc2d_ui_drawImage(&g_input_pass_ui);
            rc2d_ui_drawImage(&g_button_login_ui);


            // TEXT
            if (g_title_text.sdl_text) {
                /* Centrer approximativement le titre en haut */
                float tx = (lw - (float)g_title_w) * 0.5f;
                float ty = 20.0f;
                rc2d_graphics_drawText(&g_title_text, tx, ty);
            }
            if (g_hint_text.sdl_text) {
                /* Poser l’aide sous le centre */
                float hx = 40.0f;
                float hy = (float)lh * 0.65f;
                rc2d_graphics_drawText(&g_hint_text, hx, hy);
            }


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
    }
}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID)
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n", x, y, button, clicks, mouseID);

    if (button == RC2D_MOUSE_BUTTON_LEFT) 
    {
        if (rc2d_collision_pointInUIImagePixelPerfect(&g_input_login_ui, x, y)) {
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