#include <mygame/game.h>

#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>

#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>

static SDL_Texture*        g_ocean_tile_texture  = NULL;
static SDL_Texture*        g_ocean_navire_texture = NULL;
static SDL_GPURenderState* g_ocean_render_state  = NULL;
static RC2D_GPUShader*     g_ocean_fragment_shader = NULL;
static SDL_GPUSampler*     g_ocean_sampler = NULL;
static RC2D_Video          g_splash_video;        // Contexte de la vidéo
static bool                g_splash_active = true; // Indique si la vidéo est en cours

void rc2d_unload(void)
{
    // Libérer les ressources de la vidéo
    rc2d_video_close(&g_splash_video);

    // Libérer les ressources existantes
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

void rc2d_load(void)
{
    RC2D_log(RC2D_LOG_INFO, "My game is loading...\n");

    SDL_SetRenderLogicalPresentation(rc2d_engine_state.renderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    const char *base_path = SDL_GetBasePath();
    char full_path[512];

    // 1) Charger la vidéo du splash screen
    SDL_snprintf(full_path, sizeof(full_path), "%SSplashScreen_Studio.mp4", base_path);
    if (rc2d_video_open(&g_splash_video, full_path, rc2d_engine_state.renderer) != 0) {
        g_splash_active = false; // Passe directement au jeu si la vidéo échoue
    }

    // 2) Charger la texture et le navire
    SDL_snprintf(full_path, sizeof(full_path), "%stile.png", base_path);
    g_ocean_tile_texture = IMG_LoadTexture(rc2d_engine_state.renderer, full_path);
    if (!g_ocean_tile_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load tile.png: %s", SDL_GetError());
        rc2d_video_close(&g_splash_video);
        return;
    }

    SDL_snprintf(full_path, sizeof(full_path), "%snavire.png", base_path);
    g_ocean_navire_texture = IMG_LoadTexture(rc2d_engine_state.renderer, full_path);
    if (!g_ocean_navire_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load navire.png: %s", SDL_GetError());
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        rc2d_video_close(&g_splash_video);
        return;
    }

    // 3) Créer un sampler minimal
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
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        SDL_DestroyTexture(g_ocean_navire_texture);
        g_ocean_navire_texture = NULL;
        rc2d_video_close(&g_splash_video);
        return;
    }

    // 4) Charger le shader de fragment
    g_ocean_fragment_shader = rc2d_gpu_loadGraphicsShader("water.fragment");
    if (!g_ocean_fragment_shader) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load water.fragment shader: %s", SDL_GetError());
        SDL_DestroyTexture(g_ocean_tile_texture);
        g_ocean_tile_texture = NULL;
        SDL_DestroyTexture(g_ocean_navire_texture);
        g_ocean_navire_texture = NULL;
        SDL_ReleaseGPUSampler(rc2d_engine_state.gpu_device, g_ocean_sampler);
        g_ocean_sampler = NULL;
        rc2d_video_close(&g_splash_video);
        return;
    }

    // 5) Créer le render state
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
        rc2d_video_close(&g_splash_video);
        return;
    }
    
    RC2D_log(RC2D_LOG_INFO, "Render state created successfully!");
}

void rc2d_update(double dt)
{
    if (g_splash_active) {
        // Mettre à jour la vidéo
        int result = rc2d_video_update(&g_splash_video, dt);
        if (result == 0 || result < 0) {
            // Vidéo terminée ou erreur : passer au jeu
            g_splash_active = false;
            rc2d_video_close(&g_splash_video);
            RC2D_log(RC2D_LOG_INFO, "Splash video finished or failed, switching to game\n");
        }
    } else {
        // Mettre à jour la logique du jeu ici (par exemple, position du navire)
    }
}

void rc2d_draw(void)
{
    if (g_splash_active) {
        // Dessiner la vidéo du splash screen
        rc2d_video_draw(&g_splash_video, rc2d_engine_state.renderer);
    } else {
        // Dessiner le navire (sans shader pour l'instant)
        float texW, texH;
        SDL_GetTextureSize(g_ocean_navire_texture, &texW, &texH);
        SDL_FRect dstrect = { 0.0f, 0.0f, texW, texH };
        SDL_RenderTexture(rc2d_engine_state.renderer, g_ocean_navire_texture, NULL, &dstrect);
    }
}