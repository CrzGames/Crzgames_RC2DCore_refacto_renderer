#ifndef MAP_H
#define MAP_H

#include <RC2D/RC2D.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_engine.h>
#include <RC2D/RC2D_gpu.h>
#include <RC2D/RC2D_storage.h>

#include <SDL3/SDL.h>

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Modes d'agencement possibles pour la carte (map).
 *
 * Cette énumération définit les différents modes de disposition de la carte dans l'interface du jeu.
 * Chaque mode détermine comment les marges sont appliquées pour laisser de la place à des éléments UI
 * comme la minimap, le chat, les barres de menu, etc.
 *
 * \since Cette énumération est disponible depuis la version initiale du module Map.
 */
typedef enum MapLayoutMode {
    /**
     * Mode encadré : marges sur les côtés gauche/droite et un peu en haut/bas.
     * 
     * Idéal pour intégrer des éléments UI comme une minimap à droite, un chat à gauche,
     * et des barres en haut/bas. Cela crée un cadre autour de la carte pour un aspect plus structuré.
     */
    MAP_LAYOUT_FRAMED = 0,

    /**
     * Mode barre supérieure : plein écran sauf une barre en haut.
     * 
     * La carte occupe tout l'espace disponible sauf une barre supérieure pour les menus,
     * la déconnexion, etc. Cela maximise la visibilité de la carte tout en gardant un accès rapide aux options.
     */
    MAP_LAYOUT_TOP_BAR
} MapLayoutMode;

/**
 * \brief Structure représentant les marges (insets) pour la carte.
 *
 * Cette structure définit les marges à appliquer autour de la carte pour laisser de l'espace
 * aux éléments UI. Les valeurs peuvent être interprétées en pixels logiques ou en pourcentages
 * de la zone visible.
 *
 * \since Cette structure est disponible depuis la version initiale du module Map.
 */
typedef struct MapInsets {
    float left;    /**< Marge gauche (pixels logiques ou pourcentage). */
    float top;     /**< Marge supérieure (pixels logiques ou pourcentage). */
    float right;   /**< Marge droite (pixels logiques ou pourcentage). */
    float bottom;  /**< Marge inférieure (pixels logiques ou pourcentage). */
    bool  percent; /**< true si les valeurs sont des pourcentages (0..1), false si pixels logiques. */
} MapInsets;

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

/**
 * \brief Structure représentant la caméra de la carte.
 *
 * Cette structure définit la position et le zoom de la caméra pour visualiser la carte.
 * Le zoom est exprimé en pourcentage (1.0 = 100%). Les limites empêchent la caméra de sortir de la carte.
 *
 * \since Cette structure est disponible depuis la version 1.1 du module Map.
 */
typedef struct Camera {
    float x;       /**< Position X de la caméra (en pixels, dans l'espace de la carte). */
    float y;       /**< Position Y de la caméra (en pixels, dans l'espace de la carte). */
    float zoom;    /**< Facteur de zoom (1.0 = 100%, <1.0 = dézoom). */
    float minX;    /**< Limite minimale X pour la caméra. */
    float maxX;    /**< Limite maximale X pour la caméra. */
    float minY;    /**< Limite minimale Y pour la caméra. */
    float maxY;    /**< Limite maximale Y pour la caméra. */
} Camera;

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

/**
 * \class Map
 * \brief Classe gérant la carte (map) du jeu, incluant son agencement, l'effet océan, et la caméra.
 *
 * Cette classe encapsule toute la logique liée à la carte : modes d'agencement, calcul des rectangles,
 * gestion de l'effet océan animé via shader GPU, gestion de la caméra, et intégration avec les callbacks du moteur RC2D.
 * Elle est conçue pour être simple à utiliser en C++ tout en restant proche d'un style procédural.
 *
 * Utilisation typique :
 * - Instancier la classe dans le jeu.
 * - Appeler Load() au chargement.
 * - Appeler Update(dt) à chaque frame.
 * - Appeler Draw() pour le rendu.
 * - Appeler Unload() au déchargement.
 *
 * \note Les membres statiques et méthodes sont utilisés pour une simplicité maximale, mais une instance est recommandée pour une meilleure encapsulation.
 *
 * \since Cette classe est disponible depuis la version initiale du module Map.
 */
class Map {
private:
    // Constantes pour la grille de la carte
    static const int TILE_WIDTH;    /**< Largeur d'une tuile en pixels. */
    static const int TILE_HEIGHT;   /**< Hauteur d'une tuile en pixels. */
    static const int COLUMN;        /**< Nombre de colonnes de tuiles dans la carte. */
    static const int ROW;           /**< Nombre de lignes de tuiles dans la carte. */
    static const int MAP_WIDTH;     /**< Largeur totale de la carte (COLUMN * TILE_WIDTH). */
    static const int MAP_HEIGHT;    /**< Hauteur totale de la carte (ROW * TILE_HEIGHT). */
    static const float MIN_ZOOM;    /**< Zoom minimum (0.3 = 30%). */
    static const float MAX_ZOOM;    /**< Zoom maximum (1.0 = 100%). */

    // Mode courant d'agencement (modifiable à chaud via input).
    MapLayoutMode currentLayoutMode = MAP_LAYOUT_FRAMED;

    // Presets de marges pour chaque mode (constantes).
    static const MapInsets kInsetsFramed;
    static const MapInsets kInsetsTopBar;

    // Marges actives (copiées du preset selon le mode courant).
    MapInsets currentInsets = {0};

    // Rectangle final de la carte (dans l'espace logique rendu). Mis à jour à chaque update.
    SDL_FRect mapRect = {0,0,0,0};

    // Caméra pour visualiser la carte.
    Camera camera = {0};

    // Éléments pour l'effet océan.
    RC2D_Image          oceanTile        = {0};   /**< Texture de base pour l'eau (tile). */
    RC2D_GPUShader*     oceanShader      = NULL;  /**< Shader fragment pour l'animation de l'océan. */
    SDL_GPURenderState* oceanRenderState = NULL;  /**< État de rendu GPU pour l'océan (pipeline). */
    SDL_GPUSampler*     repeatSampler    = NULL;  /**< Sampler avec mode REPEAT pour le tiling. */
    OceanUniforms       oceanUniforms    = {0};   /**< Uniforms pour le shader océan. */
    double              timeSeconds      = 0.0;   /**< Horloge locale pour l'animation (en secondes). */

    /**
     * \brief Récupère les marges correspondantes à un mode d'agencement donné.
     *
     * Cette méthode retourne une copie des marges prédéfinies pour le mode spécifié.
     * Elle est utilisée internement pour mettre à jour les marges actives lors d'un changement de mode.
     *
     * \param mode Mode d'agencement pour lequel récupérer les marges.
     * \return Copie de la structure MapInsets pour ce mode.
     *
     * \since Cette méthode est disponible depuis la version initiale du module Map.
     */
    MapInsets GetInsetsForLayoutMode(MapLayoutMode mode) const;

    /**
     * \brief Calcule le rectangle final de la carte en appliquant les marges à une zone visible.
     *
     * Cette méthode soustrait les marges (insets) de la zone visible et sûre fournie,
     * en tenant compte si les marges sont en pixels logiques ou en pourcentages.
     * Le résultat est clampé pour éviter des dimensions négatives.
     *
     * \param visibleSafe Zone visible et sûre (intersection safe-area et overscan) en coordonnées logiques.
     * \param insets Marges à appliquer.
     * \return Rectangle final de la carte (jamais négatif).
     *
     * \since Cette méthode est disponible depuis la version initiale du module Map.
     */
    SDL_FRect ComputeRectFromVisibleSafeAndInsets(const SDL_FRect& visibleSafe, const MapInsets& insets) const;

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
     * \brief Met à jour la position et le zoom de la caméra.
     *
     * Cette méthode ajuste la position (x, y) ou le zoom de la caméra en fonction de l'entrée spécifiée,
     * et s'assure que la caméra reste dans les limites de la carte (basées sur mapRect).
     *
     * \param dx Déplacement en X (positif ou négatif).
     * \param dy Déplacement en Y (positif ou négatif).
     * \param dz Changement de zoom (positif ou négatif).
     *
     * \since Cette méthode est disponible depuis la version 1.1 du module Map.
     */
    void UpdateCamera(float dx, float dy, float dz);

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