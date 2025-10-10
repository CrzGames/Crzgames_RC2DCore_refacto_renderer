#ifndef TILE_MAP_H
#define TILE_MAP_H

#include <mygame/entity.h>

#include <RC2D/RC2D.h>

#include <vector>

#define TILEMAP_TILE_WIDTH 48
#define TILEMAP_TILE_HEIGHT 32
#define TILEMAP_COLUMNS 200
#define TILEMAP_ROWS 200
#define TILEMAP_WIDTH (TILEMAP_COLUMNS * TILEMAP_TILE_WIDTH)
#define TILEMAP_HEIGHT (TILEMAP_ROWS * TILEMAP_TILE_HEIGHT)

// Tuile du monde
typedef struct Tile {
    float x;        
    float y;
    int column;                        // Colonne dans la grille
    int row;                           // Ligne dans la grille
    int terrainID;                     // 0=océan, 1=île, 2=récif...
    bool collision;                    // true = bloqué, false = libre
    std::vector<Entity> entities;      // Liste des entités présentes au-dessus de la tuile de terrain
} Tile;

typedef enum TileMapRenderMode {
    TILEMAP_RENDER_ORTHOGRAPHIC = 0, // Grille orthogonale (top-down)
    TILEMAP_RENDER_ISOMETRIC   = 1   // Grille isométrique (2:1)
} TileMapRenderMode;

class TileMap {
    public:
        int tile_width;   // largeur d'une tuile en pixels
        int tile_height;  // hauteur d'une tuile en pixels
        int columns;       // nombre de colonnes
        int rows;          // nombre de lignes
        int width;        // largeur totale de la grille (column * tile_width)
        int height;       // hauteur totale de la grille (row * tile_height)
        float origin_x;     // ancrage écran X de la grille (-> gameScreen.rect.x)
        float origin_y;     // ancrage écran Y de la grille (-> gameScreen.rect.y)
        std::vector<Tile> tiles; // liste des tuiles
        TileMapRenderMode mode; // ORTHO ou ISO

        /**
         * \brief Crée une nouvelle tilemap avec les paramètres spécifiés.
         * 
         * \param columns Nombre de colonnes de tuiles.
         * \param rows Nombre de lignes de tuiles.
         * \param tileWidth Largeur d'une tuile en pixels.
         * \param tileHeight Hauteur d'une tuile en pixels.
         * \param originX Ancrage écran X de la grille.
         * \param originY Ancrage écran Y de la grille.
         * \param mode Mode de rendu de la tilemap (ORTHO ou ISO).
         *
         * Cela met terrain à 0, collision à false, et liste d'entités vide par défaut pour chaque tuile.
        */
        TileMap(int columns, int rows, int tileWidth, int tileHeight, float originX, float originY, TileMapRenderMode mode);
        ~TileMap();

        // Récupère le type de terrain
        int GetTileTerrain(int column, int row) const;

        // Définit le type de terrain
        void SetTileTerrain(int column, int row, int id);

        // Vérifie si la tuile est bloquée (avec collision)
        bool IsTileBlocked(int column, int row) const;

        // Définit la collision d’une tuile
        void SetTileCollision(int column, int row, bool blocked);

        // Ajoute une entité dans la tuile
        void AddTileEntity(int column, int row, const Entity& entity);

        // Supprime une entité de la tuile (suppression par type+id)
        void RemoveTileEntity(int column, int row, const Entity& entity);

        // Récupère les entités de la tuile (référence pour itération)
        std::vector<Entity>& GetTileEntities(int column, int row);
};

#endif // TILE_MAP_H