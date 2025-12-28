#include <mygame/map.h>

static std::vector<Vec2i> CompressByDirectionRuns(const std::vector<Vec2i>& path) {
    if (path.size() <= 2) return path;
    std::vector<Vec2i> out; out.reserve(path.size());
    out.push_back(path.front());
    int pdx = path[1].x - path[0].x, pdy = path[1].y - path[0].y;
    for (size_t i=2;i<path.size();++i){
        int dx = path[i].x - path[i-1].x, dy = path[i].y - path[i-1].y;
        if (dx != pdx || dy != pdy) { out.push_back(path[i-1]); pdx = dx; pdy = dy; }
    }
    out.push_back(path.back());
    return out;
}


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
    
    // Appliquer les uniforms au render state
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
  camera(gameScreen, tileMap, { TILEMAP_TILE_WIDTH / 2.0f, TILEMAP_TILE_HEIGHT / 2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f })
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

    // Position de la tilemap
    this->tileMap.SetOriginX(this->gameScreen.rect.x);
    this->tileMap.SetOriginY(this->gameScreen.rect.y);

    // Init ship au centre projeté de la tuile (0,0)
    this->playerShip.id = 1;
    this->playerShip.type = ENTITY_PLAYER;
    this->playerShip.x = 10.0f;
    this->playerShip.y = 10.0f;
    this->playerShip.direction = DIRECTION_SOUTH_WEST;
    this->playerShip.health = 100;
}

Map::~Map() {}

void Map::Unload() 
{
    rc2d_graphics_freeImage(&this->oceanTile);
    rc2d_graphics_freeImage(&this->fowNoiseTile);

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

    // 3) Déplacement de la caméra avec les touches fléchées (dans espace projeté)
    this->camera.HandleKeyboardMovement((float)dt);

    UpdateShipMovement(dt);
}

void Map::Draw()
{
    // 1) Clip zone carte
    SDL_Rect clipRect = {
        (int)SDL_roundf(this->gameScreen.rect.x),
        (int)SDL_roundf(this->gameScreen.rect.y),
        (int)SDL_roundf(this->gameScreen.rect.w),
        (int)SDL_roundf(this->gameScreen.rect.h)
    };
    SDL_SetRenderClipRect(rc2d_engine_state.renderer, &clipRect);

    // 2) Océan avec offset cam (approx ; ok pour fond)
    SDL_SetGPURenderState(rc2d_engine_state.renderer, this->oceanRenderState);
    SDL_RenderTexture(rc2d_engine_state.renderer, oceanTile.sdl_texture, nullptr, &this->gameScreen.rect);
    SDL_SetGPURenderState(rc2d_engine_state.renderer, nullptr);

    // 3) Dessiner le vaisseau (coordonnées projetées vers écran)
    float shipScreenX = this->gameScreen.rect.x + (this->playerShip.x - this->camera.config.x) * this->camera.config.zoom;
    float shipScreenY = this->gameScreen.rect.y + (this->playerShip.y - this->camera.config.y) * this->camera.config.zoom;
    const char* spriteName = GetShipSpriteName();
    rc2d_tp_drawFrameByName(&shipAtlas, spriteName, shipScreenX, shipScreenY, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, false, false);

    // 4) Reset clip
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

    if (clicks == 1 && button == RC2D_MOUSE_BUTTON_LEFT) {
        // Check in gameScreen
        if (x >= this->gameScreen.rect.x && x <= this->gameScreen.rect.x + this->gameScreen.rect.w &&
            y >= this->gameScreen.rect.y && y <= this->gameScreen.rect.y + this->gameScreen.rect.h)
        {
            // Convertir clic (écran) → monde (pixels projetés) en inversant la caméra
            float worldX = ( (x - this->gameScreen.rect.x) / this->camera.config.zoom ) + this->camera.config.x;
            float worldY = ( (y - this->gameScreen.rect.y) / this->camera.config.zoom ) + this->camera.config.y;

            // Start/Goal en grille (col,row)
            Vec2i start = WorldToGrid(this->playerShip.x, this->playerShip.y);
            Vec2i goal  = WorldToGrid(worldX, worldY);

            // Bloqueurs : utilise directement ta TileMap
            auto blocked = [this](int cx, int cy)->bool {
                return this->tileMap.IsTileBlocked(cx, cy);
            };

            ThetaStarParams params;
            params.forbidDiagonalCornerCut = true;
            params.anyAngle = false; // NEW: force A* 8-dir (diag puis cardinal)

            m_pathGrid = ThetaStar(
                this->tileMap.columns, this->tileMap.rows,
                start, goal,
                blocked,   // lambda, pas de std::function()
                params
            );
            m_pathGrid = CompressByDirectionRuns(m_pathGrid);
            m_pathIndex = 0;

            StartSegmentFromPath();

            if (m_pathGrid.empty()) {
                RC2D_log(RC2D_LOG_WARN, "No path found start=(%d,%d) goal=(%d,%d)", start.x,start.y,goal.x,goal.y);
            } else {
                RC2D_log(RC2D_LOG_INFO, "Path len=%zu from (%d,%d) to (%d,%d)", m_pathGrid.size(), start.x,start.y,goal.x,goal.y);
            }
        }
    }
}

const char* Map::GetShipSpriteName() const 
{
    bool lowHP = (playerShip.health < 50);
    bool alternate = (spriteToggle % 10) < 5;  // 5 frames each

    const char* spriteName = "";
    switch (playerShip.direction) {
        // Diagonales : fixe
        case DIRECTION_NORTH_EAST:  // 0 : haut-droite
            spriteName = lowHP ? "6.png" : "2.png";
            break;
        case DIRECTION_NORTH_WEST:  // 1 : haut-gauche
            spriteName = lowHP ? "7.png" : "3.png";
            break;
        case DIRECTION_SOUTH_EAST:  // 2 : bas-droite
            spriteName = lowHP ? "8.png" : "4.png";
            break;
        case DIRECTION_SOUTH_WEST:  // 3 : bas-gauche
            spriteName = lowHP ? "5.png" : "1.png";
            break;

        // Cardinales : alternance adjacents
        case DIRECTION_NORTH:  // 4 : haut pur → alternance haut-droite (2/6) / haut-gauche (3/7)
            spriteName = alternate ? (lowHP ? "7.png" : "3.png") : (lowHP ? "6.png" : "2.png");
            break;
        case DIRECTION_SOUTH:  // 5 : bas pur → alternance bas-gauche (1/5) / bas-droite (4/8)
            spriteName = alternate ? (lowHP ? "5.png" : "1.png") : (lowHP ? "8.png" : "4.png");
            break;
        case DIRECTION_EAST:  // 6 : droite pur → alternance haut-droite (2/6) / bas-droite (4/8)
            spriteName = alternate ? (lowHP ? "8.png" : "4.png") : (lowHP ? "6.png" : "2.png");
            break;
        case DIRECTION_WEST:  // 7 : gauche pur → alternance bas-gauche (1/5) / haut-gauche (3/7)
            spriteName = alternate ? (lowHP ? "7.png" : "3.png") : (lowHP ? "5.png" : "1.png");
            break;
    }

    return spriteName;
}

// === ISO 2:1, lignes décalées "odd-r offset" ===
// Tw=48, Th=32 -> halfW=24, halfH=16

// Centre écran (pixels) de la cellule (r=row, c=col) dans un monde projeté
void Map::GridToWorldCenter(const Vec2i& cell, float& outX, float& outY) const {
    const float Tw = (float)tileMap.tile_width;   // 48
    const float Th = (float)tileMap.tile_height;  // 32

    const int r = cell.y;
    const int c = cell.x;

    // même que tileCenter(r,c) de ton snippet Allegro
    outX = ((r & 1) ? 0.0f : Tw * 0.5f) + c * Tw;
    outY = Th * (r + 1) * 0.5f;
}

// Inverse : pixels projetés -> cellule (arrondi au plus proche centre)
Vec2i Map::WorldToGrid(float worldX, float worldY) const {
    const float Tw = (float)tileMap.tile_width;   // 48
    const float Th = (float)tileMap.tile_height;  // 32

    // r ~ round(2*y/Th - 1)
    int r = (int)SDL_floorf((2.0f * worldY) / Th - 1.0f + 0.5f);
    if (r < 0) r = 0;

    // colonne dépend du décalage de la ligne (odd-r)
    float baseX = (r & 1) ? 0.0f : Tw * 0.5f;
    int c = (int)SDL_floorf((worldX - baseX) / Tw + 0.5f);

    return { c, r };
}

// --- Directions entre deux cellules odd-r (membre const)
Direction Map::GridStepToDirectionOddR(int cx, int cy, int nx, int ny) const
{
    int dx = nx - cx;
    int dy = ny - cy;

    if (dy == 0)
        return (dx > 0) ? DIRECTION_EAST : DIRECTION_WEST;

    if (cy & 1) {
        // ligne impaire
        if (dx == +1 && dy == -1) return DIRECTION_NORTH_EAST;
        if (dx ==  0 && dy == -1) return DIRECTION_NORTH_WEST;
        if (dx == +1 && dy == +1) return DIRECTION_SOUTH_EAST;
        if (dx ==  0 && dy == +1) return DIRECTION_SOUTH_WEST;
    } else {
        // ligne paire
        if (dx ==  0 && dy == -1) return DIRECTION_NORTH_EAST;
        if (dx == -1 && dy == -1) return DIRECTION_NORTH_WEST;
        if (dx ==  0 && dy == +1) return DIRECTION_SOUTH_EAST;
        if (dx == -1 && dy == +1) return DIRECTION_SOUTH_WEST;
    }

    // fallback (ne devrait pas arriver si path compressé correctement)
    return (dy < 0) ? DIRECTION_NORTH
         : (dy > 0) ? DIRECTION_SOUTH
         : (dx >= 0) ? DIRECTION_EAST : DIRECTION_WEST;
}


Direction Map::DirectionFromVector(float dx, float dy) const
{
    // 1) Si on est (presque) à l'arrêt, garder la direction actuelle.
    const float speed2 = dx*dx + dy*dy;
    if (speed2 < 1e-6f) {
        return this->playerShip.direction;
    }

    // 2) Angle en degrés [0..360), y>0 = bas (Sud) (coord écran)
    const float PI = 3.1415926535f;
    float deg = SDL_atan2f(dy, dx) * (180.0f / PI);
    if (deg < 0.0f) deg += 360.0f;

    // 3) Secteur 45° (arrondi au plus proche)
    // sector: 0=E,1=SE,2=S,3=SW,4=W,5=NW,6=N,7=NE (sens horaire)
    int sector = (int)SDL_floorf((deg + 22.5f) / 45.0f) & 7;

    // 4) Mapping sector -> ton enum Direction
    static const Direction MAP[8] = {
        DIRECTION_EAST,         // 0 : E
        DIRECTION_SOUTH_EAST,   // 1 : SE
        DIRECTION_SOUTH,        // 2 : S
        DIRECTION_SOUTH_WEST,   // 3 : SW
        DIRECTION_WEST,         // 4 : W
        DIRECTION_NORTH_WEST,   // 5 : NW
        DIRECTION_NORTH,        // 6 : N
        DIRECTION_NORTH_EAST    // 7 : NE
    };

    Direction newDir = MAP[sector];

    // 5) (Option) petite hystérésis : si ça bascule juste à la frontière, garde l’ancienne.
    // Simple: si la nouvelle direction est différente mais l’angle est à moins de 5° du centre
    // de l’ancienne, ne change pas. (Assez pour calmer le jitter.)
    // -> Tu peux ignorer ce bloc si déjà suffisant sans.
#if 1
    auto centerDegFor = [&](Direction d)->float {
        // inverse map Direction -> centre de secteur
        switch (d) {
            case DIRECTION_EAST:        return 0.0f;
            case DIRECTION_SOUTH_EAST:  return 45.0f;
            case DIRECTION_SOUTH:       return 90.0f;
            case DIRECTION_SOUTH_WEST:  return 135.0f;
            case DIRECTION_WEST:        return 180.0f;
            case DIRECTION_NORTH_WEST:  return 225.0f;
            case DIRECTION_NORTH:       return 270.0f;
            case DIRECTION_NORTH_EAST:  return 315.0f;
        }
        return 0.0f;
    };
    auto angDiff = [&](float a, float b)->float {
        float d = SDL_fabsf(a - b);
        if (d > 180.0f) d = 360.0f - d;
        return d;
    };
    const float oldCenter = centerDegFor(this->playerShip.direction);
    const float newCenter = centerDegFor(newDir);
    // si proche du centre de l'ancienne direction (<5°) alors garde l'ancienne
    if (newDir != this->playerShip.direction && angDiff(deg, oldCenter) < 5.0f) {
        return this->playerShip.direction;
    }
#endif

    return newDir;
}

void Map::UpdateShipMovement(double dt)
{
    if (m_pathGrid.empty() || m_pathIndex >= m_pathGrid.size())
        return;

    // (Re)démarrer un segment si besoin : fusionne la run diagonale NE/NW/SE/SW
    if (!m_hasSeg)
        StartSegmentFromPath();
    if (!m_hasSeg)
        return;

    // Cible = centre de la cellule m_segTo (odd-r)
    float targetX = 0.f, targetY = 0.f;
    GridToWorldCenter(m_segTo, targetX, targetY);

    float dx = targetX - this->playerShip.x;
    float dy = targetY - this->playerShip.y;
    float dist = SDL_sqrtf(dx*dx + dy*dy);

    // Arrivé au bout de la run courante ?
    if (dist <= m_waypointEps) {
        // Sauter directement à la fin de la run
        m_pathIndex = m_segEndIndex;
        if (m_pathIndex >= m_pathGrid.size() - 1) {
            m_hasSeg = false;
            return;
        }
        // Passer au début de la run suivante
        ++m_pathIndex;
        StartSegmentFromPath();
        if (!m_hasSeg)
            return;

        GridToWorldCenter(m_segTo, targetX, targetY);
        dx = targetX - this->playerShip.x;
        dy = targetY - this->playerShip.y;
        dist = SDL_sqrtf(dx*dx + dy*dy);
        if (dist <= 0.0001f)
            return;
    }

    // Avancer vers la cible de la run (ligne droite, pas de zigzag)
    float v = m_shipSpeed * (float)dt;
    if (v > dist) v = dist; // évite l’overshoot qui peut provoquer des oscillations
    float nx = dx / dist;
    float ny = dy / dist;

    const float oldX = this->playerShip.x;
    const float oldY = this->playerShip.y;

    this->playerShip.x += nx * v;
    this->playerShip.y += ny * v;

    // Direction sprite : diagonale figée sur la run, cardinal = alternance distance-based
    const bool segIsDiagonal =
        (m_segDir == DIRECTION_NORTH_EAST) || (m_segDir == DIRECTION_NORTH_WEST) ||
        (m_segDir == DIRECTION_SOUTH_EAST) || (m_segDir == DIRECTION_SOUTH_WEST);

    if (segIsDiagonal) {
        // Diagonale : sprite strictement fixe (NE/NW/SE/SW)
        this->playerShip.direction = m_segDir;
        m_moveAccum = 0.0f; // pas d’alternance en diagonale
    } else {
        // Cardinal : garder ta quantification + alternance par distance
        this->playerShip.direction = DirectionFromVector(nx, ny);

        const float moved =
            SDL_sqrtf((this->playerShip.x - oldX)*(this->playerShip.x - oldX) +
                      (this->playerShip.y - oldY)*(this->playerShip.y - oldY));

        if (moved > 0.0f) 
        {
            const bool cardinal =
                this->playerShip.direction == DIRECTION_NORTH ||
                this->playerShip.direction == DIRECTION_SOUTH ||
                this->playerShip.direction == DIRECTION_EAST  ||
                this->playerShip.direction == DIRECTION_WEST;

            if (cardinal) {
                m_moveAccum += moved;
                while (m_moveAccum >= m_stepPixels) {
                    this->spriteToggle = (this->spriteToggle + 1) & 0x7fffffff;
                    m_moveAccum -= m_stepPixels;
                }
            } else {
                m_moveAccum = 0.0f;
            }
        }
    }
}

void Map::StartSegmentFromPath()
{
    m_hasSeg = false;
    if (m_pathGrid.size() < 2 || m_pathIndex >= m_pathGrid.size()-1) return;

    m_segFrom = m_pathGrid[m_pathIndex];
    Vec2i next = m_pathGrid[m_pathIndex+1];
    m_segDir = GridStepToDirectionOddR(m_segFrom.x, m_segFrom.y, next.x, next.y);

    // Par défaut, un seul pas
    m_segEndIndex = m_pathIndex + 1;

    // Si diagonal → étendre la run (NE… / NW… / SE… / SW…)
    bool isDiag = (m_segDir==DIRECTION_NORTH_EAST || m_segDir==DIRECTION_NORTH_WEST ||
                   m_segDir==DIRECTION_SOUTH_EAST || m_segDir==DIRECTION_SOUTH_WEST);
    if (isDiag) {
        size_t j = m_pathIndex + 1;
        while (j+1 < m_pathGrid.size()) {
            Direction d2 = GridStepToDirectionOddR(m_pathGrid[j].x, m_pathGrid[j].y,
                                                   m_pathGrid[j+1].x, m_pathGrid[j+1].y);
            if (d2 != m_segDir) break;
            ++j;
        }
        m_segEndIndex = j; // dernier de la run
    }

    m_segTo  = m_pathGrid[m_segEndIndex];
    m_hasSeg = true;
}

