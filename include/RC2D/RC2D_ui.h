#ifndef RC2D_UI_H
#define RC2D_UI_H

#include <RC2D/RC2D_graphics.h>

#include <stdbool.h> // Required for : bool

#ifdef __cplusplus
extern "C" {
#endif

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
 * \brief Dessine une texture ancrée avec marges LOGIQUES (pixels logiques).
 *
 * \param image           Image à dessiner (non NULL). La taille est récupérée via SDL_GetTextureSize().
 * \param anchor          Ancrage (coin/centre) : voir RC2D_UIAnchor.
 * \param margin_x_pixels Marge horizontale en unités logiques (pixels logiques du renderer).
 * \param margin_y_pixels Marge verticale en unités logiques (pixels logiques du renderer).
 * \return true en cas de succès, false en cas d'erreur (renderer/texture NULL, texture invalide,
 *         zone sûre invalide, etc.).
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \code
 * Exemple — ancrages et marges en pixels logiques
 *
 * // 1) En haut-droite, avec 32 px logiques de marge à droite et 24 px du haut
 * rc2d_ui_renderTextureAnchoredPx(renderer, tex, RC2D_UI_ANCHOR_TOP_RIGHT, 32.0f, 24.0f);
 *
 * // 2) En haut-centre, avec 0 px horizontal (centré) et 48 px sous le bord haut
 * rc2d_ui_renderTextureAnchoredPx(renderer, tex, RC2D_UI_ANCHOR_TOP_CENTER, 0.0f, 48.0f);
 *
 * // 3) Au centre exact (marges utilisées comme décalage relatif)
 * rc2d_ui_renderTextureAnchoredPx(renderer, tex, RC2D_UI_ANCHOR_CENTER, 16.0f, -10.0f);
 * \endcode
 */
bool rc2d_ui_drawImageAnchoredPixels(RC2D_Image image,
                                    RC2D_UIAnchor anchor,
                                    float margin_x_pixels, float margin_y_pixels);

/**
 * \brief Dessine une texture ancrée avec marges en POURCENTAGES.
 *
 * \param image                Image à dessiner (non NULL). La taille est récupérée via SDL_GetTextureSize().
 * \param anchor               Ancrage (coin/centre) : voir RC2D_UIAnchor.
 * \param margin_x_percentage  Marge horizontale en pourcentage de la largeur de la zone sûre (0..1).
 * \param margin_y_percentage  Marge verticale en pourcentage de la hauteur de la zone sûre (0..1).
 * \return true en cas de succès, false sinon.
 *
 * \note Les pourcentages sont évalués par rapport à la zone sûre visible actuelle :
 *       - TOP_RIGHT avec (0.10, 0.10) place la texture à 10% du bord droit (vers la gauche)
 *         et 10% du haut (vers le bas) de la zone sûre.
 *       - TOP_CENTER avec (0.10, 0.10) reste centré horizontalement et se décale de 10% vers le bas.
 *       - CENTER avec (0.05, -0.08) se décale du centre de +5% vers la droite et -8% vers le haut.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \code
 * Exemple — ancrages et marges en pourcentage
 *
 * // 1) En haut-droite, 10% des bords droit/haut de la zone sûre
 * rc2d_ui_renderTextureAnchoredPercent(renderer, tex, RC2D_UI_ANCHOR_TOP_RIGHT, 0.10f, 0.10f);
 *
 * // 2) En haut-centre, décalée de 10% vers le bas (X=0.0 => centré)
 * rc2d_ui_renderTextureAnchoredPercent(renderer, tex, RC2D_UI_ANCHOR_TOP_CENTER, 0.0f, 0.10f);
 *
 * // 3) Au centre, décalée de +5% à droite et -8% vers le haut
 * rc2d_ui_renderTextureAnchoredPercent(renderer, tex, RC2D_UI_ANCHOR_CENTER, 0.05f, -0.08f);
 * \endcode
 */
bool rc2d_ui_drawImageAnchoredPercentage(RC2D_Image image,
                                        RC2D_UIAnchor anchor,
                                        float margin_x_percentage, float margin_y_percentage);

#ifdef __cplusplus
}
#endif

#endif /* RC2D_UI_H */
