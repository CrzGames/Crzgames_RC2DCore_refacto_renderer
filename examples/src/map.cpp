#include <mygame/map.h>
#include <cmath> // Pour std::sqrt

/* --- Constantes de la carte --- */
const int Map::TILE_WIDTH  = 48;      // Largeur d'une tuile en pixels
const int Map::TILE_HEIGHT = 32;      // Hauteur d'une tuile en pixels
const int Map::COLUMN      = 100;     // Nombre de colonnes de tuiles
const int Map::ROW         = 100;     // Nombre de lignes de tuiles
const int Map::MAP_WIDTH   = Map::COLUMN * Map::TILE_WIDTH;  // Largeur totale (4800)
const int Map::MAP_HEIGHT  = Map::ROW * Map::TILE_HEIGHT;    // Hauteur totale (3200)
const float Map::MIN_ZOOM  = 0.6f;    // Zoom minimum (60%)
const float Map::MAX_ZOOM  = 1.0f;    // Zoom maximum (100%)

/* --- Presets de marges (constantes) --- */

// Preset #1 : Mode encadré — place pour minimap + chat + barres
const MapInsets Map::kInsetsFramed = {
    200.0f,  /* left   */
    50.0f,   /* top    */
    200.0f,  /* right  */
    150.0f,   /* bottom */
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
    timeSeconds += dt;

    oceanUniforms.params0[0] = timeSeconds; // time
    oceanUniforms.params0[1] = 0.6f;   // strength (0.4..0.8 pour un menu)
    oceanUniforms.params0[2] = 30.0f;  // px_amp : ~18 px visibles
    oceanUniforms.params0[3] = 3.0f;   // tiling : 6 répétitions

    oceanUniforms.params1[0] = this->mapRect.w; // width
    oceanUniforms.params1[1] = this->mapRect.h;  // height
    oceanUniforms.params1[2] = 0.60f;   // speed (0.0..1.0)
    oceanUniforms.params1[3] = 0.25f; // reflet/Fresnel

    // Appliquer les uniforms au render state
    SDL_SetGPURenderStateFragmentUniforms(oceanRenderState, 0,
        &oceanUniforms, sizeof(oceanUniforms));
}

void Map::UpdateCamera(float dx, float dy, float dz) 
{
    // Appliquer les déplacements et le zoom
    this->camera.x += dx;
    this->camera.y += dy;
    this->camera.zoom += dz;

    // Limiter le zoom (30% à 100%)
    if (this->camera.zoom < this->MIN_ZOOM) this->camera.zoom = this->MIN_ZOOM;
    if (this->camera.zoom > this->MAX_ZOOM) this->camera.zoom = this->MAX_ZOOM;

    // Calculer les limites de la caméra en fonction de la taille de la carte totale et du zoom
    float viewWidth = this->mapRect.w / this->camera.zoom;
    float viewHeight = this->mapRect.h / this->camera.zoom;
    this->camera.minX = 0.0f;
    this->camera.maxX = (float)this->MAP_WIDTH - viewWidth;
    this->camera.minY = 0.0f;
    this->camera.maxY = (float)this->MAP_HEIGHT - viewHeight;

    // Clamper la position de la caméra
    if (this->camera.x < this->camera.minX) this->camera.x = this->camera.minX;
    if (this->camera.x > this->camera.maxX) this->camera.x = this->camera.maxX;
    if (this->camera.y < this->camera.minY) this->camera.y = this->camera.minY;
    if (this->camera.y > this->camera.maxY) this->camera.y = this->camera.maxY;
}

/* --- Méthodes publiques --- */

Map::Map() 
{
    // Initialisation des uniforms océan
    this->oceanUniforms = { {0,0,0,0}, {0,0,0,0} };
    this->timeSeconds = 0.0;
    this->oceanShader = NULL;
    this->repeatSampler = NULL;
    this->oceanRenderState = NULL;
    this->oceanTile = { NULL };
    this->shipAtlas = { 0 };
    this->currentLayoutMode = MAP_LAYOUT_FRAMED;
    this->currentInsets = this->kInsetsFramed;
    this->mapRect = { 0, 0, 0, 0 };

    // Initialisation de la caméra
    this->camera.x = 0.0f;
    this->camera.y = 0.0f;
    this->camera.zoom = 1.0f; // 100%
    this->camera.minX = 0.0f;
    this->camera.maxX = (float)this->MAP_WIDTH;
    this->camera.minY = 0.0f;
    this->camera.maxY = (float)this->MAP_HEIGHT;
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
    this->repeatSampler = SDL_CreateGPUSampler(rc2d_gpu_getDevice(), &s);

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

    // 5) Charger l'atlas TexturePacker
    this->shipAtlas = rc2d_tp_loadAtlasFromStorage("assets/atlas/elite24/elite24.json", RC2D_STORAGE_TITLE);
    if (this->shipAtlas.frame_count == 0) {
        RC2D_log(RC2D_LOG_ERROR, "Failed to load ship atlas: %s", SDL_GetError());
    }
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

    if (this->shipAtlas.frames) {
        rc2d_tp_freeAtlas(&this->shipAtlas);
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

    // 5) Gérer les déplacements de la caméra avec les touches fléchées
    const float CAMERA_SPEED = 500.0f;
    float dtf = (float)dt;
    float dx = 0.0f, dy = 0.0f;
    if (rc2d_keyboard_isDown(RC2D_LEFT)) dx -= CAMERA_SPEED * dtf;
    if (rc2d_keyboard_isDown(RC2D_RIGHT)) dx += CAMERA_SPEED * dtf;
    if (rc2d_keyboard_isDown(RC2D_UP)) dy -= CAMERA_SPEED * dtf;
    if (rc2d_keyboard_isDown(RC2D_DOWN)) dy += CAMERA_SPEED * dtf;

    // Normaliser la vitesse pour les mouvements diagonaux
    if (dx != 0.0f && dy != 0.0f) 
    {
        float magnitude = std::sqrt(dx * dx + dy * dy);
        if (magnitude > 0.0f) 
        {
            float scale = (CAMERA_SPEED * dtf) / magnitude;
            dx *= scale;
            dy *= scale;
        }
    }

    if (dx != 0.0f || dy != 0.0f) 
    {
        RC2D_log(RC2D_LOG_INFO, "Camera move: dx=%.1f, dy=%.1f, camera=(%.1f, %.1f, %.2f)\n",
                 dx, dy, this->camera.x, this->camera.y, this->camera.zoom);
        this->UpdateCamera(dx, dy, 0.0f);
    }
}

void Map::Draw()
{
    if (!(this->oceanTile.sdl_texture && this->oceanRenderState && this->mapRect.w > 0.f && this->mapRect.h > 0.f))
        return;

    // 1) Clip à la fenêtre fixe
    SDL_Rect clipRect = {
        (int)SDL_roundf(this->mapRect.x),
        (int)SDL_roundf(this->mapRect.y),
        (int)SDL_roundf(this->mapRect.w),
        (int)SDL_roundf(this->mapRect.h)
    };
    SDL_SetRenderClipRect(rc2d_engine_state.renderer, &clipRect);

    // 2) Rendu océan : on remplit exactement la fenêtre (le shader gère UV/tiling/caméra)
    SDL_SetRenderGPUState(rc2d_engine_state.renderer, this->oceanRenderState);
    SDL_RenderTexture(rc2d_engine_state.renderer, this->oceanTile.sdl_texture, nullptr, &this->mapRect);
    SDL_SetRenderGPUState(rc2d_engine_state.renderer, nullptr);

    // 3) Exemple : dessiner un navire à une position "monde"
    //    Position monde (exemple bidon)
    float shipWorldX = 10.0f;
    float shipWorldY = 10.0f;

    float shipScreenX = WorldToScreenX(shipWorldX);
    float shipScreenY = WorldToScreenY(shipWorldY);

    // Scale visuel = scaleSprite * zoomCam
    float spriteScale = 1.0f * this->camera.zoom;

    rc2d_tp_drawFrameByName(&this->shipAtlas,
                            "1.png",
                            shipScreenX, shipScreenY,
                            0.0,
                            spriteScale, spriteScale,
                            -1.0f, -1.0f,
                            false, false);

    // 4) Reset du clip
    SDL_SetRenderClipRect(rc2d_engine_state.renderer, nullptr);
}

void Map::KeyPressed(const char* key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID) 
{
    RC2D_log(RC2D_LOG_INFO, "Key pressed: key=%s, scancode=%d, keycode=%d, mod=%d, isrepeat=%d, keyboardID=%d\n",
             key, scancode, keycode, mod, isrepeat, keyboardID);

    // Changer le mode d'agencement avec les touches 1 et 2
    if (SDL_strcmp(key, "1") == 0 && !isrepeat) 
    {
        this->currentLayoutMode = MAP_LAYOUT_FRAMED;
    } 
    else if (SDL_strcmp(key, "2") == 0 && !isrepeat) 
    {
        this->currentLayoutMode = MAP_LAYOUT_TOP_BAR;
    }
    // Gérer le zoom avec les touches du pavé numérique
    else if (SDL_strcmp(key, "Keypad +") == 0 && !isrepeat) 
    {
        this->UpdateCamera(0.0f, 0.0f, 0.1f); // ZOOM_SPEED = 0.1f
    }
    else if (SDL_strcmp(key, "Keypad -") == 0 && !isrepeat) 
    {
        this->UpdateCamera(0.0f, 0.0f, -0.1f); // -ZOOM_SPEED
    }
}

void Map::MousePressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n",
             x, y, button, clicks, mouseID);
}