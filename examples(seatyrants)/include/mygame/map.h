#ifndef MAP_H
#define MAP_H

#include <mygame/tile_map.h>
#include <mygame/game_screen.h>
#include <mygame/camera.h>
#include <mygame/entity.h>
#include <mygame/pathfinding.h>

#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>

#include <SDL3/SDL.h>

#include <vector>

/**
 * \brief Uniforms pour le shader océan.
 *
 * Cette structure regroupe les paramètres uniformes envoyés au shader pour animer l'océan.
 * Elle est divisée en deux tableaux de floats pour une compatibilité optimale avec les shaders.
 *
 * \since Cette structure est disponible depuis la version initiale du module Map.
 */
typedef struct OceanUniforms {
    float params0[4]; /**< [time, strength, px_amp, tiling] - Temps, force de l'effet, amplitude en pixels, répétition de la texture. */
    float params1[4]; /**< [width, height, speed, extra] - Largeur de la zone, hauteur de la zone, vitesse d'animation, paramètre extra (ex. : reflet/Fresnel). */
} OceanUniforms;

typedef struct FOWUniforms {
    float p0[4]; // tiling, opacity, -, -
    float p1[4]; // unused
    float p2[4]; // unused
    float p3[4]; // tint.r, tint.g, tint.b, -
} FOWUniforms;

class Map {
    private:
        /**
        * Grille de la carte contenant les informations de chaque tuile.
        * Chaque tuile peut contenir des informations sur le terrain, les collisions,
        * et les entités présentes.
        */
        TileMap tileMap;
    
        // Écran de jeu
        GameScreen gameScreen;

        // Caméra pour visualiser la carte.
        Camera camera;

        // Atlas de sprites pour les éléments de la carte (ex: navire)
        RC2D_TP_Atlas shipAtlas = {0}; /**< Atlas TexturePacker pour les sprites du jeu (ex: navire). */

        // Éléments pour l'effet océan.
        RC2D_Image          oceanTile        = {0};   /**< Texture de base pour l'eau (tile). */
        RC2D_GPUShader*     oceanShader      = NULL;  /**< Shader fragment pour l'animation de l'océan. */
        SDL_GPURenderState* oceanRenderState = NULL;  /**< État de rendu GPU pour l'océan (pipeline). */
        SDL_GPUSampler*     repeatSampler    = NULL;  /**< Sampler avec mode REPEAT pour le tiling. */
        OceanUniforms       oceanUniforms    = {0};   /**< Uniforms pour le shader océan. */
        double              timeSeconds      = 0.0;   /**< Horloge locale pour l'animation (en secondes). */

        // Element pour l'effet brouillard de guerre (FOW).
        FOWUniforms         fowUniforms      = {0};   /**< Uniforms pour le shader FOW. */
        SDL_GPUShader*     fowShader        = NULL;  /**< Shader fragment pour le brouillard de guerre (FOW). */
        SDL_GPURenderState* fowRenderState   = NULL;  /**< État de rendu GPU pour le FOW (pipeline). */
        double              fowTimeSeconds   = 0.0;   /**< Horloge locale pour l'animation du FOW (en secondes). */
        RC2D_Image          fowNoiseTile     = {0};   /**< Texture de bruit pour le FOW. */

        /**
        * \brief Met à jour les uniforms du shader océan.
        *
        * Cette méthode incrémente l'horloge locale et met à jour les uniforms avec le temps écoulé,
        * la taille de la zone visible de la carte, etc. Elle applique ensuite les uniforms au render state.
        *
        * \param dt Temps écoulé depuis la dernière frame (en secondes).
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void UpdateOceanUniforms(double dt);

        /**
        * \brief Met à jour les uniforms du shader de brouillard de guerre (FOW).
        *
        * Cette méthode incrémente l'horloge locale et met à jour les uniforms avec le temps écoulé,
        * la taille de la zone visible de la carte, etc. Elle applique ensuite les uniforms au render state.
        *
        * \param dt Temps écoulé depuis la dernière frame (en secondes).
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void UpdateFOWUniforms(double dt);

        // NOUVEAU : Pour le ship et pathfinding
        Entity playerShip = {0};  // Ex. : id=1, type=ENTITY_PLAYER, x=0 (proj), y=0 (proj), direction=DIRECTION_SOUTH_EAST
        float shipSpeed = 100.0f;  // Pixels/seconde (espace projeté)
        int spriteToggle = 0;  // Pour alternance
        float m_stepPixels   = 10.0f;  // distance entre deux bascules de sprite
        float m_moveAccum    = 0.0f;   // accumulateur de distance parcourue

        // NOUVEAU : Choisir sprite basé sur direction/angle/health/toggle
        const char* GetShipSpriteName() const;

        // --- PATHFINDING / NAVIGATION ---
        std::vector<Vec2i> m_pathGrid;   // chemin en coordonnées grille (col,row)
        size_t              m_pathIndex = 0;
        float               m_shipSpeed = 140.0f; // px/s
        float               m_waypointEps = 4.0f; // tolérance arrivée waypoint (px)

        // Suivi de segment courant dans le chemin (odd-r)
        bool   m_hasSeg = false;
        Vec2i  m_segFrom = {0,0};
        Vec2i  m_segTo   = {0,0};
        Direction m_segDir = DIRECTION_EAST; // valeur par défaut
        size_t m_segEndIndex = 0; // fin de la run courante dans m_pathGrid

        void StartSegmentFromPath();

        // Helpers
        Vec2i WorldToGrid(float worldX, float worldY) const;
        void  GridToWorldCenter(const Vec2i& c, float& outX, float& outY) const;
        void  UpdateShipMovement(double dt);
        Direction DirectionFromVector(float dx, float dy) const;
        Direction GridStepToDirectionOddR(int cx, int cy, int nx, int ny) const;

    public:
        /**
        * \brief Constructeur par défaut de la classe Map.
        *
        * Initialise les valeurs par défaut, y compris le mode d'agencement initial, les uniforms océan,
        * et la caméra.
        *
        * \since Ce constructeur est disponible depuis la version initiale du module Map.
        */
        Map();

        /**
        * \brief Destructeur de la classe Map.
        *
        * Libère les ressources GPU si elles n'ont pas été unload manuellement.
        *
        * \since Ce destructeur est disponible depuis la version initiale du module Map.
        */
        ~Map();

        /**
        * \brief Charge les ressources nécessaires pour la carte et l'effet océan.
        *
        * Cette méthode configure le shader océan, charge la texture de l'eau, crée le sampler
        * et initialise l'état de rendu GPU. Elle est appelée une fois au démarrage du jeu.
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void Load();

        /**
        * \brief Libère les ressources utilisées par la carte et l'effet océan.
        *
        * Cette méthode détruit la texture, le shader, le sampler et l'état de rendu GPU.
        * Elle est appelée à la fin du jeu ou lors d'un rechargement.
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void Unload();

        /**
        * \brief Met à jour l'état de la carte à chaque frame.
        *
        * Cette méthode recalcule le rectangle de la carte en fonction du mode d'agencement
        * et met à jour les uniforms du shader océan.
        *
        * \param dt Temps écoulé depuis la dernière frame (en secondes).
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void Update(double dt);

        /**
        * \brief Dessine la carte et l'effet océan en utilisant la caméra.
        *
        * Cette méthode rend l'océan animé dans la zone visible de la carte, en appliquant
        * le zoom via SDL_SetRenderScale. Les éléments UI peuvent être dessinés par-dessus dans les marges.
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void Draw();

        /**
        * \brief Gère les entrées clavier pour changer le mode d'agencement et contrôler la caméra.
        *
        * Cette méthode change le mode d'agencement (framed ou top-bar) et ajuste la position/zoom
        * de la caméra en fonction des touches pressées (par exemple, touches fléchées pour déplacer,
        * '+' et '-' pour zoomer).
        *
        * \param key Nom de la touche pressée.
        * \param scancode Code de scan SDL de la touche.
        * \param keycode Code de touche SDL.
        * \param mod Modificateurs de clavier (ex. : Shift, Ctrl).
        * \param isrepeat Indique si la touche est répétée.
        * \param keyboardID Identifiant du clavier.
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void KeyPressed(const char* key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID);

        /**
        * \brief Gère les entrées de la souris pour interagir avec la carte.
        *
        * Cette méthode enregistre les clics de souris pour un traitement futur (par exemple, interactions avec la carte).
        *
        * \param x Position X du clic (coordonnées logiques).
        * \param y Position Y du clic (coordonnées logiques).
        * \param button Bouton de la souris pressé.
        * \param clicks Nombre de clics (simple, double, etc.).
        * \param mouseID Identifiant de la souris.
        *
        * \since Cette méthode est disponible depuis la version initiale du module Map.
        */
        void MousePressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID);
};

#endif // MAP_H