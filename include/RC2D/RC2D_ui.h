#ifndef RC2D_UI_H
#define RC2D_UI_H

#include <RC2D/RC2D_graphics.h>

#include <stdbool.h> // Required for : bool

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Modes d’interprétation des marges (pixels logiques ou pourcentages).
 *
 * Utilisé par les fonctions rc2d_ui_drawImageAnchored*().
 *
 * \since Cette énumération est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_UIMarginMode {
    RC2D_UI_MARGIN_PIXELS = 0,
    RC2D_UI_MARGIN_PERCENT
} RC2D_UIMarginMode;

/**
 * \brief Ancrages disponibles pour le positionnement UI.
 *
 * Les textures dessinées via les fonctions rc2d_ui_* sont positionnées
 * à l'intérieur de la zone "visible et sûre" (safe area ∩ zone visible en OVERSCAN),
 * obtenue via rc2d_engine_getVisibleSafeRectRender().
 *
 * \par Interprétation des marges (modèle mental)
 * - **Lis l’ancre comme “colle l’élément à …”**, puis interprète les marges comme la distance
 *   depuis ce(s) bord(s)/axe(s).
 * - Si l’ancre contient **RIGHT** → `margin_x` est mesurée **depuis la droite**.
 * - Si l’ancre contient **LEFT** → `margin_x` est mesurée **depuis la gauche**.
 * - Si l’ancre contient **TOP** → `margin_y` est mesurée **depuis le haut**.
 * - Si l’ancre contient **BOTTOM** → `margin_y` est mesurée **depuis le bas**.
 * - Si l’ancre contient **CENTER** sur un axe → la marge correspond à un **décalage** depuis le **centre** sur cet axe
 *   (positif = droite/bas, négatif = gauche/haut).
 *
 * \par Tableau récapitulatif
 * | Anchor                 | Référence horizontale    | `margin_x` signifie…                | Référence verticale  | `margin_y` signifie…            |
 * |:-----------------------|:-------------------------|:------------------------------------|:---------------------|:--------------------------------|
 * | TOP_LEFT               | Gauche                   | Distance du **bord gauche**         | Haut                 | Distance du **haut**            |
 * | TOP_RIGHT              | Droite                   | Distance du **bord droit**          | Haut                 | Distance du **haut**            |
 * | BOTTOM_LEFT            | Gauche                   | Distance du **bord gauche**         | Bas                  | Distance du **bas**             |
 * | BOTTOM_RIGHT           | Droite                   | Distance du **bord droit**          | Bas                  | Distance du **bas**             |
 * | TOP_CENTER             | Centre (horizontal)      | **Décalage** depuis le **centre X** | Haut                 | Distance du **haut**            |
 * | BOTTOM_CENTER          | Centre (horizontal)      | **Décalage** depuis le **centre X** | Bas                  | Distance du **bas**             |
 * | CENTER                 | Centre (horizontal)      | **Décalage** depuis le **centre X** | Centre (vertical)    | **Décalage** depuis le **centre Y** |
 *
 * \since Cette énumération est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_UIAnchor {
    RC2D_UI_ANCHOR_TOP_LEFT = 0,     /**< Coin haut-gauche */
    RC2D_UI_ANCHOR_TOP_RIGHT,        /**< Coin haut-droit */
    RC2D_UI_ANCHOR_BOTTOM_LEFT,      /**< Coin bas-gauche */
    RC2D_UI_ANCHOR_BOTTOM_RIGHT,     /**< Coin bas-droit */
    RC2D_UI_ANCHOR_TOP_CENTER,       /**< Haut-centre (centré horizontalement, référencé en haut) */
    RC2D_UI_ANCHOR_BOTTOM_CENTER,    /**< Bas-centre (centré horizontalement, référencé en bas) */
    RC2D_UI_ANCHOR_CENTER            /**< Centre (centré horizontalement et verticalement) */
} RC2D_UIAnchor;

/**
 * \brief Élément UI image “ancré” avec mémo du dernier rectangle rendu.
 *
 * \note last_drawn_rect est en coordonnées LOGIQUES renderer.
 */
typedef struct RC2D_UIImage {
    RC2D_Image        image;             /**< Texture SDL de l’image (propriété appelant) */
    RC2D_UIAnchor     anchor;            /**< Ancre */
    RC2D_UIMarginMode margin_mode;       /**< Pixels ou % */
    float             margin_x;          /**< pixels logiques OU pourcentage (0..1) suivant margin_mode */
    float             margin_y;          /**< pixels logiques OU pourcentage (0..1) suivant margin_mode */
    SDL_FRect         last_drawn_rect;   /**< MAJ à chaque draw ; {0,0,0,0} si rien dessiné */
} RC2D_UIImage;

bool rc2d_ui_drawImage(RC2D_UIImage* uiImage);

#ifdef __cplusplus
}
#endif

#endif /* RC2D_UI_H */
