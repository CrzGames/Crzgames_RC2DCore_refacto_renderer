#include <mygame/map.h>

/* --- Méthodes privées --- */

void Map::UpdateOceanUniforms(double dt)
{
    timeSeconds += dt;

    this->oceanUniforms.params0[0] = timeSeconds; // time
    this->oceanUniforms.params0[1] = 0.6f;   // strength (0.4..0.8 pour un menu)
    this->oceanUniforms.params0[2] = 30.0f;  // px_amp : ~18 px visibles
    this->oceanUniforms.params0[3] = 3.0f;   // tiling : 6 répétitions

    this->oceanUniforms.params1[0] = this->gameScreen.rect.w; // width
    this->oceanUniforms.params1[1] = this->gameScreen.rect.h;  // height
    this->oceanUniforms.params1[2] = 0.60f;   // speed (0.0..1.0)
    this->oceanUniforms.params1[3] = 0.25f; // reflet/Fresnel

    // Appliquer les uniforms au render state
    SDL_SetGPURenderStateFragmentUniforms(this->oceanRenderState, 0,&this->oceanUniforms, sizeof(this->oceanUniforms));
}

void Map::UpdateFOWUniforms(double dt)
{
    fowTimeSeconds += dt;

    fowUniforms.p0[0] = 2.0f;  // tiling (↑ => plus de répétitions)
    fowUniforms.p0[1] = 0.65f; // opacity globale
    fowUniforms.p0[2] = 0.0f;
    fowUniforms.p0[3] = 0.0f;

    // unused
    fowUniforms.p1[0] = fowUniforms.p1[1] = fowUniforms.p1[2] = fowUniforms.p1[3] = 0.0f;
    fowUniforms.p2[0] = fowUniforms.p2[1] = fowUniforms.p2[2] = fowUniforms.p2[3] = 0.0f;

    // tint neutre (pas de coloration)
    fowUniforms.p3[0] = 1.0f;
    fowUniforms.p3[1] = 1.0f;
    fowUniforms.p3[2] = 1.0f;
    fowUniforms.p3[3] = 0.0f;
    
    SDL_SetGPURenderStateFragmentUniforms(this->fowRenderState, 0, &this->fowUniforms, sizeof(this->fowUniforms));
}

/* --- Méthodes publiques --- */
Map::Map()
: tileMap(TILEMAP_COLUMNS,
          TILEMAP_ROWS,
          TILEMAP_TILE_WIDTH,
          TILEMAP_TILE_HEIGHT,
          /*originX*/ 0.0f,
          /*originY*/ 0.0f,
          TILEMAP_RENDER_ISOMETRIC),
  gameScreen(GAME_SCREEN_LAYOUT_FRAMED),
  camera(gameScreen, tileMap, { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f })
{
    // Initialisation des uniforms océan et des ressources
    this->oceanUniforms = { {0,0,0,0}, {0,0,0,0} };
    this->timeSeconds = 0.0;
    this->oceanShader = NULL;
    this->repeatSampler = NULL;
    this->oceanRenderState = NULL;
    this->oceanTile = { NULL };

    // Initialisation des uniforms pour le brouillard de guerre (FOW)
    this->fowUniforms = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
    this->fowTimeSeconds = 0.0;
    this->fowShader = NULL;
    this->fowRenderState = NULL;
    this->fowNoiseTile = { NULL };

    // Initialisation de l'atlas de navires
    this->shipAtlas = { 0 };
}

Map::~Map() {}

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

    if (this->fowShader) 
    {
        SDL_ReleaseGPUShader(rc2d_gpu_getDevice(), (SDL_GPUShader*)this->fowShader);
        this->fowShader = NULL;
    }

    if (this->fowRenderState) 
    {
        SDL_DestroyGPURenderState(this->fowRenderState);
        this->fowRenderState = NULL;
    }

    if (this->shipAtlas.frames) {
        rc2d_tp_freeAtlas(&this->shipAtlas);
    }
}

void Map::Load() 
{
    // 1) Charger le shader océan + shader FOW
    this->oceanShader = rc2d_gpu_loadGraphicsShaderFromStorage("water.fragment", RC2D_STORAGE_TITLE);
    if (!this->oceanShader) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ocean shader: %s", SDL_GetError());
        return;
    }
    this->fowShader = rc2d_gpu_loadGraphicsShaderFromStorage("fogofwar.fragment", RC2D_STORAGE_TITLE);
    if (!this->fowShader) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load FOW shader: %s", SDL_GetError());
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
    this->repeatSampler = SDL_CreateGPUSampler(rc2d_gpu_getDevice(), &s);

    // 3) Charger la texture tile de l'océan et la texture de bruit pour le FOW
    this->oceanTile = rc2d_graphics_loadImageFromStorage("assets/images/tile-water.png", RC2D_STORAGE_TITLE);
    SDL_PropertiesID props = SDL_GetTextureProperties(this->oceanTile.sdl_texture);
    SDL_GPUTexture* texGPU = (SDL_GPUTexture*)SDL_GetPointerProperty(
        props, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL
    );

    this->fowNoiseTile = rc2d_graphics_loadImageFromStorage("assets/images/Matthias_HalloweenNoise.png", RC2D_STORAGE_TITLE);
    SDL_PropertiesID fowProps = SDL_GetTextureProperties(this->fowNoiseTile.sdl_texture);
    SDL_GPUTexture* fowTexGPU = (SDL_GPUTexture*)SDL_GetPointerProperty(fowProps, SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, NULL);

    // 4) Construire l'état GPU (shader + sampler) pour l'océan
    SDL_GPUTextureSamplerBinding sb[1] = {0};
    sb[0].texture = texGPU;
    sb[0].sampler = this->repeatSampler;

    SDL_GPURenderStateCreateInfo rs = {0};
    rs.fragment_shader = this->oceanShader;
    rs.num_sampler_bindings = 1;
    rs.sampler_bindings = sb;
    this->oceanRenderState = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs);
    if (!this->oceanRenderState) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to create ocean render state: %s", SDL_GetError());
        return;
    }

    // Construire l'état GPU (shader) pour le FOW
    SDL_GPUTextureSamplerBinding sb_fow[1] = {0};
    sb_fow[0].texture = fowTexGPU;
    sb_fow[0].sampler = this->repeatSampler;  // Reuse repeat sampler for tiling

    SDL_GPURenderStateCreateInfo rs2 = {0};
    rs2.fragment_shader = this->fowShader;
    rs2.num_sampler_bindings = 1;  // New: Enable texture sampling
    rs2.sampler_bindings = sb_fow;
    this->fowRenderState = SDL_CreateGPURenderState(rc2d_engine_state.renderer, &rs2);
    if (!this->fowRenderState) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Failed to create FOW render state: %s", SDL_GetError());
        return;
    }

    // 5) Charger l'atlas TexturePacker
    this->shipAtlas = rc2d_tp_loadAtlasFromStorage("assets/atlas/redcosar_lvl1/redcosar_lvl1.json", RC2D_STORAGE_TITLE);
    if (this->shipAtlas.frame_count == 0) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ship atlas: %s", SDL_GetError());
    }
}

void Map::Update(double dt) 
{
    // 1) Mettre à jour le rectangle de la zone de jeu (game screen)
    this->gameScreen.Update();

    // 2) Mettre à jour les uniforms océan et FOW
    if (this->oceanRenderState) 
    {
        this->UpdateOceanUniforms(dt);
    }
    if (this->fowRenderState) 
    {
        this->UpdateFOWUniforms(dt);
    }

    // 3) Déplacement de la caméra avec les touches fléchées
    this->camera.HandleKeyboardMovement((float)dt);
}

void Map::Draw()
{
    // 1) Clip sur la zone carte
    SDL_Rect clipRect = {
        (int)SDL_roundf(this->gameScreen.rect.x),
        (int)SDL_roundf(this->gameScreen.rect.y),
        (int)SDL_roundf(this->gameScreen.rect.w),
        (int)SDL_roundf(this->gameScreen.rect.h)
    };
    SDL_SetRenderClipRect(rc2d_engine_state.renderer, &clipRect);

    // 2) Océan
    SDL_SetGPURenderState(rc2d_engine_state.renderer, oceanRenderState);
    SDL_RenderTexture(rc2d_engine_state.renderer, oceanTile.sdl_texture, nullptr, &this->gameScreen.rect);
    SDL_SetGPURenderState(rc2d_engine_state.renderer, nullptr);

    // Afficher la grille orthographique ou isométrique


    // 3) Îles et autres éléments statiques
    // drawIslands();

    // 4) Entités (navires…) — seront masquées par le fog
    float shipScreenX = WorldToScreenX(10.0f);
    float shipScreenY = WorldToScreenY(10.0f);
    rc2d_tp_drawFrameByName(&shipAtlas, "1.png", shipScreenX, shipScreenY,
                            0.0, 1.0f, 1.0f, -1.0f, -1.0f, false, false);

    // 5) Brouillard de guerre (FOW)
    /*SDL_SetGPURenderState(rc2d_engine_state.renderer, fowRenderState);
    SDL_RenderTexture(rc2d_engine_state.renderer, this->fowNoiseTile.sdl_texture, nullptr, &this->gameScreen.rect);
    SDL_SetGPURenderState(rc2d_engine_state.renderer, nullptr);*/

    // 6) Reset clip
    SDL_SetRenderClipRect(rc2d_engine_state.renderer, nullptr);
}

void Map::KeyPressed(const char* key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID) 
{
    RC2D_log(RC2D_LOG_INFO, "Key pressed: key=%s, scancode=%d, keycode=%d, mod=%d, isrepeat=%d, keyboardID=%d\n",
             key, scancode, keycode, mod, isrepeat, keyboardID);

    // Changer le mode d'agencement avec les touches 1 et 2
    if (SDL_strcmp(key, "1") == 0 && !isrepeat) 
    {
        this->gameScreen.UpdateLayoutMode(GAME_SCREEN_LAYOUT_FRAMED);
    } 
    else if (SDL_strcmp(key, "2") == 0 && !isrepeat) 
    {
        this->gameScreen.UpdateLayoutMode(GAME_SCREEN_LAYOUT_TOP_BAR);
    }
    // Gérer le zoom avec les touches du pavé numérique
    else if (SDL_strcmp(key, "Keypad +") == 0 && !isrepeat) 
    {
        this->camera.AdjustZoom(CAMERA_ZOOM_STEP);
    }
    else if (SDL_strcmp(key, "Keypad -") == 0 && !isrepeat) 
    {
        this->camera.AdjustZoom(-CAMERA_ZOOM_STEP);
    }
}

void Map::MousePressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n",
             x, y, button, clicks, mouseID);
}