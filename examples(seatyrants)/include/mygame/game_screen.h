#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <SDL3/SDL.h>

/**
 * \brief Modes d'agencement possibles pour la carte (map).
 *
 * Cette énumération définit les différents modes de disposition de la carte dans l'interface du jeu.
 * Chaque mode détermine comment les marges sont appliquées pour laisser de la place à des éléments UI
 * comme la minimap, le chat, les barres de menu, etc.
 *
 * \since Cette énumération est disponible depuis la version initiale du module Map.
 */
typedef enum GameScreenLayoutMode {
    /**
     * Mode encadré : marges sur les côtés gauche/droite et un peu en haut/bas.
     * 
     * Idéal pour intégrer des éléments UI comme une minimap à droite, un chat à gauche,
     * et des barres en haut/bas. Cela crée un cadre autour de la carte pour un aspect plus structuré.
     */
    GAME_SCREEN_LAYOUT_FRAMED = 0,

    /**
     * Mode barre supérieure : plein écran sauf une barre en haut.
     * 
     * La carte occupe tout l'espace disponible sauf une barre supérieure pour les menus,
     * la déconnexion, etc. Cela maximise la visibilité de la carte tout en gardant un accès rapide aux options.
     */
    GAME_SCREEN_LAYOUT_TOP_BAR
} GameScreenLayoutMode;

/**
 * \brief Structure représentant les marges (insets) pour la carte.
 *
 * Cette structure définit les marges à appliquer autour de la carte pour laisser de l'espace
 * aux éléments UI. Les valeurs peuvent être interprétées en pixels logiques ou en pourcentages
 * de la zone visible.
 *
 * \since Cette structure est disponible depuis la version initiale du module Map.
 */
typedef struct GameScreenInsets {
    float left;    /**< Marge gauche (pixels logiques ou pourcentage). */
    float top;     /**< Marge supérieure (pixels logiques ou pourcentage). */
    float right;   /**< Marge droite (pixels logiques ou pourcentage). */
    float bottom;  /**< Marge inférieure (pixels logiques ou pourcentage). */
    bool  percent; /**< true si les valeurs sont des pourcentages (0..1), false si pixels logiques. */
} GameScreenInsets;

class GameScreen {
    private:
        // Preset #1 : Mode encadré — place pour minimap + chat + barres
        static const GameScreenInsets kInsetsFramed;

        // Preset #2 : Mode barre supérieure — plein écran sauf une barre en haut
        static const GameScreenInsets kInsetsTopBar;

        SDL_FRect ComputeRectFromVisibleSafeAndInsets(const SDL_FRect& visibleSafe, const GameScreenInsets& inset) const;

    public:
        SDL_FRect rect;                         /**< Rectangle de la zone de jeu (game screen) en coordonnées logiques. */
        GameScreenLayoutMode currentLayoutMode; /**< Mode d'agencement courant. */
        GameScreenInsets currentInset;          /**< Marges courantes (copiées du preset selon le mode). */

        GameScreen(GameScreenLayoutMode mode);
        ~GameScreen();

        void Update(void);
        void UpdateScreenRect(void);
        void UpdateLayoutMode(GameScreenLayoutMode mode);
        GameScreenInsets GetInsetsForLayoutMode(GameScreenLayoutMode mode) const;
};

#endif // GAME_SCREEN_H