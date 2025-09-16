#include <game/game_path.h>

#include <RC2D/RC2D_memory.h>

#include <math.h>      /* Requis pour : floorf, fabsf */
#include <string.h>    /* Requis pour : memset        */

/* ========================================================================== */
/*                               GRILLE : API                                 */
/* ========================================================================== */

RC2D_Grid rc2d_grid_create(int width, int height)
{
    RC2D_Grid grid;
    grid.width  = 0;
    grid.height = 0;
    grid.cells  = NULL;

    if (width <= 0 || height <= 0) {
        return grid;
    }

    /* Allocation du tableau (width * height octets). */
    size_t total = (size_t)width * (size_t)height;
    grid.cells = (uint8_t*)RC2D_malloc(total);
    if (!grid.cells) {
        return grid; /* {0} */
    }

    /* Initialiser toutes les cases à 0 (libre). */
    memset(grid.cells, 0, total);
    grid.width  = width;
    grid.height = height;
    return grid;
}

void rc2d_grid_destroy(RC2D_Grid* grid)
{
    if (!grid) return;

    if (grid->cells) {
        RC2D_free(grid->cells);
        grid->cells = NULL;
    }
    grid->width  = 0;
    grid->height = 0;
}


/* ========================================================================== */
/*                            PROJECTION ISO <-> ÉCRAN                        */
/* ========================================================================== */

void rc2d_iso_tileToScreen(int originX, int originY,
                           RC2D_IsoTile tile,
                           float* outX, float* outY)
{
    /* Demi-dimensions utiles à la projection 2:1 */
    const float halfWidth  = RC2D_TILE_WIDTH  * 0.5f;
    const float halfHeight = RC2D_TILE_HEIGHT * 0.5f;

    /* Formules de projection standard (2:1) */
    const float sx = (float)originX + ( (float)tile.x - (float)tile.y ) * halfWidth;
    const float sy = (float)originY + ( (float)tile.x + (float)tile.y ) * halfHeight;

    if (outX) *outX = sx;
    if (outY) *outY = sy;
}

RC2D_IsoTile rc2d_iso_screenToTile(int originX, int originY,
                                   float screenX, float screenY)
{
    /* Inversion (approchée) de la projection 2:1 :
       dx = (sx - originX) / halfWidth
       dy = (sy - originY) / halfHeight
       x  = (dx + dy) / 2
       y  = (dy - dx) / 2
       (on arrondit au plus proche : floor(v + 0.5)) */

    const float halfWidth  = RC2D_TILE_WIDTH  * 0.5f;
    const float halfHeight = RC2D_TILE_HEIGHT * 0.5f;

    const float dx = (screenX - (float)originX) / halfWidth;
    const float dy = (screenY - (float)originY) / halfHeight;

    float fx = (dx + dy) * 0.5f;
    float fy = (dy - dx) * 0.5f;

    RC2D_IsoTile t;
    t.x = (int)floorf(fx + 0.5f);
    t.y = (int)floorf(fy + 0.5f);
    return t;
}


/* ========================================================================== */
/*                          OUTILS / DÉTERMINATION DIRECTION                  */
/* ========================================================================== */

RC2D_IsoDirection rc2d_direction_fromStep(RC2D_IsoTile a, RC2D_IsoTile b)
{
    /* Différence entre A et B. */
    const int dx = b.x - a.x;
    const int dy = b.y - a.y;

    /* Quatre pas valides sur notre grille iso 4-voies. */
    if (dx == +1 && dy == -1) return DIRECTION_NORTH_EAST;
    if (dx == -1 && dy == -1) return DIRECTION_NORTH_WEST;
    if (dx == +1 && dy == +1) return DIRECTION_SOUTH_EAST;
    if (dx == -1 && dy == +1) return DIRECTION_SOUTH_WEST;

    /* Cas non valide : retourner une valeur par défaut sûre. */
    return DIRECTION_SOUTH_EAST;
}

int rc2d_choose_frameIndex(RC2D_IsoDirection direction, float healthRatio)
{
    /* Seuil de santé : >= 0.5 => pleine santé, < 0.5 => faible santé.
       Ajuste ce seuil si ton gameplay l’exige. */
    const bool fullHealth = (healthRatio >= 0.5f);

    if (fullHealth) {
        switch (direction) {
            case DIRECTION_SOUTH_WEST: return 1;
            case DIRECTION_NORTH_EAST: return 2;
            case DIRECTION_NORTH_WEST: return 3;
            case DIRECTION_SOUTH_EAST: return 4;
            default:                   return 4;
        }
    } else {
        switch (direction) {
            case DIRECTION_SOUTH_WEST: return 5;
            case DIRECTION_NORTH_EAST: return 6;
            case DIRECTION_NORTH_WEST: return 7;
            case DIRECTION_SOUTH_EAST: return 8;
            default:                   return 8;
        }
    }
}

void rc2d_make_frameName(int index, char out[16])
{
    /* Sécurisation : contraindre 1..8 puis écrire "N.png". */
    if (index < 1) index = 1;
    if (index > 8) index = 8;

    /* 16 octets suffisent largement pour "DD.png\0". */
    (void)snprintf(out, 16, "%d.png", index);
}


/* ========================================================================== */
/*                          A* (4-VOIES ISO : NE, NW, SE, SW)                 */
/* ========================================================================== */

/* --- Détails d’implémentation -------------------------------------------- */
/* On utilise une structure interne "Node" pour stocker :
   - coût g (depuis le départ),
   - heuristique h (estimation vers l’arrivée),
   - coût total f = g + h,
   - parent (index linéaire du nœud précédent),
   - drapeaux open/closed.

   Pour rester simple et lisible (et vu des grilles 2D de taille variable),
   on emploie :
     - un tableau linéaire de nodes (taille = width*height),
     - une "open list" sur un simple tableau avec recherche du min f (O(N)).

   Cette approche est suffisante pour des cartes moyennes. Pour de très
   grandes cartes, remplacer l’open list par un tas binaire (binary heap).
*/

typedef struct AStarNode {
    int   parent;   /* index du parent dans le tableau nodes, -1 sinon            */
    float g;        /* coût depuis le départ                                       */
    float h;        /* heuristique vers l’arrivée                                   */
    float f;        /* g + h                                                        */
    uint8_t opened; /* 1 si dans open-list, 0 sinon                                 */
    uint8_t closed; /* 1 si déjà traité (dans closed-list), 0 sinon                 */
} AStarNode;

/* Conversions 2D <-> index linéaire. */
static inline int astar_index(const RC2D_Grid* grid, int x, int y) {
    return y * grid->width + x;
}

/* Heuristique admissible pour nos pas iso 4-voies :
   Avec ces mouvements, on peut corriger |dx| et |dy| d'une unité à chaque pas,
   d'où le nombre minimal de pas = max(|dx|, |dy|). */
static inline float astar_heuristic(int dx, int dy) {
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return (float)(dx > dy ? dx : dy);
}

/* Remontée du chemin (depuis goalIdx jusque startIdx via parent),
   puis inversion pour obtenir l’ordre départ -> arrivée. */
static RC2D_Path astar_reconstructPath(const RC2D_Grid* grid,
                                       AStarNode* nodes,
                                       int startIdx, int goalIdx)
{
    RC2D_Path path = { NULL, 0 };

    /* 1) Compter le nombre de pas. */
    int count = 0;
    for (int idx = goalIdx; idx != -1; idx = nodes[idx].parent) {
        ++count;
        if (idx == startIdx) break;
    }
    if (count <= 0) return path;

    /* 2) Allouer le chemin. */
    path.nodes = (RC2D_IsoTile*)RC2D_malloc((size_t)count * sizeof(RC2D_IsoTile));
    if (!path.nodes) {
        return path; /* {NULL, 0} */
    }
    path.count = count;

    /* 3) Remplir à rebours puis inverser. */
    int write = count - 1;
    for (int idx = goalIdx; idx != -1; idx = nodes[idx].parent) {
        int x = idx % grid->width;
        int y = idx / grid->width;
        path.nodes[write].x = x;
        path.nodes[write].y = y;
        if (idx == startIdx) break;
        --write;
    }

    return path;
}

RC2D_Path rc2d_astar_find(const RC2D_Grid* grid,
                          RC2D_IsoTile start,
                          RC2D_IsoTile goal)
{
    RC2D_Path empty = { NULL, 0 };

    /* Validation des entrées de base. */
    if (!grid || !grid->cells) return empty;
    if (!rc2d_grid_inBounds(grid, start.x, start.y)) return empty;
    if (!rc2d_grid_inBounds(grid, goal.x, goal.y))   return empty;

    /* Case de départ/d’arrivée bloquée ? */
    if (rc2d_grid_get(grid, start.x, start.y) != 0) return empty;
    if (rc2d_grid_get(grid, goal.x,  goal.y)  != 0) return empty;

    /* Cas trivial : départ == arrivée. */
    if (start.x == goal.x && start.y == goal.y) {
        RC2D_Path p;
        p.nodes = (RC2D_IsoTile*)RC2D_malloc(sizeof(RC2D_IsoTile));
        if (!p.nodes) return empty;
        p.nodes[0] = start;
        p.count = 1;
        return p;
    }

    const int totalNodes = grid->width * grid->height;

    /* Nodes A* (un par cellule). */
    AStarNode* nodes = (AStarNode*)RC2D_malloc((size_t)totalNodes * sizeof(AStarNode));
    if (!nodes) return empty;
    for (int i = 0; i < totalNodes; ++i) {
        nodes[i].parent = -1;
        nodes[i].g = nodes[i].h = nodes[i].f = 0.0f;
        nodes[i].opened = 0;
        nodes[i].closed = 0;
    }

    /* Open list : indices de nodes ouverts. Simple tableau + recherche du min. */
    int* open = (int*)RC2D_malloc((size_t)totalNodes * sizeof(int));
    if (!open) {
        RC2D_free(nodes);
        return empty;
    }
    int openCount = 0;

    /* Initialiser avec le départ. */
    const int startIdx = astar_index(grid, start.x, start.y);
    const int goalIdx  = astar_index(grid, goal.x,  goal.y);

    nodes[startIdx].g = 0.0f;
    nodes[startIdx].h = astar_heuristic(goal.x - start.x, goal.y - start.y);
    nodes[startIdx].f = nodes[startIdx].g + nodes[startIdx].h;
    nodes[startIdx].opened = 1;
    open[openCount++] = startIdx;

    /* Offsets des 4 voisins iso (NE, NW, SE, SW). */
    const int OFF_X[4] = { +1, -1, +1, -1 };
    const int OFF_Y[4] = { -1, -1, +1, +1 };

    /* Boucle A*. */
    RC2D_Path result = empty;

    while (openCount > 0) {
        /* 1) Sélectionner le nœud ouvert avec le plus petit f. */
        int best = 0;
        float bestF = nodes[open[0]].f;
        for (int i = 1; i < openCount; ++i) {
            const int idx = open[i];
            const float f = nodes[idx].f;
            if (f < bestF) { bestF = f; best = i; }
        }
        const int currentIdx = open[best];

        /* Retirer de l'open-list et marquer en closed. */
        open[best] = open[--openCount];
        nodes[currentIdx].opened = 0;
        nodes[currentIdx].closed = 1;

        /* Si arrivé, reconstruire le chemin. */
        if (currentIdx == goalIdx) {
            result = astar_reconstructPath(grid, nodes, startIdx, goalIdx);
            break;
        }

        /* Coordonnées du nœud courant. */
        const int cx = currentIdx % grid->width;
        const int cy = currentIdx / grid->width;

        /* 2) Explorer les 4 voisins. */
        for (int n = 0; n < 4; ++n) {
            const int nx = cx + OFF_X[n];
            const int ny = cy + OFF_Y[n];

            /* Hors limite ou bloqué ? */
            if (!rc2d_grid_inBounds(grid, nx, ny)) continue;
            if (rc2d_grid_get(grid, nx, ny) != 0)   continue;

            const int nIdx = astar_index(grid, nx, ny);
            if (nodes[nIdx].closed) continue; /* déjà traité */

            /* Coût g si on passe par current -> neighbor (coût constant = 1). */
            const float tentativeG = nodes[currentIdx].g + 1.0f;

            /* Nouveau nœud découvert ? */
            if (!nodes[nIdx].opened) {
                nodes[nIdx].opened = 1;
                nodes[nIdx].parent = currentIdx;
                nodes[nIdx].g = tentativeG;
                nodes[nIdx].h = astar_heuristic(goal.x - nx, goal.y - ny);
                nodes[nIdx].f = nodes[nIdx].g + nodes[nIdx].h;
                open[openCount++] = nIdx;
            }
            /* Déjà dans l’open-list mais meilleur chemin trouvé ? */
            else if (tentativeG < nodes[nIdx].g) {
                nodes[nIdx].parent = currentIdx;
                nodes[nIdx].g = tentativeG;
                nodes[nIdx].f = nodes[nIdx].g + nodes[nIdx].h;
                /* Pas besoin d'ajouter : déjà dans l’open-list. */
            }
        }
    }

    /* Nettoyage des structures internes. */
    RC2D_free(open);
    RC2D_free(nodes);

    return result;
}

void rc2d_path_destroy(RC2D_Path* path)
{
    if (!path) return;
    if (path->nodes) {
        RC2D_free(path->nodes);
        path->nodes = NULL;
    }
    path->count = 0;
}


bool rc2d_grid_inBounds(const RC2D_Grid* grid, int x, int y) 
{
    return grid && x >= 0 && y >= 0 && x < grid->width && y < grid->height;
}

uint8_t rc2d_grid_get(const RC2D_Grid* grid, int x, int y) 
{
    return grid->cells[y * grid->width + x];
}

void rc2d_grid_set(RC2D_Grid* grid, int x, int y, uint8_t value) 
{
    grid->cells[y * grid->width + x] = value;
}