#ifndef MYGAME_PATH_H
#define MYGAME_PATH_H

#include <stdbool.h>   /* Requis pour : bool */
#include <stdint.h>    /* Requis pour : uint8_t */

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                          CONSTANTES / PARAMÈTRES                           */
/* ========================================================================== */

/**
 * \brief Dimensions d'une tuile isométrique (ratio 2:1).
 *
 * Ces valeurs contrôlent les conversions tuile <-> écran.
 * Pour un rendu isométrique type 2:1 :
 * - la largeur visuelle d’une tuile est deux fois sa hauteur,
 * - la projection utilise des demi-dimensions (width/2, height/2).
 *
 * \since Ces macros sont disponibles depuis RC2D 1.0.0.
 */
#define RC2D_TILE_WIDTH   48
#define RC2D_TILE_HEIGHT  32


/* ========================================================================== */
/*                                 STRUCTURES                                 */
/* ========================================================================== */

/**
 * \brief Coordonnées d'une tuile isométrique (entier).
 *
 * Représente la position logique d’un bloc sur la grille isométrique.
 * Les axes (x, y) s’incrémentent/ou décrémentent selon les pas diagonaux :
 * - NORTH_EAST  : (+1, -1)
 * - NORTH_WEST  : (-1, -1)
 * - SOUTH_EAST  : (+1, +1)
 * - SOUTH_WEST  : (-1, +1)
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_IsoTile {
    int x;  /**< Abscisse sur la grille isométrique. */
    int y;  /**< Ordonnée sur la grille isométrique. */
} RC2D_IsoTile;

/**
 * \brief Grille logique pour le pathfinding (0 = libre, 1 = bloqué).
 *
 * La grille est stockée sous forme d’un tableau 1D de `width * height` octets.
 * L’accès à la case (x, y) se fait via l’index : y * width + x.
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_Grid {
    int      origin_x; /**< Origine écran X de la tuile (0,0). */
    int      origin_y; /**< Origine écran Y de la tuile (0,0). */
    int      width;    /**< Nombre de colonnes de la grille.   */
    int      height;   /**< Nombre de lignes de la grille.     */
    uint8_t* cells;    /**< Tableau d’octets (0 libre, 1 mur). */
} RC2D_Grid;

/**
 * \brief Chemin résultant d’un A* : séquence ordonnée de tuiles.
 *
 * Le chemin inclut généralement la tuile de départ et la tuile d’arrivée.
 * L’appelant est responsable de libérer la mémoire via \ref rc2d_path_destroy.
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_Path {
    RC2D_IsoTile* nodes;  /**< Tableau de tuiles (ordonnées du départ vers l’arrivée). */
    int           count;  /**< Longueur du chemin (nombre de nœuds).                  */
} RC2D_Path;


/**
 * \brief Directions isométriques (4 voies) compatibles sprites.
 *
 * Dans un rendu 2:1, on considère 4 directions « diagonales » sur la grille
 * carrée sous-jacente. Ces directions correspondent à tes sprites 1..4 (plein
 * santé) et 5..8 (faible santé) selon la table de mapping ci-dessous.
 *
 * \since Cette énumération est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_IsoDirection {
    DIRECTION_NORTH_EAST = 0,  /**< Vers le haut-droit.  */
    DIRECTION_NORTH_WEST,      /**< Vers le haut-gauche. */
    DIRECTION_SOUTH_EAST,      /**< Vers le bas-droit.   */
    DIRECTION_SOUTH_WEST       /**< Vers le bas-gauche.  */
} RC2D_IsoDirection;


/* ========================================================================== */
/*                                   GRILLE                                   */
/* ========================================================================== */

/**
 * \brief Crée une grille logique initialisée à 0 (toutes cases libres).
 *
 * \param width   Nombre de colonnes.
 * \param height  Nombre de lignes.
 * \return        Grille valide si succès, ou {0} si échec d’allocation.
 *
 * \threadsafety À appeler depuis le thread principal si la grille est partagée
 *               avec le rendu, sinon thread-safe en soi.
 *
 * \note La grille doit être libérée par \ref rc2d_grid_destroy.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
RC2D_Grid rc2d_grid_create(int width, int height);

/**
 * \brief Détruit la mémoire détenue par une grille et remet ses champs à 0.
 *
 * \param grid  Pointeur vers la grille à détruire (peut être NULL).
 *
 * \threadsafety Thread principal recommandé si partagé.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_grid_destroy(RC2D_Grid* grid);

/**
 * \brief Teste si une coordonnée est dans les limites de la grille.
 *
 * \param grid  Grille valide.
 * \param x     Colonne à tester.
 * \param y     Ligne à tester.
 * \return      true si (x, y) est dans la grille, false sinon.
 */
bool rc2d_grid_inBounds(const RC2D_Grid* grid, int x, int y);

/**
 * \brief Lit l’état d’une cellule (0 libre, 1 bloquée).
 *
 * \warning L’appelant doit garantir que (x, y) est dans la grille.
 */
uint8_t rc2d_grid_get(const RC2D_Grid* grid, int x, int y);

/**
 * \brief Modifie l’état d’une cellule (0 libre, 1 bloquée).
 *
 * \warning L’appelant doit garantir que (x, y) est dans la grille.
 */
void rc2d_grid_set(RC2D_Grid* grid, int x, int y, uint8_t value);


/* ========================================================================== */
/*                               PATHFINDING A*                               */
/* ========================================================================== */

/**
 * \brief Trouve un chemin en A* entre deux tuiles sur une grille.
 *
 * L’algorithme utilise les 4 voisins isométriques (NE, NW, SE, SW).  
 * Le coût de chaque pas est constant (1), et l’heuristique utilisée est
 * `max(|dx|, |dy|)`, admissible pour ce graphe (métrique Chebyshev).
 *
 * \param grid   Grille de navigation (0 libre, 1 bloqué).
 * \param start  Tuile de départ.
 * \param goal   Tuile d’arrivée.
 * \return       Chemin alloué sur le tas, ou {NULL,0} si impossible.
 *
 * \note Le chemin doit être libéré par \ref rc2d_path_destroy.
 */
RC2D_Path rc2d_astar_find(const RC2D_Grid* grid,
                          RC2D_IsoTile start,
                          RC2D_IsoTile goal);

/**
 * \brief Libère la mémoire d’un chemin et remet ses champs à 0.
 */
void rc2d_path_destroy(RC2D_Path* path);


/* ========================================================================== */
/*                       PROJECTION ISO <-> COORD. ÉCRAN                      */
/* ========================================================================== */

/**
 * \brief Convertit une tuile isométrique en coordonnées écran (centre).
 *
 * \param originX  Décalage X de l’origine de la carte sur l’écran (pixels).
 * \param originY  Décalage Y de l’origine de la carte sur l’écran (pixels).
 * \param tile     Position de la tuile (x, y) en coordonnées de grille iso.
 * \param outX     Sortie : position X écran (pixels).
 * \param outY     Sortie : position Y écran (pixels).
 *
 * \details Projection 2:1 standard :
 *   screenX = originX + (tile.x - tile.y) * (RC2D_TILE_WIDTH  / 2)
 *   screenY = originY + (tile.x + tile.y) * (RC2D_TILE_HEIGHT / 2)
 */
void rc2d_iso_tileToScreen(int originX, int originY,
                           RC2D_IsoTile tile,
                           float* outX, float* outY);

/**
 * \brief Convertit des coordonnées écran (pixels) en tuile isométrique (arrondie).
 *
 * \param originX  Décalage X de l’origine de la carte sur l’écran (pixels).
 * \param originY  Décalage Y de l’origine de la carte sur l’écran (pixels).
 * \param screenX  Position X écran (pixels).
 * \param screenY  Position Y écran (pixels).
 * \return         Tuile isométrique approchée (arrondie au plus proche).
 *
 * \details Inversion approximative de la projection 2:1 :
 *   dx = (screenX - originX) / (RC2D_TILE_WIDTH  / 2)
 *   dy = (screenY - originY) / (RC2D_TILE_HEIGHT / 2)
 *   tile.x ≈ (dx + dy) / 2
 *   tile.y ≈ (dy - dx) / 2
 */
RC2D_IsoTile rc2d_iso_screenToTile(int originX, int originY,
                                   float screenX, float screenY);


/* ========================================================================== */
/*                            OUTILS DE DIRECTION                             */
/* ========================================================================== */

/**
 * \brief Déduit la direction isométrique en passant de A vers B (voisin direct).
 *
 * \warning Cette fonction suppose que B est un voisin direct 4-voies de A.
 *          Si la différence ne correspond pas exactement à l’un des pas
 *          (NE, NW, SE, SW), le résultat est par défaut DIRECTION_SOUTH_EAST.
 */
RC2D_IsoDirection rc2d_direction_fromStep(RC2D_IsoTile a, RC2D_IsoTile b);

/**
 * \brief Choisit l’index de sprite (1..8) selon la direction et la santé.
 *
 * Mappage demandé :
 *   Pleine santé (healthRatio >= 0.5) : 1..4
 *     - SOUTH_WEST : 1
 *     - NORTH_EAST : 2
 *     - NORTH_WEST : 3
 *     - SOUTH_EAST : 4
 *
 *   Faible santé  (healthRatio < 0.5) : 5..8
 *     - SOUTH_WEST : 5
 *     - NORTH_EAST : 6
 *     - NORTH_WEST : 7
 *     - SOUTH_EAST : 8
 *
 * \param direction    Direction isométrique.
 * \param healthRatio  Ratio de santé [0..1].
 * \return             Index de sprite 1..8.
 */
int rc2d_choose_frameIndex(RC2D_IsoDirection direction, float healthRatio);

/**
 * \brief Construit un nom de fichier "N.png" (N=1..8) dans le tampon fourni.
 *
 * \param index   Index du sprite (1..8).
 * \param out     Tampon de sortie (taille minimale recommandée : 16 octets).
 */
void rc2d_make_frameName(int index, char out[16]);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // MYGAME_PATH_H
