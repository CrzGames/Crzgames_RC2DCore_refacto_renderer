#include <mygame/map.h>
#include <mygame/game.h>

#include <RC2D/RC2D.h>

static RC2D_Image backgroundUI = {0};

static RC2D_UIImage barreActionUI = {0};
static RC2D_UIImage minimapUI = {0};
static RC2D_UIImage buttonCenterMapUI = {0};

static Map map;

void rc2d_unload(void) 
{
    map.Unload();
}

void rc2d_load(void) 
{
    // Taille fenêtre initiale
    rc2d_window_setSize(1280, 720);

    // Charger le background UI
    backgroundUI = rc2d_graphics_loadImageFromStorage("assets/images/background-ui-ingame.png", RC2D_STORAGE_TITLE);

    /* =========================
    BARRE D'ACTION — BAS CENTRE
    ========================= */
    barreActionUI.image       = rc2d_graphics_loadImageFromStorage("assets/images/barre-action-ingame.png", RC2D_STORAGE_TITLE);
    barreActionUI.imageData   = rc2d_graphics_loadImageDataFromStorage("assets/images/barre-action-ingame.png", RC2D_STORAGE_TITLE);
    barreActionUI.anchor      = RC2D_UI_ANCHOR_BOTTOM_CENTER;   // collé en bas, centré horizontalement
    barreActionUI.margin_mode = RC2D_UI_MARGIN_PERCENT;         // marge en % de la zone visible/safe
    barreActionUI.margin_x    = 0.0f;                           // pas de décalage horizontal
    barreActionUI.margin_y    = 0.015f;                         // ~1.5% au-dessus du bord bas
    barreActionUI.visible     = true;
    barreActionUI.hittable    = true;

    /* =========================
    MINIMAP — HAUT DROIT
    ========================= */
    minimapUI.image       = rc2d_graphics_loadImageFromStorage("assets/images/minimap.png", RC2D_STORAGE_TITLE);
    minimapUI.imageData   = rc2d_graphics_loadImageDataFromStorage("assets/images/minimap.png", RC2D_STORAGE_TITLE);
    minimapUI.anchor      = RC2D_UI_ANCHOR_TOP_RIGHT;           // coin haut-droit
    minimapUI.margin_mode = RC2D_UI_MARGIN_PERCENT;             // marges en %
    minimapUI.margin_x    = 0.01f;                              // ~2% depuis la droite
    minimapUI.margin_y    = 0.01f;                              // ~2% depuis le haut
    minimapUI.visible     = true;
    minimapUI.hittable    = true;

    /* =========================
    BOUTON CENTRER LA CARTE — BAS CENTRE
    ========================= */
    buttonCenterMapUI.image       = rc2d_graphics_loadImageFromStorage("assets/images/button-centermap-ingame.png", RC2D_STORAGE_TITLE);
    buttonCenterMapUI.imageData   = rc2d_graphics_loadImageDataFromStorage("assets/images/button-centermap-ingame.png", RC2D_STORAGE_TITLE);
    buttonCenterMapUI.anchor      = RC2D_UI_ANCHOR_BOTTOM_CENTER;
    buttonCenterMapUI.margin_mode = RC2D_UI_MARGIN_PERCENT;
    buttonCenterMapUI.margin_x    = 0.0f;
    buttonCenterMapUI.margin_y    = 0.25f; 
    buttonCenterMapUI.visible     = true;
    buttonCenterMapUI.hittable    = true;

    // Load la carte
    map.Load();
}

void rc2d_update(double dt) 
{
    map.Update(dt);
}

void rc2d_draw(void) 
{
    // Background UI pour le fond
    if (backgroundUI.sdl_texture) 
    {
        rc2d_graphics_drawImage(&backgroundUI, 0.0f, 0.0f, 0.0, 1.0f, 1.0f, 0.0f, 0.0f, false, false);
    }

    // Dessiner la carte (map) avec l'océan animé et les éléments dedans
    map.Draw();

    // Dessiner les éléments UI par-dessus
    rc2d_ui_drawImage(&barreActionUI);
    rc2d_ui_drawImage(&minimapUI);
    rc2d_ui_drawImage(&buttonCenterMapUI);
}

void rc2d_keypressed(const char* key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID) 
{
    map.KeyPressed(key, scancode, keycode, mod, isrepeat, keyboardID);
}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{
    map.MousePressed(x, y, button, clicks, mouseID);
}