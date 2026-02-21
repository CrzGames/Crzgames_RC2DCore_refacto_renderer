#include <mygame/tile_map.h>

TileMap::TileMap(int columns, int rows, int tileWidth, int tileHeight, float originX, float originY, TileMapRenderMode mode)
{
    // Vérifier les paramètres
    if (columns <= 0 || rows <= 0 || tileWidth <= 0 || tileHeight <= 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "TileMapCreate: invalid parameters");
        return;
    }
    // Initialiser les propriétés de la tilemap
    this->columns = columns;
    this->rows = rows;
    this->tile_width = tileWidth;
    this->tile_height = tileHeight;
    this->width = columns * tileWidth;
    this->height = rows * tileHeight;
    this->origin_x = originX;
    this->origin_y = originY;
    this->mode = mode;

    // Allouer la grille de tuiles
    this->tiles.resize(this->columns * this->rows);

    // Set la position x,y et column,row pour chaque cellule / tuile
    for (int row = 0; row < this->rows; ++row) 
    {
        for (int column = 0; column < this->columns; ++column) 
        {
            this->tiles[row * this->columns + column].column = column;
            this->tiles[row * this->columns + column].row = row;
            this->tiles[row * this->columns + column].x = this->origin_x + column * this->tile_width;
            this->tiles[row * this->columns + column].y = this->origin_y + row * this->tile_height;
            this->tiles[row * this->columns + column].terrainID = 0; // 0 par défaut (aucun terrain)
            this->tiles[row * this->columns + column].collision = false; // libre par défaut
            this->tiles[row * this->columns + column].entities.clear(); // liste d'entités vide
        }
    }
}

TileMap::~TileMap() {}

int TileMap::GetTileTerrain(int column, int row) const
{
    // Hors carte = océan par défaut
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return 0;

    // Retourne le type de terrain
    return this->tiles[row * this->columns + column].terrainID;
}

void TileMap::SetTileTerrain(int column, int row, int id)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return;

    // Définir le type de terrain
    this->tiles[row * this->columns + column].terrainID = id;
}

bool TileMap::IsTileBlocked(int column, int row) const
{
    // Hors carte = bloqué par défaut
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return true;

    // Retourne la collision de la tuile
    return this->tiles[row * this->columns + column].collision;
}

void TileMap::SetTileCollision(int column, int row, bool blocked)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return;

    // Définir la collision de la tuile
    this->tiles[row * this->columns + column].collision = blocked;
}

void TileMap::AddTileEntity(int column, int row, const Entity& entity)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return;

    // Ajouter l'entité à la liste de la tuile (ajout en fin de liste)
    this->tiles[row * this->columns + column].entities.push_back(entity);
}

void TileMap::RemoveTileEntity(int column, int row, const Entity& entity)
{
    // Hors carte = ignorer
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return;

    // Supprimer l'entité de la liste de la tuile
    auto &entities = this->tiles[row * this->columns + column].entities;
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
                       [&entity](const Entity& e) {
                           return e.type == entity.type && e.id == entity.id;
                       }),
        entities.end()
    );
}

std::vector<Entity>& TileMap::GetTileEntities(int column, int row)
{
    // Hors carte = liste vide
    static std::vector<Entity> empty;
    if (column < 0 || row < 0 || column >= this->columns || row >= this->rows)
        return empty;

    // Retourne la liste des entités de la tuile
    return this->tiles[row * this->columns + column].entities;
}