#ifndef RC2D_MOUSE_H
#define RC2D_MOUSE_H

#include <SDL3/SDL_mouse.h>
#include <stdbool.h> // Required for : bool

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Bouton de souris reconnu par RC2D.
 *
 * Cette énumération représente les différents boutons de la souris pouvant être
 * utilisés dans les callbacks `rc2d_mousepressed` et `rc2d_mousereleased`.
 *
 * Elle est directement mappée sur les valeurs définies par SDL3 pour les boutons
 * standards : gauche, milieu, droit, et les boutons additionnels X1/X2.
 *
 * \since Cette énumération est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_MouseButton {
    RC2D_MOUSE_BUTTON_UNKNOWN = 0,  /**< Bouton inconnu ou non supporté */
    RC2D_MOUSE_BUTTON_LEFT    = 1,  /**< Bouton gauche de la souris */
    RC2D_MOUSE_BUTTON_MIDDLE  = 2,  /**< Bouton du milieu (roulette) */
    RC2D_MOUSE_BUTTON_RIGHT   = 3,  /**< Bouton droit de la souris */
    RC2D_MOUSE_BUTTON_X1      = 4,  /**< Bouton latéral 1 de la souris */
    RC2D_MOUSE_BUTTON_X2      = 5   /**< Bouton latéral 2 de la souris */
} RC2D_MouseButton;

/**
 * \brief Direction de défilement de la molette de souris.
 *
 * Cette énumération décrit les différentes directions possibles lors d’un mouvement
 * de la molette, que ce soit à l’horizontale ou à la verticale.
 *
 * \since Cette énumération est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_MouseWheelDirection {
    RC2D_SCROLL_NONE = 0,   /**< Aucun défilement détecté */
    RC2D_SCROLL_UP,         /**< Défilement vers le haut */
    RC2D_SCROLL_DOWN,       /**< Défilement vers le bas */
    RC2D_SCROLL_LEFT,       /**< Défilement vers la gauche */
    RC2D_SCROLL_RIGHT       /**< Défilement vers la droite */
} RC2D_MouseWheelDirection;

/**
 * \brief Enum définissant les différents types de curseurs système.
 * 
 * \since Cette enum est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_SystemCursor {
    /**
     * Le curseur par défaut. Généralement une flèche.
     */
    RC2D_SYSTEM_CURSOR_DEFAULT,

    /**
     * Curseur de sélection de texte. 
     * Généralement un I-beam.
     */
    RC2D_SYSTEM_CURSOR_TEXT,

    /**
     * Curseur d'attente. Généralement un sablier, une montre ou une roue tournante.
     */
    RC2D_SYSTEM_CURSOR_WAIT,

    /**
     * Curseur en forme de croix pour des sélections précises.
     */
    RC2D_SYSTEM_CURSOR_CROSSHAIR,

    /**
     * Le programme est occupé mais reste interactif.
     * Généralement une flèche combinée avec un sablier.
     */
    RC2D_SYSTEM_CURSOR_PROGRESS,

    /**
     * Flèche double pointant vers le nord-ouest et le sud-est.
     * Utilisée pour redimensionner les fenêtres.
     */
    RC2D_SYSTEM_CURSOR_NWSE_RESIZE,

    /**
     * Flèche double pointant vers le nord-est et le sud-ouest.
     * Utilisée pour redimensionner les fenêtres.
     */
    RC2D_SYSTEM_CURSOR_NESW_RESIZE,

    /**
     * Flèche double pointant vers l’est et l’ouest.
     * Utilisée pour redimensionner horizontalement.
     */
    RC2D_SYSTEM_CURSOR_EW_RESIZE,

    /**
     * Flèche double pointant vers le nord et le sud.
     * Utilisée pour redimensionner verticalement.
     */
    RC2D_SYSTEM_CURSOR_NS_RESIZE,

    /**
     * Curseur de déplacement. Généralement une croix à quatre flèches.
     */
    RC2D_SYSTEM_CURSOR_MOVE,

    /**
     * Action non permise. Généralement représenté par un cercle barré.
     */
    RC2D_SYSTEM_CURSOR_NOT_ALLOWED,

    /**
     * Curseur indiquant un lien cliquable. 
     * Généralement une main pointant.
     */
    RC2D_SYSTEM_CURSOR_POINTER,

    /**
     * Redimensionnement depuis le coin supérieur gauche.
     */
    RC2D_SYSTEM_CURSOR_NW_RESIZE,

    /**
     * Redimensionnement depuis le haut.
     */
    RC2D_SYSTEM_CURSOR_N_RESIZE,

    /**
     * Redimensionnement depuis le coin supérieur droit.
     */
    RC2D_SYSTEM_CURSOR_NE_RESIZE,

    /**
     * Redimensionnement depuis la droite.
     */
    RC2D_SYSTEM_CURSOR_E_RESIZE,

    /**
     * Redimensionnement depuis le coin inférieur droit.
     */
    RC2D_SYSTEM_CURSOR_SE_RESIZE,

    /**
     * Redimensionnement depuis le bas.
     */
    RC2D_SYSTEM_CURSOR_S_RESIZE,

    /**
     * Redimensionnement depuis le coin inférieur gauche.
     */
    RC2D_SYSTEM_CURSOR_SW_RESIZE,

    /**
     * Redimensionnement depuis la gauche.
     */
    RC2D_SYSTEM_CURSOR_W_RESIZE,

    /**
     * Nombre total de curseurs système définis.
     */
    RC2D_SYSTEM_CURSOR_COUNT
} RC2D_SystemCursor;

/**
 * \brief Vérifie si la souris est capturée (grab) par la fenêtre RC2D.
 *
 * Lorsque la souris est capturée, ses mouvements sont limités à l’intérieur de la fenêtre.
 *
 * \return {bool} true si la souris est capturée, false sinon.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_mouse_isGrabbed(void);

/**
 * \brief Active ou désactive l’affichage du curseur.
 *
 * \param {bool} visible true pour afficher le curseur, false pour le cacher.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_setVisible(const bool visible);

/**
 * \brief Crée un nouveau curseur système d’après un identifiant RC2D.
 *
 * \param {RC2D_SystemCursor} systemCursorId Identifiant du curseur système souhaité.
 * \return {SDL_Cursor*} Le curseur système créé, ou NULL en cas d’échec.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
SDL_Cursor* rc2d_mouse_newSystemCursor(const RC2D_SystemCursor systemCursorId);

/**
 * \brief Active/désactive la capture (grab) de la souris par la fenêtre RC2D.
 *
 * \param {bool} grabbed true pour capturer la souris, false pour la relâcher.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_window_setGrabbed(bool grabbed);

/**
 * \brief Vérifie si un bouton de la souris est actuellement pressé.
 *
 * \param {RC2D_MouseButton} button Le bouton à vérifier.
 * \return {bool} true si le bouton est pressé, false sinon.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_mouse_isDown(const RC2D_MouseButton button);

/**
 * \brief Récupère la position X actuelle de la souris dans la fenêtre.
 *
 * \return {float} La position X de la souris.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
float rc2d_mouse_getX(void);

/**
 * \brief Récupère la position Y actuelle de la souris dans la fenêtre.
 *
 * \return {float} La position Y de la souris.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
float rc2d_mouse_getY(void);

/**
 * \brief Récupère la position (X,Y) actuelle de la souris dans la fenêtre.
 *
 * \param {float*} x Sortie : position X.
 * \param {float*} y Sortie : position Y.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_getPosition(float* x, float* y);

/**
 * \brief Active/désactive le mode relatif de la souris.
 *
 * En mode relatif, le curseur est masqué, limité à la fenêtre, et les mouvements sont
 * rapportés continuellement même si la souris touche les bords.
 *
 * \param {bool} enabled true pour activer le mode relatif, false pour le désactiver.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_setRelativeMode(const bool enabled);

/**
 * \brief Déplace le curseur de la souris à la position X donnée (Y conservé).
 *
 * \param {float} x Nouvelle position X du curseur.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_setX(float x);

/**
 * \brief Déplace le curseur de la souris à la position Y donnée (X conservé).
 *
 * \param {float} y Nouvelle position Y du curseur.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_setY(float y);

/**
 * \brief Indique si le curseur est actuellement visible.
 *
 * \return {bool} true si le curseur est visible, false sinon.
 *
 * \threadsafety Cette fonction peut être appelée depuis n’importe quel thread.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_mouse_isVisible(void);

/**
 * \brief Détruit et libère un curseur SDL.
 *
 * \param {SDL_Cursor*} cursor Le curseur à détruire (NULL toléré).
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_freeCursor(SDL_Cursor* cursor);

/**
 * \brief Définit le curseur actuellement utilisé par l’application.
 *
 * \param {SDL_Cursor*} cursor Le curseur SDL à activer (non NULL).
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_setCursor(const SDL_Cursor* cursor);

/**
 * \brief Déplace le curseur de la souris à la position (x,y) donnée dans la fenêtre.
 *
 * \param {float} x Position X cible.
 * \param {float} y Position Y cible.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_mouse_setPosition(float x, float y);

#ifdef __cplusplus
}
#endif

#endif // RC2D_MOUSE_H