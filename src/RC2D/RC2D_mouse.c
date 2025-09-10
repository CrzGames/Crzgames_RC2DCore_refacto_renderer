#include <RC2D/RC2D_mouse.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_logger.h>

#include <SDL3/SDL_mouse.h>
#include <SDL3_image/SDL_image.h>

/**
 * Vérifie si la souris est capturée par la fenêtre. Lorsque la souris est capturée,
 * les mouvements du curseur sont limités à l'intérieur de la fenêtre de l'application.
 * 
 * @return True si la souris est capturée, False sinon.
 */
bool rc2d_mouse_isGrabbed(void) 
{
    return SDL_GetWindowMouseGrab(rc2d_engine_state.window);
}

/**
 * Définit la visibilité du curseur de la souris.
 * 
 * @param visible Détermine si le curseur doit être visible (true) ou caché (false).
 */
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

/**
 * \brief Crée un nouveau curseur système en fonction de l'identifiant spécifié.
 * 
 * Cette fonction permet d’obtenir un curseur système standard
 * correspondant à l’identifiant RC2D fourni.
 * 
 * @param systemCursorId Identifiant du curseur système souhaité.
 * @return Le curseur système créé. Si la création échoue, le curseur retourné sera invalide (NULL).
 * 
 * @since Cette fonction est disponible depuis RC2D 1.0.0.
 */
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

/**
 * Définit si la souris doit être capturée par la fenêtre.
 * 
 * @param grabbed Détermine si la souris doit être capturée (true) ou relâchée (false).
 */
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

/**
 * @brief Vérifie si un bouton de la souris est actuellement enfoncé.
 *
 * Cette fonction utilise SDL3 pour interroger l'état courant de la souris
 * et détermine si le bouton demandé est pressé.
 *
 * @param button Le bouton de la souris à vérifier (RC2D_MouseButtons).
 * @return true si le bouton spécifié est enfoncé, false sinon.
 *
 * @since This function is available since RC2D 1.0.0.
 */
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

/**
 * Récupère la position x de la souris.
 * 
 * @return La position x actuelle de la souris.
 */
float rc2d_mouse_getX(void)
{
    float x;
    SDL_GetMouseState(&x, NULL);

    return x;
}

/**
 * Récupère la position y de la souris.
 * 
 * @return La position y actuelle de la souris.
 */
float rc2d_mouse_getY(void)
{
    float y;
    SDL_GetMouseState(NULL, &y);

    return y;
}

/**
 * Récupère à la fois les positions x et y de la souris.
 * 
 * @param x Pointeur vers une variable où stocker la position x actuelle de la souris.
 * @param y Pointeur vers une variable où stocker la position y actuelle de la souris.
 */
void rc2d_mouse_getPosition(float* x, float* y) 
{
    if (x == NULL || y == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "Les pointeurs x et y sont NULL dans rc2d_mouse_getPosition().");
        return;
    }

    SDL_GetMouseState(x, y);
}

/**
 * Lorsque la fenêtre est active et que le mode de souris relatif est activé, 
 * le curseur est masqué, la position de la souris est limitée à la fenêtre et 
 * SDL signalera un mouvement relatif continu de la souris même si la souris 
 * est au bord de la fenêtre
 * 
 * @param enabled Détermine si le mode relatif doit être activé (true) ou désactivé (false).
 */
void rc2d_mouse_setRelativeMode(const bool enabled) 
{    
    if(!SDL_SetWindowRelativeMouseMode(rc2d_engine_state.window, enabled))
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_SetRelativeMouseMode failed in rc2d_mouse_setRelativeMode: %s\n", SDL_GetError());
    }
}

/**
 * Définit la position X de la souris dans la fenêtre actuelle.
 * Cette fonction déplace le curseur de la souris à la position X spécifiée, en conservant la position Y actuelle.
 * 
 * @param x La nouvelle position X pour le curseur de la souris.
 */
void rc2d_mouse_setX(float x)
{
    float currentMouseY;
    SDL_GetMouseState(NULL, &currentMouseY);
    SDL_WarpMouseInWindow(rc2d_engine_state.window, x, currentMouseY);
}

/**
 * Définit la position Y de la souris dans la fenêtre actuelle.
 * Cette fonction déplace le curseur de la souris à la position Y spécifiée, en conservant la position X actuelle.
 * 
 * @param y La nouvelle position Y pour le curseur de la souris.
 */
void rc2d_mouse_setY(float y)
{
    float currentMouseX;
    SDL_GetMouseState(&currentMouseX, NULL);
    SDL_WarpMouseInWindow(rc2d_engine_state.window, currentMouseX, y);
}

/**
 * Renvoie si le curseur est actuellement affiché.
 * 
 * @return {bool} True si le curseur est visible, False sinon.
 */
bool rc2d_mouse_isVisible(void)
{
    return SDL_CursorVisible();
}

/**
 * Libère les ressources associées à un curseur.
 * 
 * @param {SDL_Cursor*} cursor - Le curseur à libérer.
 */
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

/**
 * Définit le curseur actuellement utilisé.
 * 
 * @param {SDL_Cursor*} cursor - Le curseur à définir comme actif.
 */
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

/**
 * Déplacez le curseur de la souris à la position donnée dans la fenêtre.
 * 
 * @param {float} x - La position x cible pour le curseur.
 * @param {float} y - La position y cible pour le curseur.
 */
void rc2d_mouse_setPosition(float x, float y) 
{
    // Déplacer le curseur de la souris à la position ajustée
    SDL_WarpMouseInWindow(rc2d_engine_state.window, x, y);
}