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

/**
 * \brief Dessine un élément UI de type image, positionné par ancre et marges, et mémorise son rectangle rendu.
 *
 * Cette fonction calcule la position finale de l’image dans la **zone visible et sûre**
 * (intersection safe-area ∩ zone visible en OVERSCAN) obtenue via
 * `rc2d_engine_getVisibleSafeRectRender()`, en fonction de :
 * - l’**ancre** (`RC2D_UIAnchor`) ;
 * - le **mode de marge** (`RC2D_UIMarginMode`) ;
 * - les valeurs de marges `margin_x`, `margin_y` (interprétées en **pixels logiques** ou en **pourcentage**).
 *
 * Le rectangle effectivement dessiné est écrit dans `uiImage->last_drawn_rect` (coordonnées **logiques**).
 * Si le dessin échoue, `last_drawn_rect` est réinitialisé à `{0,0,0,0}`.
 *
 * \param uiImage Élément à dessiner (non NULL). Les champs utilisés sont :
 *  - `image.sdl_texture` : texture SDL (doit être valide),
 *  - `anchor` : choix de l’ancre (voir \ref RC2D_UIAnchor),
 *  - `margin_mode` : `RC2D_UI_MARGIN_PIXELS` ou `RC2D_UI_MARGIN_PERCENT`,
 *  - `margin_x`, `margin_y` : valeur des marges (px logiques ou 0..1),
 *  - `last_drawn_rect` : mis à jour par la fonction.
 *
 * \return `true` en cas de succès, `false` si la texture ou le renderer est invalide,
 *         si la zone visible est nulle, ou si le rendu SDL échoue.
 *
 * \threadsafety Doit être appelée sur le thread principal (rendu SDL).
 *
 * \since Disponible depuis RC2D 1.0.0.
 *
 * \par Interprétation des marges (rappel rapide)
 * - Ancre contenant **RIGHT** ⇒ `margin_x` mesure la distance depuis le **bord droit**.
 * - Ancre contenant **LEFT**  ⇒ `margin_x` mesure la distance depuis le **bord gauche**.
 * - Ancre contenant **TOP**   ⇒ `margin_y` mesure la distance depuis le **haut**.
 * - Ancre contenant **BOTTOM**⇒ `margin_y` mesure la distance depuis le **bas**.
 * - Ancre **CENTER** (sur un axe) ⇒ la marge est un **décalage** depuis le **centre** sur cet axe
 *   (positif = droite/bas, négatif = gauche/haut).
 *
 * \note En mode pourcentage (`RC2D_UI_MARGIN_PERCENT`), `margin_x` est relatif à la **largeur** de la zone visible,
 *       `margin_y` à sa **hauteur** (valeurs attendues dans [0..1], mais les valeurs négatives/>\!1 sont autorisées
 *       pour des décalages volontaires).
 *
 * \code
 * // Exemple 1 — Pixels logiques : minimap en bas-droite avec 20 px de marge
 * RC2D_UIImage minimap = {
 *     .image       = rc2d_graphics_newImage("minimap.png"),
 *     .anchor      = RC2D_UI_ANCHOR_BOTTOM_RIGHT,
 *     .margin_mode = RC2D_UI_MARGIN_PIXELS,
 *     .margin_x    = 20.0f,   // depuis la droite
 *     .margin_y    = 20.0f    // depuis le bas
 * };
 *
 * void rc2d_draw(void)
 * {
 *     rc2d_ui_drawImage(&minimap); // met à jour minimap.last_drawn_rect
 * }
 * \endcode
 *
 * \code
 * // Exemple 2 — Pourcentage : logo en haut-droite, 10%/10%
 * RC2D_UIImage logo = {
 *     .image       = rc2d_graphics_newImage("logo.png"),
 *     .anchor      = RC2D_UI_ANCHOR_TOP_RIGHT,
 *     .margin_mode = RC2D_UI_MARGIN_PERCENT,
 *     .margin_x    = 0.10f,   // 10% de la largeur depuis la droite
 *     .margin_y    = 0.10f    // 10% de la hauteur depuis le haut
 * };
 * \endcode
 *
 * \code
 * // Exemple 3 — Hit-test (clic) avec rc2d_collision_pointInAABB
 * void rc2d_mousepressed(float x, float y, RC2D_MouseButton b, int clicks, SDL_MouseID id)
 * {
 *     const SDL_FRect r = minimap.last_drawn_rect;
 *
 *     RC2D_AABB box = { r.x, r.y, r.w, r.h };
 *     RC2D_Point p = { x, y };
 *
 *     if (rc2d_collision_pointInAABB(p, box)) {
 *         // ... clic sur la minimap
 *     }
 * }
 * \endcode
 */
bool rc2d_ui_drawImage(RC2D_UIImage* uiImage);


#ifdef __cplusplus
}
#endif

#endif /* RC2D_UI_H */
