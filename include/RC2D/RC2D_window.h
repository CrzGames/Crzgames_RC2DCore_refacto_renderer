#ifndef RC2D_WINDOW_H
#define RC2D_WINDOW_H

#include <RC2D/RC2D_pixels.h>
#include <RC2D/RC2D_math.h>

#include <SDL3/SDL_video.h>

#include <stdbool.h> // Required for : bool
#include <stdint.h> // Required for : uint32_t

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Il s'agit d'un identifiant unique pour un écran pendant la durée de sa connexion au système, 
 * qui n'est jamais réutilisé pendant la durée de vie de l'application.
 *
 * Si l'écran est déconnecté puis reconnecté, il recevra un nouvel identifiant.
 *
 * La valeur 0 est un identifiant non valide.
 *
 * \since Ce type de données est disponible depuis RC2D 1.0.0.
 */
typedef SDL_DisplayID RC2D_DisplayID;

/**
 * \brief TODO
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_DisplayMode {
    /**
     * Identifiant de l'affichage.
     */
    RC2D_DisplayID display_id;
    RC2D_PixelFormat format;
    int width;
    int height;
    float pixel_density;
    float refresh_rate;
    int refresh_rate_numerator;
    int refresh_rate_denominator;
} RC2D_DisplayMode;

/**
 * \brief Enum représentant les orientations d'affichage.
 *
 * \since Cette enum est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_DisplayOrientation {
    /**
     * L'orientation de l'affichage ne peut pas être déterminée
     */
    RC2D_ORIENTATION_UNKNOWN,

    /**
     * L'écran est en mode paysage, avec le côté droit vers le haut, 
     * par rapport au mode portrait.
     */
    RC2D_ORIENTATION_LANDSCAPE,

    /**
     * L'affichage est en mode paysage, avec le côté gauche vers le haut, 
     * par rapport au mode portrait.
     */
    RC2D_ORIENTATION_LANDSCAPE_FLIPPED,

    /**
     * L'écran est en mode portrait
     */
    RC2D_ORIENTATION_PORTRAIT,

    /**
     * L'écran est en mode portrait, à l'envers.
     */
    RC2D_ORIENTATION_PORTRAIT_FLIPPED
} RC2D_DisplayOrientation;

/**
 * \brief Enum représentant les types de mode plein écran.
 *
 * \since Cette enum est disponible depuis RC2D 1.0.0.
 */
typedef enum RC2D_FullscreenType {
    /**
     * Fenêtre standard, sans plein écran.
     */
    RC2D_FULLSCREEN_NONE,

    /**
     * Plein écran fenêtré sans bordure (aussi appelé "fullscreen desktop").
     *
     * - Utilise la résolution actuelle du bureau sans changer la résolution réelle de l'affichage.
     * - Ne prend pas le contrôle exclusif de l'affichage, ce qui permet des transitions fluides vers le bureau ou d'autres applications.
     * - Convient pour la majorité des jeux et logiciels ne nécessitant pas une résolution spécifique.
     */
    RC2D_FULLSCREEN_BORDERLESS,

    /**
     * Plein écran exclusif, permettant de changer la résolution de l'écran.
     *
     * - Utilise le meilleur mode d'affichage disponible pour le moniteur qui contient la fenêtre.
     * - Ce mode offre potentiellement de meilleures performances, notamment sur les systèmes embarqués ou les jeux demandant une résolution fixe.
     * - L'affichage passe en "mode natif" et l'application prend le contrôle total de l'écran.
     * - Peut entraîner un effet de scintillement temporaire lors de la transition.
     */
    RC2D_FULLSCREEN_EXCLUSIVE
} RC2D_FullscreenType;

/**
 * \brief Structure représentant les informations sur le mode plein écran.
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_FullscreenInfo {
    /**
     * Indique si la fenêtre est en mode plein écran ou non.
     */
    bool is_fullscreen;

    /**
     * Indique si la fenêtre est en mode plein écran exclusif ou non.
     */
    RC2D_FullscreenType type;
} RC2D_FullscreenInfo;

/**
 * \brief Définit le titre de la fenêtre de l'application.
 *
 * \param {const char *} title - Le titre de la fenêtre.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getTitle
 */
void rc2d_window_setTitle(const char *title);

/**
 * \brief Configure le mode plein écran de la fenêtre.
 *
 * \param {bool} fullscreen - Si vrai, active le mode plein écran. Sinon, désactive le mode plein écran.
 * \param {RC2D_FullscreenType} type - Le type de mode plein écran à utiliser.
 * \param {bool} syncWindow - Sur les systèmes de fenêtrage asynchrones, cela agit comme une barrière de synchronisation 
 * pour l'état des fenêtres en attente. L'application attendra que l'état de fenêtre en attente soit appliqué et le retour 
 * sera garanti dans un délai limité. Notez que la durée de blocage potentiel dépend du système de fenêtrage sous-jacent, 
 * car les changements d'état peuvent impliquer des animations assez longues qui doivent se terminer avant que la fenêtre 
 * n'atteigne son état final. Sur les systèmes de fenêtrage où les changements sont immédiats, cela ne fait rien.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getFullscreen
 */
void rc2d_window_setFullscreen(const bool fullscreen, const RC2D_FullscreenType type, const bool syncWindow);

/**
 * \brief Active ou désactive la synchronisation verticale (VSync).
 *
 * La synchronisation verticale permet de synchroniser le taux de rafraîchissement 
 * de l'affichage du moniteur, avec le taux de rafraîchissement de la fenêtre pour 
 * réduire le déchirement de l'image.
 *
 * \param {bool} vsync - true pour activer la synchronisation verticale, false pour la désactiver.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getVSync
 */
void rc2d_window_setVSync(const bool vsync);

/**
 * \brief Définit la taille de la fenêtre de l'application.
 * 
 * Cette fonction ajuste la taille de la fenêtre à la largeur et à la hauteur spécifiées.
 *
 * \param {int} width - La nouvelle largeur de la fenêtre en pixels.
 * \param {int} height - La nouvelle hauteur de la fenêtre en pixels.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getWidth
 * \see rc2d_window_getHeight
 */
void rc2d_window_setSize(const int width, const int height);

/**
 * \brief Définit la position de la fenêtre sur l'écran.
 * 
 * \details La position de la fenêtre est dans l'espace de coordonnées 
 * du moniteur contenant la fenêtre.
 * 
 * \param {int} x - La coordonnée x de la position de la fenêtre.
 * \param {int} y - La coordonnée y de la position de la fenêtre.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getPosition
 */
void rc2d_window_setPosition(const int x, const int y);

/**
 * TODO: A REVOIR
 * 
 * \brief Récupère le nombre de moniteurs connectés.
 *
 * \return {int} - Le nombre de moniteurs connectés.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
int rc2d_window_getDisplayCount(void);

/**
 * \brief Récupère le nom du moniteur spécifié.
 *
 * \param {int} displayID - L'ID du moniteur à récupérer.
 * \return {const char *} - Le nom du moniteur.
 * 
 * \warning Le pointeur retourné est géré par SDL et ne doit pas être libéré par l'appelant.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getDisplayCount
 * \see rc2d_window_getDisplayForWindow
 */
const char *rc2d_window_getDisplayName(int displayID);

/**
 * \brief Récupère l'orientation actuelle du moniteur principal qui contient la fenêtre.
 *
 * \return {RC2D_DisplayOrientation} - L'orientation actuelle du moniteur principal
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getDisplayCount
 * \see rc2d_window_getDisplayName
 * \see rc2d_window_getDisplayForWindow
 */
RC2D_DisplayOrientation rc2d_window_getDisplayOrientation(void);

/**
 * \brief Récupère le titre de la fenêtre.
 *
 * \return {const char *} - Le titre de la fenêtre.
 * 
 * \warning Le pointeur retourné est géré par SDL et ne doit pas être libéré par l'appelant.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_setTitle
 */
const char *rc2d_window_getTitle(void);

/**
 * \brief Permet de savoir si la synchronisation verticale (VSync) est activée.
 *
 * \return {bool} - true si VSync est activé, sinon false.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_setVSync
 */
bool rc2d_window_getVSync(void);

/**
 * \brief Récupère la position de la fenêtre à l'écran.
 *
 * \param {int *} x - La coordonnée x de la position de la fenêtre.
 * \param {int *} y - La coordonnée y de la position de la fenêtre.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_setPosition
 */
void rc2d_window_getPosition(int *x, int *y);

/**
 * \brief Permet de savoir si la fenêtre est en mode plein écran.
 *
 * \return {RC2D_FullscreenInfo} - Informations sur le mode plein écran de la fenêtre.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_setFullscreen
 */
RC2D_FullscreenInfo rc2d_window_getFullscreen(void);

/**
 * \brief Récupère la hauteur actuelle de la fenêtre de l'application.
 *
 * \return {int} - La hauteur de la fenêtre en pixels.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getWidth
 * \see rc2d_window_getSize
 */
int rc2d_window_getHeight(void);

/**
 * \brief Récupère la largeur actuelle de la fenêtre de l'application.
 *
 * \return {int} - La largeur de la fenêtre en pixels.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getHeight
 * \see rc2d_window_getSize
 */
int rc2d_window_getWidth(void);

/**
 * \brief Restaure la fenêtre à sa taille et position d'origine si elle était minimisée ou maximisée.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_window_restore(void);

/**
 * \brief Vérifie si la fenêtre est minimisée.
 *
 * \return {bool} - true si la fenêtre est minimisée, sinon false.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_isMinimized
 */
bool rc2d_window_isMaximized(void);

/**
 * \brief Vérifie si la fenêtre est maximisée.
 *
 * \return {bool} - true si la fenêtre est maximisée, sinon false.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_isMaximized
 */
bool rc2d_window_isMinimized(void);

/**
 * \brief Vérifie si la fenêtre du jeu est visible.
 *
 * \return {bool} - true si la fenêtre est visible, sinon false.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_window_isVisible(void);

/**
 * \brief Maximise la fenêtre.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_isMaximized
 */
void rc2d_window_maximize(void);

/**
 * \brief Minimise la fenêtre.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_isMinimized
 */
void rc2d_window_minimize(void);

/**
 * \brief Vérifie si la fenêtre du jeu a le focus du clavier.
 *
 * \return {bool} - true si la fenêtre a le focus du clavier, sinon false.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_hasMouseFocus
 */
bool rc2d_window_hasKeyboardFocus(void);

/**
 * \brief Vérifie si la fenêtre du jeu a le focus de la souris.
 *
 * \return {bool} - true si la fenêtre a le focus de la souris, sinon false.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_hasKeyboardFocus
 */
bool rc2d_window_hasMouseFocus(void);

/**
 * \brief Récupère le facteur de mise à l'échelle DPI (HiDPI) de la fenêtre.
 *
 * Sur les écrans haute densité (comme Retina sur macOS), la surface de rendu peut
 * avoir plus de pixels que la taille logique de la fenêtre. Ce facteur représente
 * le ratio entre la taille réelle en pixels et la taille logique.
 *
 * Par exemple, si la fenêtre est définie comme 800x600 mais utilise 1600x1200 pixels
 * de rendu, cette fonction retournera 2.0.
 *
 * \return Le facteur d'échelle DPI associé à la fenêtre (ex: 1.0, 2.0...).
 *         Retourne 0.0f si la fenêtre n'est pas valide ou si la récupération échoue.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Disponible depuis RC2D 1.0.0
 * 
 * \see rc2d_window_getDisplayScale
 * \see rc2d_window_getSizeInPixels
 * \see rc2d_window_getContentScale
 * \see rc2d_window_getSize
 */
float rc2d_window_getPixelDensity(void);

/**
 * \brief Récupère la taille logique de la fenêtre (en coordonnées natives, pas en pixels réels).
 *
 * Cette fonction renvoie la largeur et la hauteur actuelles de la fenêtre en unités logiques,
 * c’est-à-dire sans prendre en compte la densité de pixels ou l’échelle de rendu.
 *
 * Sur les écrans haute densité (HiDPI, Retina...), cette taille est souvent plus petite
 * que la taille réelle en pixels, car l’utilisateur attend une interface plus lisible.
 *
 * \note Les pointeurs peuvent être NULL si la valeur n’est pas désirée.
 * 
 * \param {int *} width - Pointeur vers un entier où sera stockée la largeur de la fenêtre (en unités logiques).
 * \param {int *} height - Pointeur vers un entier où sera stockée la hauteur de la fenêtre (en unités logiques).
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_getSizeInPixels
 * \see rc2d_window_getDisplayScale
 * \see rc2d_window_getPixelDensity
 */
void rc2d_window_getSize(int *width, int *height);

/**
 * \brief Récupère la taille réelle de la fenêtre en pixels physiques (HiDPI).
 *
 * Cette fonction renvoie la taille de la surface de rendu réelle utilisée par le GPU.
 * Elle est utile pour adapter manuellement ton rendu à la densité de pixels, surtout
 * sur des écrans Retina ou HiDPI. Par exemple, une fenêtre de 800x600 avec un facteur
 * de pixel density de 2.0 donnera une taille physique de 1600x1200.
 *
 * \note Les pointeurs peuvent être NULL si la valeur n’est pas désirée.
 * 
 * \param {int *} width - Pointeur vers un entier où sera stockée la largeur physique (en pixels).
 * \param {int *} height - Pointeur vers un entier où sera stockée la hauteur physique (en pixels).
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_getSize
 * \see rc2d_window_getPixelDensity
 * \see rc2d_window_getDisplayScale
 * \see rc2d_window_getContentScale
 */
void rc2d_window_getSizeInPixels(int *width, int *height);

/**
 * \brief Récupère le facteur de mise à l’échelle d’affichage appliqué à la fenêtre.
 *
 * Cette valeur représente l’échelle d’affichage de l’OS (souvent définie par l’utilisateur)
 * pour améliorer la lisibilité des interfaces sur les écrans à haute densité.
 *
 * Par exemple :
 * - Windows avec 200% de mise à l’échelle.
 * - macOS Retina avec le flag High Pixel Density.
 * Cette fonction retournera 2.0.
 *
 * Cela permet d’ajuster la taille logique des éléments de l’UI sans sacrifier la qualité visuelle.
 *
 * \return {float} - Le facteur de mise à l’échelle (ex: 1.0f, 2.0f, etc.). Retourne 0.0f par défaut si échec.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_getPixelDensity
 * \see rc2d_window_getSize
 * \see rc2d_window_getSizeInPixels
 * \see rc2d_window_getContentScale
 */
float rc2d_window_getDisplayScale(void);

/**
 * \brief Récupère le facteur de mise à l’échelle du contenu de la fenêtre.
 *
 * Cette valeur est le produit de rc2d_window_getPixelDensity() et rc2d_window_getDisplayScale().
 * Le calcul est fait ainsi : pixel_density * display_scale.
 *
 * Elle permet de déterminer à quel point les éléments d’interface doivent être agrandis
 * pour être lisibles et nets sur des écrans HiDPI ou 4K.
 *
 * Exemples :
 * - macOS Retina avec pixelDensity = 2.0 et displayScale = 1.0 → contentScale = 2.0
 * - Windows 4K avec 200% de mise à l’échelle → displayScale = 2.0, pixelDensity = 1.0 → contentScale = 2.0
 *
 * \return {float} - Le facteur de mise à l’échelle du contenu (ex: 1.0f, 2.0f, etc.). Retourne 0.0f par défaut si échec.
 *
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \see rc2d_window_getDisplayScale
 * \see rc2d_window_getPixelDensity
 * \see rc2d_window_getSize
 * \see rc2d_window_getSizeInPixels
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
float rc2d_window_getContentScale(void);

/**
 * @brief Récupère la zone sûre (safe area) de la fenêtre actuelle.
 * 
 * Cette fonction permet d'obtenir la région de la fenêtre qui est garantie d'être visible à l'utilisateur,
 * c'est-à-dire non obstruée par des éléments système comme :
 * - l'encoche (notch) d’un iPhone ou d’un appareil Android moderne,
 * - les bords arrondis de certains écrans,
 * - les barres de navigation système ou de titre masquant partiellement l'affichage.
 * 
 * Cela est particulièrement utile pour positionner des éléments d’interface utilisateur critiques
 * (boutons, textes, HUD) sans qu’ils soient partiellement cachés.
 * 
 * Sur les plateformes où cette information n’est pas disponible, la zone retournée couvrira l’intégralité
 * de la fenêtre.
 *
 * \param {RC2D_Rect *} rect - Pointeur vers un rectangle où sera stockée la zone sûre.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_getWidth
 * \see rc2d_window_getHeight
 */
void rc2d_window_getSafeArea(RC2D_Rect *rect);

/**
 * \brief Définit si la fenêtre peut être redimensionnée par l'utilisateur.
 *
 * Cette fonction permet d'autoriser ou d'interdire le redimensionnement de la fenêtre par l'utilisateur. 
 * Sur les plateformes qui supportent cette fonctionnalité, cela affecte l'apparence de la bordure de la fenêtre
 * et permet à l'utilisateur de changer sa taille manuellement avec la souris ou le doigt.
 *
 * Cela peut être utile pour des applications dont la taille d'affichage doit être figée, comme certains jeux.
 *
 * \param {bool} resizable - true pour autoriser le redimensionnement, false pour le désactiver.
 *
 * \since This function is available since RC2D 1.0.0.
 *
 * \see rc2d_window_isResizable
 */
void rc2d_window_setResizable(bool resizable);

/**
 * \brief Indique si la fenêtre actuelle est redimensionnable.
 *
 * Retourne true si l'utilisateur peut redimensionner la fenêtre à l'aide de la souris ou du doigt.
 * Cela reflète l'état défini précédemment par `rc2d_window_setResizable` ou lors de la création de la fenêtre.
 *
 * \return {bool} - true si la fenêtre est redimensionnable, false sinon.
 *
 * \since This function is available since RC2D 1.0.0.
 *
 * \see rc2d_window_setResizable
 */
bool rc2d_window_isResizable(void);

/**
 * \brief Définit si la fenêtre doit rester toujours au-dessus des autres fenêtres.
 *
 * Lorsque activé, la fenêtre restera visible au-dessus des autres fenêtres, même si elle n'a pas le focus.
 * Cela est utile pour les fenêtres flottantes, HUDs, ou des overlays persistants.
 *
 * \param {bool} enable - true pour activer le mode "toujours au-dessus", false pour le désactiver.
 *
 * \since This function is available since RC2D 1.0.0.
 */
void rc2d_window_setAlwaysOnTop(bool enable);

/**
 * \brief Active ou désactive la capture de la souris pour la fenêtre actuelle.
 *
 * Lorsque la capture de souris est activée (`true`), tous les événements de souris
 * sont redirigés vers la fenêtre, même si le curseur sort visuellement de ses limites.
 *
 * Cela permet d’implémenter des comportements comme la rotation libre de caméra
 * dans un jeu à la première personne ou les mouvements illimités dans les éditeurs 3D.
 *
 * \note À ne pas confondre avec `rc2d_window_setMouseCaptured`, qui permet la réception
 *       d'événements même lorsque le curseur sort de la fenêtre, sans l’enfermer dedans.
 *       `rc2d_window_setMouseGrabbed` restreint visuellement le curseur à l’intérieur de la fenêtre.
 *
 * \param {bool} grabbed - true pour capturer la souris, false pour relâcher la capture.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_isMouseGrabbed
 */
void rc2d_window_setMouseGrabbed(bool grabbed);

/**
 * \brief Vérifie si la fenêtre actuelle a capturé la souris.
 *
 * Retourne `true` si la souris est actuellement capturée par la fenêtre, 
 * c’est-à-dire que les événements de souris ne sortent pas de la fenêtre même si le curseur le fait.
 *
 * \note À ne pas confondre avec `rc2d_window_isMouseCaptured`, qui garantit la réception
 *       des événements de souris même en dehors de la fenêtre, sans contraindre visuellement le curseur.
 *
 * \return {bool} - true si la souris est capturée, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_setMouseGrabbed
 */
bool rc2d_window_isMouseGrabbed(void);

/**
 * \brief Active ou désactive la capture du clavier pour la fenêtre actuelle.
 *
 * Cela permet d’assigner tous les événements clavier à la fenêtre même si celle-ci n’a pas le focus système.
 * C’est utile dans les contextes spécifiques comme les applications en kiosque, les émulateurs ou le multifenêtrage.
 *
 * \param {bool} grabbed - true pour capturer le clavier, false pour relâcher la capture.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_isKeyboardGrabbed
 */
void rc2d_window_setKeyboardGrabbed(bool grabbed);

/**
 * \brief Vérifie si la fenêtre actuelle a capturé le clavier.
 *
 * Retourne `true` si la fenêtre a capturé l'entrée clavier,
 * indépendamment du focus système.
 *
 * \return {bool} - true si le clavier est capturé, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_setKeyboardGrabbed
 */
bool rc2d_window_isKeyboardGrabbed(void);

/**
 * \brief Vérifie si la souris est capturée par la fenêtre actuelle.
 *
 * Cette fonction indique si la fenêtre actuelle a activé le mode de capture de la souris,
 * c’est-à-dire que tous les événements de souris sont redirigés vers la fenêtre,
 * même si le curseur sort de ses limites.
 *
 * \note À ne pas confondre avec `rc2d_window_isMouseGrabbed`, qui restreint visuellement
 *       le curseur à l'intérieur de la fenêtre. La capture permet la réception d’événements
 *       même hors de la fenêtre.
 * 
 * \return {bool} - true si la souris est capturée, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_setMouseCaptured
 */
bool rc2d_window_isMouseCaptured(void);

/**
 * \brief Active ou désactive la capture de la souris.
 *
 * Lorsqu’elle est activée, tous les événements de souris sont dirigés vers la fenêtre,
 * même si le curseur sort de ses limites. Cela est utile pour les FPS ou les éditeurs 3D.
 * 
 * \note À ne pas confondre avec `rc2d_window_setMouseGrabbed`, qui enferme visuellement
 *       le curseur dans la fenêtre. `rc2d_window_setMouseCaptured` ne l’empêche pas de sortir,
 *       mais garantit la réception d’événements.
 * 
 * \param {bool} capture - true pour activer la capture, false pour la désactiver.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_isMouseCaptured
 */
void rc2d_window_setMouseCaptured(bool capture);

/**
 * \brief Vérifie si la fenêtre utilise le mode relatif de la souris.
 *
 * En mode relatif, les mouvements de la souris sont rapportés sous forme de décalages
 * depuis la dernière position, et le curseur est masqué.
 *
 * \return {bool} - true si le mode relatif est activé, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_setMouseRelativeMode
 */
bool rc2d_window_isMouseInRelativeMode(void);

/**
 * \brief Active ou désactive le mode relatif de la souris.
 *
 * Ce mode est utilisé pour obtenir des mouvements de souris relatifs sans limite d’écran,
 * utile notamment pour les jeux à la première personne.
 *
 * \param {bool} enabled - true pour activer, false pour désactiver.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 * 
 * \see rc2d_window_isMouseInRelativeMode
 */
void rc2d_window_setMouseRelativeMode(bool enabled);

/**
 * \brief Vérifie si la fenêtre est actuellement masquée derrière d’autres fenêtres (occlusion).
 *
 * Une fenêtre est considérée comme *occluded* si elle est entièrement couverte
 * par d’autres fenêtres ou n’est pas visible à l’écran, bien qu’elle soit techniquement ouverte.
 *
 * Cela peut être utilisé pour suspendre certaines opérations graphiques coûteuses
 * (comme le rendu) si la fenêtre n’est pas visible à l’utilisateur.
 *
 * \return {bool} - true si la fenêtre est masquée (occluded), false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_window_isVisible
 * \see rc2d_window_isMinimized
 */
bool rc2d_window_isOccluded(void);

/**
 * \brief Vérifie si la fenêtre ne peut pas recevoir le focus.
 *
 * Cela peut être utilisé pour créer des fenêtres non interactives, comme certains overlays ou HUD.
 *
 * \return {bool} - true si la fenêtre est non-focusable, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_window_isNotFocusable(void);

/**
 * \brief Récupère l'ID de l'affichage contenant la fenêtre actuelle.
 *
 * Cette fonction retourne l'identifiant de l'affichage sur lequel la fenêtre est actuellement affichée.
 * Cela peut être utile pour déterminer sur quel écran l'application est exécutée, surtout dans un environnement multi-écran.
 *
 * \return {RC2D_DisplayID} - L'ID de l'affichage contenant la fenêtre actuelle.
 *                            0 si la fenêtre n'est pas valide ou si l'identifiant ne peut pas être récupéré.
 *
 * \since This function is available since RC2D 1.0.0.
 * 
 * \see rc2d_window_getDisplayName
 * \see rc2d_window_getDisplayOrientation
 */
RC2D_DisplayID rc2d_window_getDisplayForWindow(void);

/**
 * \brief Récupère le pointeur vers la fenêtre SDL associée à la fenêtre RC2D.
 *
 * \return {SDL_Window*} - Pointeur vers la fenêtre SDL associée à la fenêtre RC2D.
 * 
 * \warning Le pointeur retourné est géré par RC2D et ne doit pas être libéré par l'appelant.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
SDL_Window* rc2d_window_getWindow(void);

/**
 * \brief Définit la taille minimale de la fenêtre.
 *
 * \param {int} width - La largeur minimale de la fenêtre en pixels.
 * \param {int} height - La hauteur minimale de la fenêtre en pixels.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_window_setMinimumSize(int width, int height);

/**
 * \brief Définit la taille maximale de la fenêtre.
 *
 * \param {int} width - La largeur maximale de la fenêtre en pixels.
 * \param {int} height - La hauteur maximale de la fenêtre en pixels.
 * 
 * \threadsafety Cette fonction ne doit être appelée que sur le thread principal.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_window_setMaximumSize(int width, int height);

/* Termine les définitions de fonctions C lors de l'utilisation de C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_WINDOW_H