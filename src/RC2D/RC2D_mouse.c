#include <RC2D/RC2D_mouse.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL_mouse.h>

bool rc2d_mouse_isGrabbed(void) 
{
    return SDL_GetWindowMouseGrab(rc2d_engine_state.window);
}

void rc2d_mouse_setVisible(const bool visible)
{
	if (visible)
    {
        if (!SDL_ShowCursor())
        {
            RC2D_log(RC2D_LOG_ERROR, "SDL_ShowCursor failed in rc2d_mouse_setVisible: %s\n", SDL_GetError());
        }
    }
	else 
    {
        if (!SDL_HideCursor())
        {
            RC2D_log(RC2D_LOG_ERROR, "SDL_ShowCursor failed in rc2d_mouse_setVisible: %s\n", SDL_GetError());
        }
    }
}

SDL_Cursor* rc2d_mouse_newSystemCursor(const RC2D_SystemCursor systemCursorId) 
{
    SDL_SystemCursor sdlCursorId;
    switch (systemCursorId) 
    {
        case RC2D_SYSTEM_CURSOR_DEFAULT:       sdlCursorId = SDL_SYSTEM_CURSOR_DEFAULT; break;
        case RC2D_SYSTEM_CURSOR_TEXT:          sdlCursorId = SDL_SYSTEM_CURSOR_TEXT; break;
        case RC2D_SYSTEM_CURSOR_WAIT:          sdlCursorId = SDL_SYSTEM_CURSOR_WAIT; break;
        case RC2D_SYSTEM_CURSOR_CROSSHAIR:     sdlCursorId = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
        case RC2D_SYSTEM_CURSOR_PROGRESS:      sdlCursorId = SDL_SYSTEM_CURSOR_PROGRESS; break;
        case RC2D_SYSTEM_CURSOR_NWSE_RESIZE:   sdlCursorId = SDL_SYSTEM_CURSOR_NWSE_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_NESW_RESIZE:   sdlCursorId = SDL_SYSTEM_CURSOR_NESW_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_EW_RESIZE:     sdlCursorId = SDL_SYSTEM_CURSOR_EW_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_NS_RESIZE:     sdlCursorId = SDL_SYSTEM_CURSOR_NS_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_MOVE:          sdlCursorId = SDL_SYSTEM_CURSOR_MOVE; break;
        case RC2D_SYSTEM_CURSOR_NOT_ALLOWED:   sdlCursorId = SDL_SYSTEM_CURSOR_NOT_ALLOWED; break;
        case RC2D_SYSTEM_CURSOR_POINTER:       sdlCursorId = SDL_SYSTEM_CURSOR_POINTER; break;
        case RC2D_SYSTEM_CURSOR_NW_RESIZE:     sdlCursorId = SDL_SYSTEM_CURSOR_NW_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_N_RESIZE:      sdlCursorId = SDL_SYSTEM_CURSOR_N_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_NE_RESIZE:     sdlCursorId = SDL_SYSTEM_CURSOR_NE_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_E_RESIZE:      sdlCursorId = SDL_SYSTEM_CURSOR_E_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_SE_RESIZE:     sdlCursorId = SDL_SYSTEM_CURSOR_SE_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_S_RESIZE:      sdlCursorId = SDL_SYSTEM_CURSOR_S_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_SW_RESIZE:     sdlCursorId = SDL_SYSTEM_CURSOR_SW_RESIZE; break;
        case RC2D_SYSTEM_CURSOR_W_RESIZE:      sdlCursorId = SDL_SYSTEM_CURSOR_W_RESIZE; break;
        default:                                sdlCursorId = SDL_SYSTEM_CURSOR_DEFAULT; break; // Fallback
    }

    SDL_Cursor* cursor = NULL;
    cursor = SDL_CreateSystemCursor(sdlCursorId);
    if (cursor == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_CreateSystemCursor failed in rc2d_mouse_newSystemCursor: %s", SDL_GetError());
        return NULL; // Erreur lors de la création du curseur
    }

    return cursor;
}

void rc2d_window_setGrabbed(bool grabbed)
{
    // Vérifie si la fenêtre est valide
    if (rc2d_engine_state.window == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Impossible de modifier la capture de souris : aucune fenêtre active.\n");
        return;
    }

    // Définit la capture de souris de la fenêtre
    if(!SDL_SetWindowMouseGrab(rc2d_engine_state.window, grabbed))
    {
        RC2D_log(RC2D_LOG_ERROR, "Impossible de définir la capture de souris : %s\n", SDL_GetError());
    }
}

bool rc2d_mouse_isDown(const RC2D_MouseButton button)
{
    SDL_MouseButtonFlags mouseState = SDL_GetMouseState(NULL, NULL);

    switch (button)
    {
        case RC2D_MOUSE_BUTTON_LEFT:
            return (mouseState & SDL_BUTTON_LMASK) != 0;
        case RC2D_MOUSE_BUTTON_MIDDLE:
            return (mouseState & SDL_BUTTON_MMASK) != 0;
        case RC2D_MOUSE_BUTTON_RIGHT:
            return (mouseState & SDL_BUTTON_RMASK) != 0;
        case RC2D_MOUSE_BUTTON_X1:
            return (mouseState & SDL_BUTTON_X1MASK) != 0;
        case RC2D_MOUSE_BUTTON_X2:
            return (mouseState & SDL_BUTTON_X2MASK) != 0;
        default:
            return false;
    }
}

float rc2d_mouse_getX(void)
{
    float x;
    SDL_GetMouseState(&x, NULL);

    return x;
}

float rc2d_mouse_getY(void)
{
    float y;
    SDL_GetMouseState(NULL, &y);

    return y;
}

void rc2d_mouse_getPosition(float* x, float* y) 
{
    if (x == NULL || y == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Les pointeurs x et y sont NULL dans rc2d_mouse_getPosition().");
        return;
    }

    SDL_GetMouseState(x, y);
}

void rc2d_mouse_setRelativeMode(const bool enabled) 
{    
    if(!SDL_SetWindowRelativeMouseMode(rc2d_engine_state.window, enabled))
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_SetRelativeMouseMode failed in rc2d_mouse_setRelativeMode: %s\n", SDL_GetError());
    }
}

void rc2d_mouse_setX(float x)
{
    float currentMouseY;
    SDL_GetMouseState(NULL, &currentMouseY);
    SDL_WarpMouseInWindow(rc2d_engine_state.window, x, currentMouseY);
}

void rc2d_mouse_setY(float y)
{
    float currentMouseX;
    SDL_GetMouseState(&currentMouseX, NULL);
    SDL_WarpMouseInWindow(rc2d_engine_state.window, currentMouseX, y);
}

bool rc2d_mouse_isVisible(void)
{
    return SDL_CursorVisible();
}

void rc2d_mouse_freeCursor(SDL_Cursor* cursor)
{
    if(cursor == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "Le curseur est NULL dans rc2d_mouse_freeCursor(). Rien à libérer.");
        return;
    }

	SDL_DestroyCursor(cursor);
	cursor = NULL;
}

void rc2d_mouse_setCursor(const SDL_Cursor* cursor) 
{
    if (cursor != NULL) 
	{
        if(!SDL_SetCursor(cursor))
        {
            RC2D_log(RC2D_LOG_ERROR, "SDL_SetCursor failed in rc2d_mouse_setCursor: %s\n", SDL_GetError());
        }
    }
}

void rc2d_mouse_setPosition(float x, float y) 
{
    // Déplacer le curseur de la souris à la position ajustée
    SDL_WarpMouseInWindow(rc2d_engine_state.window, x, y);
}