#ifndef RC2D_TEXTUREPACKER_H
#define RC2D_TEXTUREPACKER_H

#include <RC2D/RC2D_graphics.h>  /* RC2D_Image, rc2d_graphics_* */
#include <RC2D/RC2D_storage.h>   /* RC2D_StorageKind */

#include <SDL3/SDL_rect.h>       /* SDL_FRect, SDL_FPoint */

#include <stdbool.h>             /* bool */

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Frame d’un atlas TexturePacker (format "JSON (Array)").
 *
 * \details
 * Cette structure contient :
 * - la zone source dans l’atlas (coordonnées pixel dans l’image d’atlas),
 * - la position/tailles nécessaires pour replacer correctement la frame
 *   **trimée** dans son canevas d’origine (non trimé).
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_TP_Frame {
    /**
     * Nom de la frame (ex: "walk_01.png"), tel que dans le JSON.
     * \note Mémoire possédée par l’atlas — ne pas libérer manuellement.
     */
    char* filename;

    /**
     * Rectangle de la frame DANS L’IMAGE ATLAS (pixels).
     * - `frame.x`, `frame.y` : origine dans l’atlas (coin haut-gauche).
     * - `frame.w`, `frame.h` : taille découpée stockée dans l’atlas (souvent = zone utile après trim).
     */
    SDL_FRect frame;

    /**
     * Placement de la partie TRIMÉE dans son canevas d’origine (non trimé).
     * - `spriteSourceSize.x`, `spriteSourceSize.y` : offset (pixels) depuis
     *   le coin haut-gauche du canevas d’origine.
     * - `spriteSourceSize.w`, `spriteSourceSize.h` : taille de la partie conservée
     *   (souvent égale à `frame.w`/`frame.h`).
     *
     * \example Pour dessiner "comme si l’image n’avait pas été trimée", on dessine
     * la zone `frame` de l’atlas à la position :
     *   (canvasX + spriteSourceSize.x, canvasY + spriteSourceSize.y)
     * où (canvasX, canvasY) est l’emplacement du canevas d’origine.
     */
    SDL_FRect spriteSourceSize;

    /**
     * Dimensions du canevas d’origine NON TRIMÉ (pixels).
     * - `sourceSize.x` = largeur d’origine (W)
     * - `sourceSize.y` = hauteur d’origine (H)
     *
     * \note Utile pour connaître l’encombrement logique du sprite complet.
     */
    SDL_FPoint sourceSize;
} RC2D_TP_Frame;

/**
 * \brief Atlas TexturePacker chargé (image d’atlas + frames + méta utiles).
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_TP_Atlas {
    /** Texture SDL de l’atlas (déjà chargée). */
    RC2D_Image atlas_image;

    /** Tableau des frames (de taille `frame_count`). */
    RC2D_TP_Frame* frames;

    /** Nombre de frames dans l’atlas. */
    int frame_count;

    /* Méta optionnelles */
    /** Nom du fichier image d’atlas (meta.image), copié depuis le JSON. */
    char* atlas_image_name;

    /** Taille de l’image d’atlas (meta.size.w/h) si présente dans le JSON. */
    SDL_FPoint atlas_size;
} RC2D_TP_Atlas;

/**
 * \brief Charge un atlas TexturePacker (format "JSON (Array)") depuis le storage RC2D.
 *
 * \attention L’image d’atlas (meta.image) est recherchée dans le **même dossier** que le JSON.
 *
 * \param json_path    Chemin relatif (dans le storage) du JSON.
 * \param storage_kind RC2D_STORAGE_TITLE ou RC2D_STORAGE_USER.
 * \return Atlas initialisé, ou atlas vide (tous champs = 0) en cas d’échec.
 *
 * \threadsafety À appeler sur le thread principal.
 *
 * \see rc2d_tp_freeAtlas()
 */
RC2D_TP_Atlas rc2d_tp_loadAtlasFromStorage(const char* json_path, RC2D_StorageKind storage_kind);

/**
 * \brief Libère les ressources d’un atlas TexturePacker (texture, frames, chaînes).
 *
 * \param atlas Atlas à libérer (champs remis à 0).
 *
 * \threadsafety À appeler sur le thread principal.
 */
void rc2d_tp_freeAtlas(RC2D_TP_Atlas* atlas);

/**
 * \brief Récupère une frame par son nom (ex: "1.png").
 *
 * \param atlas    Atlas à consulter.
 * \param filename Nom exact tel que dans le JSON.
 * \return Pointeur interne vers la frame (ne pas libérer), ou NULL si introuvable.
 */
const RC2D_TP_Frame* rc2d_tp_getFrame(const RC2D_TP_Atlas* atlas, const char* filename);

/**
 * \brief Dessine une frame **trimée** en la replaçant correctement dans son canevas d’origine.
 *
 * \details
 * - (canvasX, canvasY) représente la position du **canevas non trimé** (coin haut-gauche).
 * - La zone réellement dessinée (découpée) est `frame->frame` dans l’atlas.
 * - Elle est rendue à l’offset `(frame->spriteSourceSize.x, frame->spriteSourceSize.y)`
 *   par rapport à (canvasX, canvasY).
 *
 * \param atlas     Atlas source (doit contenir la texture).
 * \param frame     Frame à dessiner (issue de rc2d_tp_getFrame ou du tableau `atlas->frames`).
 * \param canvasX   Position X du canevas d’origine.
 * \param canvasY   Position Y du canevas d’origine.
 * \param angle     Rotation en degrés.
 * \param scaleX    Échelle horizontale.
 * \param scaleY    Échelle verticale.
 * \param offsetX   Centre de rotation X (passer -1 pour centrer automatiquement).
 * \param offsetY   Centre de rotation Y (passer -1 pour centrer automatiquement).
 * \param flipH     Retournement horizontal.
 * \param flipV     Retournement vertical.
 */
void rc2d_tp_drawFrame(const RC2D_TP_Atlas* atlas, const RC2D_TP_Frame* frame,
                       float canvasX, float canvasY,
                       double angle,
                       float scaleX, float scaleY,
                       float offsetX, float offsetY,
                       bool flipH, bool flipV);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif /* RC2D_TEXTUREPACKER_H */
