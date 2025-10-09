#include <mygame/map.h>

/* --- Constantes de la carte --- */
const int Map::TILE_WIDTH  = 48;      // Largeur d'une tuile en pixels
const int Map::TILE_HEIGHT = 32;      // Hauteur d'une tuile en pixels
const int Map::COLUMN      = 100;     // Nombre de colonnes de tuiles
const int Map::ROW         = 100;     // Nombre de lignes de tuiles
const int Map::MAP_WIDTH   = Map::COLUMN * Map::TILE_WIDTH;  // Largeur totale (4800)
const int Map::MAP_HEIGHT  = Map::ROW * Map::TILE_HEIGHT;    // Hauteur totale (3200)
const float Map::MIN_ZOOM  = 0.6f;    // Zoom minimum (60%)
const float Map::MAX_ZOOM  = 1.0f;    // Zoom maximum (100%)
const float Map::ZOOM_STEP = 0.1f;    // Incrément de zoom par pression (10%)

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

    this->oceanUniforms.params0[0] = timeSeconds; // time
    this->oceanUniforms.params0[1] = 0.6f;   // strength (0.4..0.8 pour un menu)
    this->oceanUniforms.params0[2] = 30.0f;  // px_amp : ~18 px visibles
    this->oceanUniforms.params0[3] = 3.0f;   // tiling : 6 répétitions

    this->oceanUniforms.params1[0] = this->mapRect.w; // width
    this->oceanUniforms.params1[1] = this->mapRect.h;  // height
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

void Map::UpdateCamera(float dx, float dy, float dz) 
{
    // Appliquer les déplacements et le zoom
    this->camera.x += dx;
    this->camera.y += dy;
    this->camera.zoom += dz;

    // Limiter le zoom (60% à 100%)
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

    // Initialisation du layout
    this->currentLayoutMode = MAP_LAYOUT_FRAMED;
    this->currentInsets = this->kInsetsFramed;

    // Initialisation du rectangle de la carte
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

    // 6) Initialiser la grille de la carte
    this->InitializeGrid();
}

void Map::Update(double dt) 
{
    // 1) Mettre à jour le rectangle de la carte selon le layout
    this->UpdateMapRect();

    // 2) Mettre à jour les uniforms océan et FOW
    if (this->oceanRenderState) 
    {
        this->UpdateOceanUniforms(dt);
    }
    if (this->fowRenderState) 
    {
        this->UpdateFOWUniforms(dt);
    }

    // 3) Gérer les déplacements de la caméra avec les touches fléchées (horizontal + vertical + diagonales)
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
        float magnitude = SDL_sqrt(dx * dx + dy * dy);
        if (magnitude > 0.0f) 
        {
            float scale = (CAMERA_SPEED * dtf) / magnitude;
            dx *= scale;
            dy *= scale;
        }
    }

    // Appliquer le déplacement de la caméra
    if (dx != 0.0f || dy != 0.0f) 
    {
        RC2D_log(RC2D_LOG_INFO, "Camera move: dx=%.1f, dy=%.1f, camera=(%.1f, %.1f, %.2f)\n",
                 dx, dy, this->camera.x, this->camera.y, this->camera.zoom);
        this->UpdateCamera(dx, dy, 0.0f);
    }
}

void Map::Draw()
{
    // 1) Clip sur la zone carte
    SDL_Rect clipRect = {
        (int)SDL_roundf(mapRect.x),
        (int)SDL_roundf(mapRect.y),
        (int)SDL_roundf(mapRect.w),
        (int)SDL_roundf(mapRect.h)
    };
    SDL_SetRenderClipRect(rc2d_engine_state.renderer, &clipRect);

    // 2) Océan
    SDL_SetGPURenderState(rc2d_engine_state.renderer, oceanRenderState);
    SDL_RenderTexture(rc2d_engine_state.renderer, oceanTile.sdl_texture, nullptr, &mapRect);
    SDL_SetGPURenderState(rc2d_engine_state.renderer, nullptr);

    // Afficher la grille orthographique ou isométrique
    //this->Draw2DOrthographicGrid();
    this->Draw2DIsometricGrid();

    // 3) Îles et autres éléments statiques
    // drawIslands();

    // 4) Entités (navires…) — seront masquées par le fog
    float shipScreenX = WorldToScreenX(10.0f);
    float shipScreenY = WorldToScreenY(10.0f);
    rc2d_tp_drawFrameByName(&shipAtlas, "1.png", shipScreenX, shipScreenY,
                            0.0, 1.0f, 1.0f, -1.0f, -1.0f, false, false);

    // 5) Brouillard de guerre (FOW)
    /*SDL_SetGPURenderState(rc2d_engine_state.renderer, fowRenderState);
    SDL_RenderTexture(rc2d_engine_state.renderer, this->fowNoiseTile.sdl_texture, nullptr, &mapRect);
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
        this->currentLayoutMode = MAP_LAYOUT_FRAMED;
    } 
    else if (SDL_strcmp(key, "2") == 0 && !isrepeat) 
    {
        this->currentLayoutMode = MAP_LAYOUT_TOP_BAR;
    }
    // Gérer le zoom avec les touches du pavé numérique
    else if (SDL_strcmp(key, "Keypad +") == 0 && !isrepeat) 
    {
        this->UpdateCamera(0.0f, 0.0f, this->ZOOM_STEP); // ZOOM_SPEED = 0.1f
    }
    else if (SDL_strcmp(key, "Keypad -") == 0 && !isrepeat) 
    {
        this->UpdateCamera(0.0f, 0.0f, -this->ZOOM_STEP); // -ZOOM_SPEED
    }
}

void Map::MousePressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{
    RC2D_log(RC2D_LOG_INFO, "Mouse pressed at (%.1f, %.1f), button=%d, clicks=%d, mouseID=%d\n",
             x, y, button, clicks, mouseID);
}

void Map::InitializeGrid() 
{
    // Allouer la grille
    this->grid.resize(this->ROW * this->COLUMN);

    // Initialiser les métadonnées de la grille
    this->gridMeta.origin_x = mapRect.x;
    this->gridMeta.origin_y = mapRect.y;
    this->gridMeta.mode     = GRID_ISO;

    // Initialiser chaque tuile de la grille
    for (int row = 0; row < this->ROW; ++row) 
    {
        for (int column = 0; column < this->COLUMN; ++column) 
        {
            // Set la position dans la grille (colonne, ligne)
            this->grid[row * this->COLUMN + column].column = column;
            this->grid[row * this->COLUMN + column].row = row;

            // Calculer la position (x,y) de la tuile en pixels
            this->grid[row * this->COLUMN + column].x = this->gridMeta.origin_x + column * this->TILE_WIDTH;
            this->grid[row * this->COLUMN + column].y = this->gridMeta.origin_y + row * this->TILE_HEIGHT;

            // Un peu de 0 et 1 aléatoire pour le terrain (0 = océan, 1 = île)
            int terrainID = (SDL_rand(70) < 20) ? 1 : 0; // 20% d'îles

            // Définir le type de terrain en fonction de la valeur aléatoire
            this->SetTileTerrain(column, row, terrainID);

            // Collision si île (1=île=bloqué, 0=océan=libre)
            this->SetTileCollision(column, row, terrainID == 1);
        }
    }
}

int Map::GetTileTerrain(int column, int row) const
{
    // Hors carte = océan par défaut
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return 0;

    // Retourne le type de terrain
    return this->grid[row * this->COLUMN + column].terrainID;
}

void Map::SetTileTerrain(int column, int row, int id)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return;

    // Définir le type de terrain
    this->grid[row * this->COLUMN + column].terrainID = id;
}

bool Map::IsTileBlocked(int column, int row) const
{
    // Hors carte = bloqué par défaut
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return true;

    // Retourne la collision de la tuile
    return this->grid[row * this->COLUMN + column].collision;
}

void Map::SetTileCollision(int column, int row, bool blocked)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return;

    // Définir la collision de la tuile
    this->grid[row * this->COLUMN + column].collision = blocked;
}

void Map::AddTileEntity(int column, int row, const TileEntity& entity)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return;

    // Ajouter l'entité à la liste de la tuile (ajout en fin de liste)
    this->grid[row * this->COLUMN + column].entities.push_back(entity);
}

void Map::RemoveTileEntity(int column, int row, const TileEntity& entity)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return;

    // Supprimer l'entité de la liste de la tuile
    auto &entities = this->grid[row * this->COLUMN + column].entities;
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
                       [&entity](const TileEntity& e) {
                           return e.type == entity.type && e.id == entity.id;
                       }),
        entities.end()
    );
}

std::vector<TileEntity>& Map::GetTileEntities(int column, int row)
{
    // Hors carte = liste vide
    static std::vector<TileEntity> empty;
    if (column < 0 || row < 0 || column >= this->COLUMN || row >= this->ROW)
        return empty;

    // Retourne la liste des entités de la tuile
    return this->grid[row * this->COLUMN + column].entities;
}

RC2D_Vector2D Map::To3DCoordinates(float x, float y) const 
{
    RC2D_Vector2D newCoord;
    newCoord.x = x - y;
    newCoord.y = (x + y) / 2.0f;

    return newCoord;
}

void Map::Draw2DIsometricGrid()
{
    // Choisis la tuile à tester (2,1) comme dans le tuto
    const int c = 2;
    const int r = 1;

    float topX, topY;
    Iso_MapToScreen((float)c, (float)r, topX, topY);

    const float hx = IsoHalfW() * camera.zoom; // 24 * zoom
    const float hy = IsoHalfH() * camera.zoom; // 16 * zoom

    // Sommets du losange
    const float leftX   = topX - hx;
    const float leftY   = topY + hy;
    const float rightX  = topX + hx;
    const float rightY  = topY + hy;
    const float bottomX = topX;
    const float bottomY = topY + 2.0f * hy;

    // Petit culling contre la zone carte
    SDL_FRect bbox = { topX - hx, topY, 2.0f * hx, 2.0f * hy };
    if (!SDL_HasRectIntersectionFloat(&bbox, &mapRect)) {
        return;
    }

    // Couleur suivant le terrain (optionnel)
    if (GetTileTerrain(c, r) == 1) {
        SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 255, 215, 0, 160); // île
    } else {
        SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 255, 255, 255, 100); // océan
    }

    // Trace l’outline de la tuile
    SDL_RenderLine(rc2d_engine_state.renderer, topX,    topY,    rightX,  rightY);
    SDL_RenderLine(rc2d_engine_state.renderer, rightX,  rightY,  bottomX, bottomY);
    SDL_RenderLine(rc2d_engine_state.renderer, bottomX, bottomY, leftX,   leftY);
    SDL_RenderLine(rc2d_engine_state.renderer, leftX,   leftY,   topX,    topY);

    // reset couleur
    SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 255, 255, 255, 255);
}

void Map::Draw2DOrthographicGrid() 
{
    for (int row = 0; row < this->ROW; ++row) 
    {
        for (int column = 0; column < this->COLUMN; ++column) 
        {
            float x = column * (float)this->TILE_WIDTH;    
            float y = row * (float)this->TILE_HEIGHT;

            float worldX = WorldToScreenX(x);
            float worldY = WorldToScreenY(y);

            SDL_FRect rect = { worldX, worldY, (float)this->TILE_WIDTH, (float)this->TILE_HEIGHT };
            if (this->GetTileTerrain(column, row) == 0) 
            {
                SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 0, 255, 0, 100); // Vert clair pour océan
                SDL_RenderFillRect(rc2d_engine_state.renderer, &rect);
            }
            else if (this->GetTileTerrain(column, row) == 1) 
            {
                SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 139, 69, 19, 200); // Marron pour île
                SDL_RenderFillRect(rc2d_engine_state.renderer, &rect);
            }
        }
    }

    // reset couleur après la boucle
    SDL_SetRenderDrawColor(rc2d_engine_state.renderer, 255, 255, 255, 255);
}

void Map::UpdateMapRect()
{
    this->currentInsets = this->GetInsetsForLayoutMode(this->currentLayoutMode);
    const SDL_FRect visibleSafe = rc2d_engine_getVisibleSafeRectRender();
    this->mapRect = this->ComputeRectFromVisibleSafeAndInsets(visibleSafe, this->currentInsets);
}