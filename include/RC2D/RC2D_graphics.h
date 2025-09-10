#ifndef RC2D_GRAPHICS_H
#define RC2D_GRAPHICS_H

#include <RC2D/RC2D_math.h>
#include <RC2D/RC2D_storage.h>

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

/* Configuration pour les définitions de fonctions C, même en C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumération des modes de mélange pour le rendu.
 * @typedef {enum} RC2D_BlendMode
 */
typedef enum {
    RC2D_BLENDMODE_NONE,
    RC2D_BLENDMODE_BLEND,
    RC2D_BLENDMODE_BLEND_PREMULTIPLIED,
    RC2D_BLENDMODE_ADD_PREMULTIPLIED,
    RC2D_BLENDMODE_MOD,
    RC2D_BLENDMODE_MUL
} RC2D_BlendMode;

/**
 * Structure représentant une couleur RGBA.
 * @typedef {object} RC2D_Color
 * @property {number} r - Composante rouge (0-255).
 * @property {number} g - Composante verte (0-255).
 * @property {number} b - Composante bleue (0-255).
 * @property {number} a - Composante alpha (transparence) (0-255).
 */
typedef struct RC2D_Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} RC2D_Color;

/**
 * Structure représentant une image.
 * @typedef {object} RC2D_Image
 * @property {SDL_Texture} sdl_texture - Pointeur vers la texture SDL de l'image.
 */
typedef struct RC2D_Image {
    SDL_Texture* sdl_texture;
} RC2D_Image;

/**
 * Structure représentant les données d'une image.
 * @typedef {object} RC2D_ImageData
 * @property {SDL_Surface} sdl_surface - Pointeur vers la surface SDL contenant les données de l'image.
 */
typedef struct RC2D_ImageData {
    SDL_Surface* sdl_surface;
} RC2D_ImageData;

/**
 * Structure représentant une police de caractères.
 * @typedef {object} RC2D_Font
 * @property {TTF_Font} sdl_font - Pointeur vers la police de caractères SDL.
 * @property {number} size - Taille de la police.
 */
typedef struct RC2D_Font {
    TTF_Font* sdl_font;
    int size;
} RC2D_Font;

/**
 * Structure représentant un texte rendu.
 * @typedef {object} RC2D_Text
 * @property {number} width - Largeur du texte.
 * @property {number} height - Hauteur du texte.
 * @property {SDL_Texture} sdl_texture - Pointeur vers la texture SDL du texte.
 */
typedef struct RC2D_Text {
    int width;
    int height;
    SDL_Texture* sdl_texture;
} RC2D_Text;

/**
 * Efface l'écran ou la cible de rendu actuelle avec la couleur de fond.
 */
void rc2d_graphics_clear(void);

/**
 * Présente le contenu rendu à l'écran.
 */
void rc2d_graphics_present(void);

/**
 * Dessine une image à l'écran avec transformations.
 * @param image - L'image à dessiner.
 * @param x - Position x de l'image.
 * @param y - Position y de l'image.
 * @param angle - Angle de rotation en degrés.
 * @param scaleX - Échelle horizontale.
 * @param scaleY - Échelle verticale.
 * @param offsetX - Décalage x pour le centre de rotation.
 * @param offsetY - Décalage y pour le centre de rotation.
 * @param flipHorizontal - Applique un retournement horizontal.
 * @param flipVertical - Applique un retournement vertical.
 *
 * \remarque: Si offsetX et offsetY sont >= 0 sont utilisés comme point de rotation,
 * sinon la rotation se fait autour du centre de l'image (donc passer -1, -1 pour centrer).
 */
void rc2d_graphics_drawImage(RC2D_Image image, float x, float y, double angle, float scaleX, float scaleY, float offsetX, float offsetY, bool flipHorizontal, bool flipVertical);

/**
 * Dessine un rectangle à l'écran.
 * @param mode - Mode de dessin ("fill" pour rempli, "line" pour contour).
 * @param rect - Rectangle à dessiner (SDL_FRect).
 * @return bool - Vrai si le dessin est réussi, faux sinon.
 */
bool rc2d_graphics_rectangle(const char* mode, const SDL_FRect *rect);

/**
 * Dessine plusieurs rectangles à l'écran.
 * @param mode - Mode de dessin ("fill" pour rempli, "line" pour contour).
 * @param numRects - Nombre de rectangles à dessiner.
 * @param rects - Tableau de rectangles (SDL_FRect).
 * @return bool - Vrai si le dessin est réussi, faux sinon.
 */
bool rc2d_graphics_rectangles(const char* mode, const int numRects, const SDL_FRect *rects);

/**
 * Dessine une ligne à l'écran.
 * @param x1 - Coordonnée x du point de départ.
 * @param y1 - Coordonnée y du point de départ.
 * @param x2 - Coordonnée x du point d'arrivée.
 * @param y2 - Coordonnée y du point d'arrivée.
 * @return bool - Vrai si le dessin est réussi, faux sinon.
 */
bool rc2d_graphics_line(const float x1, const float y1, const float x2, const float y2);

/**
 * Dessine plusieurs lignes connectées à l'écran.
 * @param numPoints - Nombre de points définissant les lignes.
 * @param points - Tableau de points (SDL_FPoint).
 * @return bool - Vrai si le dessin est réussi, faux sinon.
 */
bool rc2d_graphics_lines(const int numPoints, const SDL_FPoint *points);

/**
 * Dessine un point à l'écran.
 * @param x - Coordonnée x du point.
 * @param y - Coordonnée y du point.
 * @return bool - Vrai si le dessin est réussi, faux sinon.
 */
bool rc2d_graphics_point(const float x, const float y);

/**
 * Dessine plusieurs points à l'écran.
 * @param numPoints - Nombre de points à dessiner.
 * @param points - Tableau de points (SDL_FPoint).
 * @return bool - Vrai si le dessin est réussi, faux sinon.
 */
bool rc2d_graphics_points(const int numPoints, const SDL_FPoint *points);

/**
 * Crée des données d'image à partir d'un fichier dans le stockage spécifié.
 * @param storage_path - Chemin relatif dans le stockage.
 * @param storage_kind - Type de stockage (RC2D_STORAGE_TITLE ou RC2D_STORAGE_USER).
 * @return RC2D_ImageData - Structure contenant les données de l'image.
 */
RC2D_ImageData rc2d_graphics_newImageDataFromStorage(const char* storage_path, RC2D_StorageKind storage_kind);

/**
 * Crée une image à partir d'un fichier dans le stockage spécifié.
 * @param storage_path - Chemin relatif dans le stockage.
 * @param storage_kind - Type de stockage (RC2D_STORAGE_TITLE ou RC2D_STORAGE_USER).
 * @return RC2D_Image - Structure contenant la texture de l'image.
 */
RC2D_Image rc2d_graphics_newImageFromStorage(const char* storage_path, RC2D_StorageKind storage_kind);

/**
 * Libère les données d'une image.
 * @param imageData - Données de l'image à libérer.
 */
void rc2d_graphics_freeImageData(RC2D_ImageData* imageData);

/**
 * Libère une image.
 * @param image - Image à libérer.
 */
void rc2d_graphics_freeImage(RC2D_Image* image);

/**
 * Crée un texte rendu à partir d'une police et d'une chaîne de caractères.
 * @param font - Police de caractères à utiliser.
 * @param textString - Chaîne de caractères à rendre.
 * @param coloredText - Couleur du texte.
 * @return RC2D_Text - Structure contenant la texture du texte.
 */
RC2D_Text rc2d_graphics_newText(RC2D_Font font, const char* textString, RC2D_Color coloredText);

/**
 * Définit le style de la police de caractères.
 * @param font - Police de caractères à modifier.
 * @param style - Style de la police (par ex. TTF_STYLE_NORMAL).
 */
void rc2d_graphic_setFont(TTF_Font* font, TTF_FontStyleFlags style);

/**
 * Définit la taille de la police de caractères.
 * @param font - Police de caractères à modifier.
 * @param fontSize - Nouvelle taille de la police.
 * @return bool - Vrai si la modification est réussie, faux sinon.
 */
bool rc2d_graphic_setFontSize(TTF_Font* font, float fontSize);

/**
 * Définit la couleur de rendu.
 * @param color - Couleur RGBA à utiliser.
 * @return bool - Vrai si la modification est réussie, faux sinon.
 */
bool rc2d_graphics_setColor(const RC2D_Color color);

/**
 * Récupère la couleur d'un pixel dans les données d'une image.
 * @param imageData - Données de l'image.
 * @param point - Coordonnées du pixel.
 * @param color - Couleur du pixel (sortie).
 */
void rc2d_graphics_getPixel(RC2D_ImageData imageData, const RC2D_Point point, const RC2D_Color color);

/**
 * Définit la couleur d'un pixel dans les données d'une image.
 * @param imageData - Données de l'image.
 * @param point - Coordonnées du pixel.
 * @param color - Couleur à appliquer.
 */
void rc2d_graphics_setPixel(RC2D_ImageData imageData, const RC2D_Point point, const RC2D_Color color);

/**
 * Définit le mode de mélange pour le rendu.
 * @param blendMode - Mode de mélange à appliquer.
 * @return bool - Vrai si la modification est réussie, faux sinon.
 */
bool rc2d_graphics_setBlendMode(const RC2D_BlendMode blendMode);

/**
 * Définit l'échelle de rendu.
 * @param scaleX - Échelle horizontale.
 * @param scaleY - Échelle verticale.
 * @return bool - Vrai si la modification est réussie, faux sinon.
 */
bool rc2d_graphics_scale(float scaleX, float scaleY);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_GRAPHICS_H