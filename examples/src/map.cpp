#include <mygame/map.h>

/* --- Presets de marges (constantes) --- */

// Preset #1 : Mode encadré — place pour minimap + chat + barres
const MapInsets Map::kInsetsFramed = {
    200.0f,  /* left   */
    50.0f,   /* top    */
    200.0f,  /* right  */
    80.0f,   /* bottom */
    false    /* interprétation en pixels logiques */
};

// Preset #2 : Mode barre supérieure — plein écran sauf une barre en haut
const MapInsets Map::kInsetsTopBar = {
    0.0f,    /* left   */
    50.0f,   /* top    */
    0.0f,    /* right  */
    0.0f,    /* bottom */
    false    /* interprétation en pixels logiques */
};

/* --- Méthodes privées --- */

MapInsets Map::GetInsetsForLayoutMode(MapLayoutMode mode) const 
{
    switch (mode) {
        case MAP_LAYOUT_TOP_BAR:
            return this->kInsetsTopBar;
        case MAP_LAYOUT_FRAMED:
        default:
            return this->kInsetsFramed;
    }
}

SDL_FRect Map::ComputeRectFromVisibleSafeAndInsets(const SDL_FRect& visibleSafe, const MapInsets& insets) const 
{
    // Conversion : pixels logiques OU pourcentage de la zone
    const float L = insets.percent ? visibleSafe.w * insets.left : insets.left;
    const float T = insets.percent ? visibleSafe.h * insets.top : insets.top;
    const float R = insets.percent ? visibleSafe.w * insets.right : insets.right;
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

void Map::UpdateOceanUniforms(double dt) 
{
    this->timeSeconds += dt;

    this->oceanUniforms.params0[0] = (float)this->timeSeconds; // Temps animé
    this->oceanUniforms.params1[0] = this->mapRect.w;          // Largeur zone MAP
    this->oceanUniforms.params1[1] = this->mapRect.h;          // Hauteur zone MAP

    SDL_SetGPURenderStateFragmentUniforms(this->oceanRenderState, 0, &this->oceanUniforms, sizeof(this->oceanUniforms));
}

/* --- Méthodes publiques --- */

Map::Map() 
{
    // Initialisation des uniforms océan
    this->oceanUniforms.params0[0] = 0.0f;   // time
    this->oceanUniforms.params0[1] = 0.6f;   // strength
    this->oceanUniforms.params0[2] = 30.0f;  // px_amp
    this->oceanUniforms.params0[3] = 3.0f;   // tiling
    this->oceanUniforms.params1[2] = 0.60f;  // speed
    this->oceanUniforms.params1[3] = 0.25f;  // extra: reflet/Fresnel
}

Map::~Map() 
{
    this->Unload(); // Assure que les ressources sont libérées
}

void Map::Load() 
{
    // 1) Charger le shader
    this->oceanShader = rc2d_gpu_loadGraphicsShaderFromStorage("water.fragment", RC2D_STORAGE_TITLE);
    if (!this->oceanShader) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ocean shader: %s", SDL_GetError());
        return;
    }

    // 2) Créer un sampler REPEAT
    SDL_GPUSamplerCreateInfo s = {0};
    s.min_filter = SDL_GPU_FILTER_LINEAR;
    s.mag_filter = SDL_GPU_FILTER_LINEAR;
    s.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    s.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    s.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    this->repeatSampler = SDL_CreateGPUSampler(rc2d_engine_state.gpu_device, &s);

    // 3) Charger la texture tile
    this->oceanTile = rc2d_graphics_loadImageFromStorage("assets/images/tile-water.png", RC2D_STORAGE_TITLE);
    SDL_PropertiesID props = SDL_GetTextureProperties(this->oceanTile.sdl_texture);
    SDL_GPUTexture* texGPU = (SDL_GPUTexture*)SDL_GetPointerProperty(
        props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL
    );

    // 4) Construire l'état GPU (shader + sampler)
    SDL_GPUTextureSamplerBinding sb[1] = {0};
    sb[0].texture = texGPU;
    sb[0].sampler = this->repeatSampler;

    SDL_GPURenderStateCreateInfo rs = {0};
    rs.fragment_shader = this->oceanShader;
    rs.num_sampler_bindings = 1;
    rs.sampler_bindings = sb;
    this->oceanRenderState = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs);
}

void Map::Unload() 
{
    rc2d_graphics_freeImage(&this->oceanTile);

    if (this->oceanRenderState) 
    {
        SDL_DestroyGPURenderState(this->oceanRenderState);
        this->oceanRenderState = NULL;
    }

    if (this->repeatSampler) 
    {
        SDL_ReleaseGPUSampler(rc2d_gpu_getDevice(), this->repeatSampler);
        this->repeatSampler = NULL;
    }

    if (this->oceanShader) 
    {
        SDL_ReleaseGPUShader(rc2d_gpu_getDevice(), (SDL_GPUShader*)this->oceanShader);
        this->oceanShader = NULL;
    }
}

void Map::Update(double dt) 
{
    // 1) Choisir les marges selon le mode courant
    this->currentInsets = this->GetInsetsForLayoutMode(this->currentLayoutMode);

    // 2) Zone visible (safe-area ∩ overscan) en coordonnées logiques
    const SDL_FRect visibleSafe = rc2d_engine_getVisibleSafeRectRender();

    // 3) Calcul du rectangle final de la carte
    this->mapRect = this->ComputeRectFromVisibleSafeAndInsets(visibleSafe, this->currentInsets);

    // 4) Mettre à jour les uniforms océan
    if (this->oceanRenderState) 
    {
        this->UpdateOceanUniforms(dt);
    }
}

void Map::Draw() 
{
    // Dessiner l'océan dans la zone de la carte
    if (this->oceanTile.sdl_texture && this->oceanRenderState && this->mapRect.w > 0.f && this->mapRect.h > 0.f) 
    {
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, this->oceanRenderState);
        SDL_RenderTexture(rc2d_engine_state.renderer, this->oceanTile.sdl_texture, NULL, &this->mapRect);
        SDL_SetRenderGPUState(rc2d_engine_state.renderer, NULL);
    }

    // TODO : Ajouter le rendu des éléments UI dans les marges si nécessaire
}

void Map::KeyPressed(const char* key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID) 
{
    RC2D_log(RC2D_LOG_INFO, "Key pressed: key=%s, scancode=%d, keycode=%d, mod=%d, isrepeat=%d, keyboardID=%d\n",
             key, scancode, keycode, mod, isrepeat, keyboardID);

    // Changer le mode d'agencement avec les touches 1 et 2
    if (SDL_strcmp(key, "1") == 0) 
    {
        this->currentLayoutMode = MAP_LAYOUT_FRAMED;
    } 
    else if (SDL_strcmp(key, "2") == 0) 
    {
        this->currentLayoutMode = MAP_LAYOUT_TOP_BAR;
    }
}

void Map::MousePressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n",
             x, y, button, clicks, mouseID);
}