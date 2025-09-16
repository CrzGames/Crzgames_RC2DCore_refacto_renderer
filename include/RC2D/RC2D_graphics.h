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
 * Représente un sous-rectangle (source) d'une texture.
 */
typedef struct RC2D_Quad {
    SDL_FRect src; /* x, y, w, h dans l'espace pixel de la texture */
} RC2D_Quad;

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
 * Structure représentant un texte rendu.
 * @typedef {object} RC2D_Text
 * @property {TTF_Text} sdl_text - Pointeur vers le texte rendu SDL.
 * @property {const char*} string - Chaîne de caractères du texte.
 * @property {RC2D_Color} color - Couleur du texte.
 */
typedef struct RC2D_Text {
    TTF_Text* sdl_text;
    const char* string;
    RC2D_Color color;
} RC2D_Text;

/**
 * Structure représentant une police de caractères.
 * @typedef {object} RC2D_Font
 * @property {TTF_Font} sdl_font - Pointeur vers la police de caractères SDL.
 * @property {float} fontSize - Taille de la police.
 * @property {TTF_FontStyleFlags} style - Style de la police (normal, gras, italique, etc.).
 * @property {TTF_HorizontalAlignment} alignment - Alignement horizontal pour le wrapping du texte.
 * @property {void*} _file_data - Pointeur vers les données de la police en mémoire (interne).
 */
typedef struct RC2D_Font {
    TTF_Font* sdl_font;
    float fontSize;
    TTF_FontStyleFlags style;
    TTF_HorizontalAlignment alignment;
    void* _file_data; // <-- garde le buffer vivant pour rc2d_graphics_closeFont
} RC2D_Font;

/* ========================================================================= */
/*                               GRAPHICS CLASSIC                            */
/* ========================================================================= */

/**
 * Efface l'écran ou la cible de rendu actuelle avec la couleur de fond.
 */
void rc2d_graphics_clear(void);

/**
 * Présente le contenu rendu à l'écran.
 */
void rc2d_graphics_present(void);

/**
 * Dessine une tuile isométrique (losange) centrée en (cx,cy) de taille (tile_w,tile_h).
 * mode = "fill" ou "line" (comme rc2d_graphics_rectangle).
 */
bool rc2d_graphics_drawTileIsometric(const char* mode, float cx, float cy, float tile_w, float tile_h);

/**
 * Helper : dessine la tuile (i,j) d’une grille iso.
 * origin_x,origin_y = origine écran de la tuile (0,0)
 * tile_w,tile_h = dimensions iso (diamant).
 */
bool rc2d_graphics_drawTileIsometricAt(const char* mode, int i, int j,
                                        float origin_x, float origin_y,
                                        float tile_w, float tile_h);

/**
 * Crée un Quad (sous-rectangle) borné dans l'image.
 * @param image  Image source (utilisée pour borner/clamp).
 * @param x,y    Position haute-gauche dans la texture (pixels).
 * @param width  Largeur du Quad (pixels).
 * @param height Hauteur du Quad (pixels).
 * @return Quad valide (w/h==0 si échec).
 */
RC2D_Quad rc2d_graphics_newQuad(RC2D_Image* image, float x, float y, float width, float height);

/**
 * Dessine une portion d'image (Quad) avec transformations.
 * Paramètres identiques à rc2d_graphics_drawImage, mais la source est quad->src.
 */
void rc2d_graphics_drawQuad(RC2D_Image* image, const RC2D_Quad* quad,
                            float x, float y,
                            double angle,
                            float scaleX, float scaleY,
                            float offsetX, float offsetY,
                            bool flipHorizontal, bool flipVertical);

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
void rc2d_graphics_drawImage(RC2D_Image* image, float x, float y, double angle, float scaleX, float scaleY, float offsetX, float offsetY, bool flipHorizontal, bool flipVertical);

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
RC2D_ImageData rc2d_graphics_loadImageDataFromStorage(const char* storage_path, RC2D_StorageKind storage_kind);

/**
 * Crée une image à partir d'un fichier dans le stockage spécifié.
 * @param storage_path - Chemin relatif dans le stockage.
 * @param storage_kind - Type de stockage (RC2D_STORAGE_TITLE ou RC2D_STORAGE_USER).
 * @return RC2D_Image - Structure contenant la texture de l'image.
 */
RC2D_Image rc2d_graphics_loadImageFromStorage(const char* storage_path, RC2D_StorageKind storage_kind);

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


/* ========================================================================= */
/*                                 TEXT / FONT                               */
/* ========================================================================= */

/**
 * \brief Ouvre une police de caractères depuis le stockage RC2D.
 *
 * Cette fonction charge une police TrueType/OpenType depuis le stockage spécifié
 * (RC2D_STORAGE_TITLE ou RC2D_STORAGE_USER). Les données du fichier sont lues en mémoire,
 * puis utilisées pour créer une police SDL_ttf via un flux IO.
 *
 * \param {const char*} storage_path Le chemin relatif de la police dans le stockage.
 * \param {RC2D_StorageKind} storage_kind Le type de stockage (TITLE ou USER).
 * \param {float} fontSize La taille de la police en points.
 * \return {RC2D_Font} La structure RC2D_Font initialisée, ou une police vide en cas d’échec.
 *
 * \threadsafety Cette fonction doit être appelée sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_graphics_closeFont()
 */
RC2D_Font rc2d_graphics_openFontFromStorage(const char* storage_path,
                                            RC2D_StorageKind storage_kind,
                                            float fontSize);

/**
 * \brief Ferme et libère une police de caractères RC2D.
 *
 * \param {RC2D_Font*} font La police RC2D à fermer (son TTF_Font interne sera libéré).
 */
void rc2d_graphics_closeFont(RC2D_Font* font);

/**
 * \brief Définit le style appliqué à une police RC2D existante.
 *
 * \note Le style appliqué est lu depuis `font->style`.
 *
 * \param {RC2D_Font*} font La police RC2D à modifier.
 */
void rc2d_graphics_setFontStyle(RC2D_Font* font);

/**
 * \brief Définit la taille d’une police RC2D existante.
 *
 * \note La taille appliquée est lue depuis `font->fontSize`.
 *
 * \param {RC2D_Font*} font La police RC2D à modifier.
 * \return {bool} true si la taille a été appliquée avec succès, false sinon.
 */
bool rc2d_graphics_setFontSize(RC2D_Font* font);

/**
 * \brief Définit l’alignement du wrapping pour une police RC2D.
 *
 * \note L’alignement appliqué est lu depuis `font->alignment`.
 *
 * \param {RC2D_Font*} font La police RC2D à modifier.
 */
void rc2d_graphics_setFontWrapAlignment(RC2D_Font* font);

/**
 * \brief Crée un objet texte RC2D basé sur une police et une chaîne UTF-8.
 *
 * \param {RC2D_Font*} font La police à utiliser.
 * \param {const char*} string La chaîne UTF-8 initiale (non copiée).
 * \return {RC2D_Text} Le texte créé, ou un texte vide en cas d’échec.
 *
 * \note La chaîne n’est pas copiée : l’appelant doit garantir sa durée de vie.
 */
RC2D_Text rc2d_graphics_createText(RC2D_Font* font, const char* string);

/**
 * \brief Détruit un objet texte RC2D et libère ses ressources.
 */
void rc2d_graphics_destroyText(RC2D_Text* text);

/**
 * \brief Met à jour la chaîne UTF-8 d’un texte RC2D depuis `text->string`.
 *
 * \param {RC2D_Text*} text Le texte à modifier.
 * \return {bool} true si la mise à jour a réussi, false sinon.
 */
bool rc2d_graphics_setTextString(RC2D_Text* text);

/**
 * \brief Ajoute la chaîne UTF-8 pointée par `text->string` à la fin du texte RC2D.
 *
 * \param {RC2D_Text*} text Le texte à modifier.
 * \return {bool} true si l’ajout a réussi, false sinon.
 */
bool rc2d_graphics_appendTextString(RC2D_Text* text);

/**
 * \brief Définit la largeur de wrapping (retour à la ligne) pour un texte.
 *
 * \param {RC2D_Text*} text Le texte à modifier.
 * \param {int} wrapWidth La largeur maximale en pixels. 0 désactive le wrapping.
 * \return {bool} true si la largeur a été appliquée avec succès, false sinon.
 */
bool rc2d_graphics_setTextWrapWidth(RC2D_Text* text, int wrapWidth);

/**
 * \brief Définit la couleur RGBA d’un texte RC2D depuis `text->color`.
 *
 * \param {RC2D_Text*} text Le texte à modifier.
 * \return {bool} true si la couleur a été appliquée avec succès, false sinon.
 */
bool rc2d_graphics_setTextColor(RC2D_Text* text);

/**
 * \brief Récupère la taille (largeur, hauteur) d’un texte RC2D.
 */
bool rc2d_graphics_getTextSize(RC2D_Text* text, int* w, int* h);

/**
 * \brief Récupère la taille d’une chaîne UTF-8 pour une police RC2D donnée.
 *
 * \param {RC2D_Font*} font La police RC2D à utiliser.
 * \param {const char*} text La chaîne UTF-8 à mesurer.
 * \param {size_t} length La longueur maximale de la chaîne à mesurer.
 * \param {int*} w Sortie pour la largeur.
 * \param {int*} h Sortie pour la hauteur.
 */
bool rc2d_graphics_getStringSize(RC2D_Font* font, const char* text,
                                 size_t length, int *w, int *h);

/**
 * \brief Récupère la taille d’une chaîne UTF-8 avec wrapping pour une police RC2D donnée.
 *
 * \param {RC2D_Font*} font La police RC2D à utiliser.
 * \param {const char*} text La chaîne UTF-8 à mesurer.
 * \param {size_t} length La longueur maximale de la chaîne à mesurer.
 * \param {int} wrapLength La largeur maximale de wrapping en pixels.
 * \param {int*} w Sortie pour la largeur.
 * \param {int*} h Sortie pour la hauteur.
 */
bool rc2d_graphics_getStringSizeWrapped(RC2D_Font* font, const char* text,
                                        size_t length, int wrapLength,
                                        int *w, int *h);

/**
 * \brief Dessine un objet texte RC2D sur le renderer SDL.
 *
 * \param {RC2D_Text*} text Le texte RC2D à dessiner.
 * \param {float} x La coordonnée X en pixels (du bord gauche vers la droite).
 * \param {float} y La coordonnée Y en pixels (du bord haut vers le bas).
 */
bool rc2d_graphics_drawText(RC2D_Text* text, float x, float y);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif /* RC2D_GRAPHICS_H */