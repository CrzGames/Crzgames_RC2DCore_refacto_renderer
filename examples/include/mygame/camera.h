#ifndef CAMERA_H
#define CAMERA_H

#include <mygame/game_screen.h>
#include <mygame/tile_map.h>

#define CAMERA_DEFAULT_ZOOM 1.0f   /**< Zoom par défaut (100%). */
#define CAMERA_MIN_ZOOM     0.6f   /**< Zoom minimum (60%). */
#define CAMERA_MAX_ZOOM     1.0f   /**< Zoom maximum (100%). */
#define CAMERA_MOVE_SPEED   500.0f /**< Vitesse de déplacement de la caméra en pixels par seconde. */
#define CAMERA_ZOOM_STEP    0.1f   /**< Incrément de zoom par pression (10%). */

/**
 * \brief Configuration complète de la caméra.
 *
 * Contient la position, le zoom et les bornes de déplacement
 * calculées selon la taille de la carte et de l'écran de jeu.
 */
typedef struct CameraConfig {
    float x;        /**< Position X initiale de la caméra (en pixels, dans l'espace de la carte). */
    float y;        /**< Position Y initiale de la caméra (en pixels, dans l'espace de la carte). */
    float zoom;     /**< Facteur de zoom initial (1.0 = 100%, <1.0 = dézoom). */
    float minX;     /**< Limite minimale X pour la caméra. */
    float maxX;     /**< Limite maximale X pour la caméra. */
    float minY;     /**< Limite minimale Y pour la caméra. */
    float maxY;     /**< Limite maximale Y pour la caméra. */
} CameraConfig;

class Camera {
    private:
        const GameScreen* gameScreen = nullptr; /**< Référence vers l’écran de jeu. */
        const TileMap*    tileMap    = nullptr; /**< Référence vers la carte du monde. */

        void RecalculateVisibleBounds(void);
        void ClampPositionWithinBounds(void);

    public:
        CameraConfig config; /**< Données de configuration de la caméra. */

        Camera(const GameScreen& gameScreen, const TileMap& tileMap, CameraConfig config = {});
        ~Camera();

        /**
        * \brief Déplace la caméra selon un delta en pixels dans l’espace du monde.
        * \param deltaX Mouvement horizontal (en pixels).
        * \param deltaY Mouvement vertical (en pixels).
        */
        void MoveCamera(float deltaX, float deltaY);

        /**
        * \brief Gère le déplacement caméra à partir des touches fléchées.
        * \param deltaTime Temps écoulé depuis la dernière frame (en secondes).
        */
        void HandleKeyboardMovement(float deltaTime);

        /**
        * \brief Modifie le niveau de zoom de la caméra et recalcule les bornes.
        * \param zoomDelta Variation de zoom (positive = zoom avant, négative = zoom arrière).
        */
        void AdjustZoom(float zoomDelta);
};

#endif // CAMERA_H