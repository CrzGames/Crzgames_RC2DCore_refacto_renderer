#include <mygame/game_screen.h>
#include <RC2D/RC2D_engine.h> // <- pour rc2d_engine_getVisibleSafeRectRender()

// Preset #1 : Mode encadré — place pour minimap + chat + barres
const GameScreenInsets GameScreen::kInsetsFramed = {
    200.0f,  /* left   */
    50.0f,   /* top    */
    200.0f,  /* right  */
    150.0f,   /* bottom */
    false    /* interprétation en pixels logiques */
};

// Preset #2 : Mode barre supérieure — plein écran sauf une barre en haut
const GameScreenInsets GameScreen::kInsetsTopBar = {
    0.0f,    /* left   */
    50.0f,   /* top    */
    0.0f,    /* right  */
    0.0f,    /* bottom */
    false    /* interprétation en pixels logiques */
};

GameScreen::GameScreen(GameScreenLayoutMode mode) 
{
    this->currentLayoutMode = mode;
    this->currentInset = this->GetInsetsForLayoutMode(this->currentLayoutMode);
    const SDL_FRect visibleSafe = rc2d_engine_getVisibleSafeRectRender();
    this->rect = this->ComputeRectFromVisibleSafeAndInsets(visibleSafe, this->currentInset);
}

GameScreen::~GameScreen() {}

void GameScreen::Update(void) 
{
    this->UpdateScreenRect();
}

GameScreenInsets GameScreen::GetInsetsForLayoutMode(GameScreenLayoutMode mode) const
{
    switch (mode) {
        case GAME_SCREEN_LAYOUT_TOP_BAR:
            return this->kInsetsTopBar;
        case GAME_SCREEN_LAYOUT_FRAMED:
        default:
            return this->kInsetsFramed;
    }
}

SDL_FRect GameScreen::ComputeRectFromVisibleSafeAndInsets(const SDL_FRect& visibleSafe, const GameScreenInsets& inset) const 
{
    // Conversion : pixels logiques OU pourcentage de la zone
    const float L = inset.percent ? visibleSafe.w * inset.left : inset.left;
    const float T = inset.percent ? visibleSafe.h * inset.top : inset.top;
    const float R = inset.percent ? visibleSafe.w * inset.right : inset.right;
    const float B = inset.percent ? visibleSafe.h * inset.bottom : inset.bottom;

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

void GameScreen::UpdateScreenRect(void) 
{
    const SDL_FRect visibleSafe = rc2d_engine_getVisibleSafeRectRender();
    this->rect = this->ComputeRectFromVisibleSafeAndInsets(visibleSafe, this->currentInset);
}

void GameScreen::UpdateLayoutMode(GameScreenLayoutMode mode) 
{
    if (this->currentLayoutMode != mode) 
    {
        this->currentLayoutMode = mode;
        this->currentInset = this->GetInsetsForLayoutMode(this->currentLayoutMode);
        const SDL_FRect visibleSafe = rc2d_engine_getVisibleSafeRectRender();
        this->rect = this->ComputeRectFromVisibleSafeAndInsets(visibleSafe, this->currentInset);
    }
}