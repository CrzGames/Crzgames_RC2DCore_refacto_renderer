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

void rc2d_unload(void)
{
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

    // 1) Charger la texture (tile.png à côté de l'exe)
    const char *base_path = SDL_GetBasePath();
    char full_path[512];
    SDL_snprintf(full_path, sizeof(full_path), "%stile.png", base_path);

    g_ocean_tile_texture = IMG_LoadTexture(rc2d_engine_state.renderer, full_path);
    if (!g_ocean_tile_texture) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load tile.png: %s", SDL_GetError());
        return;
    }

    // 2) Créer un sampler minimal
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
        return;
    }

    // 3) Charger le shader de fragment
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
        return;
    }

    // 4) Créer le render state
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
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateGPURenderState failed: %s (0x%x)", SDL_GetError());
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

void rc2d_update(double dt)
{

}

void rc2d_draw(void)
{
    // Activer le render state pour les prochains appels de dessin
    if (!SDL_SetRenderGPUState(rc2d_engine_state.renderer, g_ocean_render_state)) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to set render state: %s", SDL_GetError());
        return;
    }

    // Dessiner la texture avec le shader de fragment
    //SDL_RenderTexture(rc2d_engine_state.renderer, g_ocean_tile_texture, NULL, NULL);

    // Désactiver le render state
    SDL_SetRenderGPUState(rc2d_engine_state.renderer, NULL);
}