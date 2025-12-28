#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <vector>
#include <queue>
#include <limits>
#include <algorithm>
#include <cmath>

/* ----------------------------------------------------------------
   Types & paramètres
------------------------------------------------------------------*/
struct Vec2i { int x, y; };

struct ThetaStarParams {
    bool forbidDiagonalCornerCut = true; // utile seulement en 8-connexe ortho
    bool anyAngle = false;               // true = Theta* (any-angle), false = A* discret
    bool oddrDiamond = true;             // true = voisinage "odd-r" (lignes décalées)
};

/* ----------------------------------------------------------------
   Helpers
------------------------------------------------------------------*/
inline bool pf_InBounds(int x, int y, int W, int H) {
    return (unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H;
}

inline float pf_H(int x, int y, int gx, int gy) {
    int dx = gx - x, dy = gy - y;
    return std::sqrt((float)(dx*dx + dy*dy));
}

inline float pf_StepCost(int dx, int dy) {
    // pour 8-connexe ortho
    return (dx && dy) ? 1.41421356237f : 1.0f;
}

template<typename TBlocked>
inline bool pf_DiagonalCornerCut(int x, int y, int nx, int ny, TBlocked&& blocked) {
    // pour 8-connexe ortho uniquement
    int dx = nx - x, dy = ny - y;
    if (dx && dy) {
        if (blocked(x+dx, y) && blocked(x, y+dy)) return true;
    }
    return false;
}

/* ----------------------------------------------------------------
   Ligne de vue (supercover) — utilisé seulement si anyAngle=true
   (inutile / ignoré en oddrDiamond=true)
------------------------------------------------------------------*/
template<typename TBlocked>
inline bool LineOfSightSupercover(int x0,int y0,int x1,int y1, TBlocked&& blocked) {
    int dx = std::abs(x1-x0), dy = std::abs(y1-y0);
    int sx = (x1 >= x0) ? 1 : -1;
    int sy = (y1 >= y0) ? 1 : -1;

    int x = x0, y = y0;
    int err = dx - dy;

    if (blocked(x,y)) return false;

    for (;;) {
        if (x == x1 && y == y1) return true;

        int e2 = err << 1;
        int xOld = x, yOld = y;

        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 <  dx) { err += dx; y += sy; }

        // supercover: couvre les deux cases en diagonale
        if (x != xOld && y != yOld) {
            if (blocked(x, yOld) && blocked(xOld, y)) return false;
            if (blocked(x, y)) return false;
        } else {
            if (blocked(x, y)) return false;
        }
    }
}

/* ----------------------------------------------------------------
   A* / Theta* (any-angle optionnel)
   - oddrDiamond=true  : graphe "diamant" à lignes décalées (odd-r offset),
                         parfait pour isométrique 2:1 façon Seafight.
   - oddrDiamond=false : graphe 8-connexe orthogonal classique.
   - anyAngle=true     : active Theta* (ignorer si oddrDiamond=true).
------------------------------------------------------------------*/
template<typename TBlocked>
inline std::vector<Vec2i> ThetaStar(
    int W, int H,
    Vec2i S, Vec2i G,
    TBlocked&& blocked,
    const ThetaStarParams& params = {}
){
    auto idx = [W](int x,int y){ return y*W + x; };

    std::vector<float> g(W*H, std::numeric_limits<float>::infinity());
    std::vector<char>  closed(W*H, 0);
    std::vector<Vec2i> parent(W*H, {-1,-1});

    struct PQItem {
        int x,y; float f,g; int px,py;
        bool operator<(const PQItem& o) const { return f > o.f; }
    };
    std::priority_queue<PQItem> open;

    if (!pf_InBounds(S.x,S.y,W,H) || !pf_InBounds(G.x,G.y,W,H) || blocked(S.x,S.y) || blocked(G.x,G.y))
        return {};

    g[idx(S.x,S.y)] = 0.f;
    parent[idx(S.x,S.y)] = S;
    open.push({S.x,S.y, pf_H(S.x,S.y,G.x,G.y), 0.f, S.x,S.y});

    // table directions pour 8-connexe ortho
    static const int DIRS8[8][2] = {
        { 1, 0},{ 1, 1},{ 0, 1},{-1, 1},
        {-1, 0},{-1,-1},{ 0,-1},{ 1,-1}
    };

    while (!open.empty()) {
        auto cur = open.top(); open.pop();
        int ci = idx(cur.x,cur.y);
        if (closed[ci]) continue;
        closed[ci] = 1;

        if (cur.x == G.x && cur.y == G.y) {
            std::vector<Vec2i> path;
            Vec2i p = {cur.x,cur.y};
            while (!(p.x == parent[idx(p.x,p.y)].x && p.y == parent[idx(p.x,p.y)].y)) {
                path.push_back(p);
                p = parent[idx(p.x,p.y)];
            }
            path.push_back(S);
            std::reverse(path.begin(), path.end());
            return path;
        }

        Vec2i u = parent[ci];

        /* ---------- Génération des voisins ---------- */
        int N[6][2];   // max 6 voisins pour oddr
        int ncount = 0;

        if (params.oddrDiamond) {
            const int r = cur.y;

            // E / W (toujours)
            N[ncount][0] = cur.x + 1; N[ncount][1] = cur.y; ncount++; // E
            N[ncount][0] = cur.x - 1; N[ncount][1] = cur.y; ncount++; // W

            if (r & 1) {
                // lignes impaires
                N[ncount][0] = cur.x + 1; N[ncount][1] = cur.y - 1; ncount++; // NE
                N[ncount][0] = cur.x + 1; N[ncount][1] = cur.y + 1; ncount++; // SE
                N[ncount][0] = cur.x + 0; N[ncount][1] = cur.y - 1; ncount++; // NW
                N[ncount][0] = cur.x + 0; N[ncount][1] = cur.y + 1; ncount++; // SW
            } else {
                // lignes paires
                N[ncount][0] = cur.x + 0; N[ncount][1] = cur.y - 1; ncount++; // NE
                N[ncount][0] = cur.x + 0; N[ncount][1] = cur.y + 1; ncount++; // SE
                N[ncount][0] = cur.x - 1; N[ncount][1] = cur.y - 1; ncount++; // NW
                N[ncount][0] = cur.x - 1; N[ncount][1] = cur.y + 1; ncount++; // SW
            }
        } else {
            // 8-connexe orthogonal
            for (int k=0;k<8;k++) {
                N[k][0] = cur.x + DIRS8[k][0];
                N[k][1] = cur.y + DIRS8[k][1];
            }
            ncount = 8;
        }

        /* ---------- Détente des voisins ---------- */
        for (int kk=0; kk<ncount; ++kk) {
            int nx = N[kk][0];
            int ny = N[kk][1];
            if (!pf_InBounds(nx,ny,W,H)) continue;
            if (blocked(nx,ny)) continue;

            int ni = idx(nx,ny);
            if (closed[ni]) continue;

            float gCand;
            Vec2i newParent;

            if (!params.oddrDiamond && params.anyAngle) {
                // Theta* only on 8-connexe ortho
                bool canShortcut = LineOfSightSupercover(u.x,u.y,nx,ny,blocked);
                if (canShortcut) {
                    gCand = g[idx(u.x,u.y)] + pf_H(nx,ny,u.x,u.y);
                    newParent = u;
                } else {
                    int dx = nx - cur.x, dy = ny - cur.y;
                    gCand = g[ci] + pf_StepCost(dx,dy);
                    newParent = {cur.x,cur.y};
                }
            } else {
                // Graphe discret : pas de LOS
                float step = params.oddrDiamond ? 1.0f
                                                : pf_StepCost(nx - cur.x, ny - cur.y);

                // Corner-cut à éviter uniquement en 8-connexe
                if (!params.oddrDiamond && params.forbidDiagonalCornerCut) {
                    if (pf_DiagonalCornerCut(cur.x,cur.y,nx,ny,blocked))
                        continue;
                }

                gCand = g[ci] + step;
                newParent = {cur.x,cur.y};
            }

            if (gCand < g[ni]) {
                g[ni] = gCand;
                parent[ni] = newParent;
                float f = gCand + pf_H(nx,ny,G.x,G.y);
                open.push({nx,ny,f,gCand,newParent.x,newParent.y});
            }
        }
    }

    return {};
}

#endif // PATHFINDING_H
