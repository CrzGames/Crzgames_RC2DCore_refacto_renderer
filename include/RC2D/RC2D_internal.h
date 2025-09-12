#ifndef RC2D_INTERNAL_H
#define RC2D_INTERNAL_H

#include <RC2D/RC2D_engine.h>
#include <RC2D/RC2D_math.h>
#include <RC2D/RC2D_gpu.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>

#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

/**
 * IMPORTANT: 
 * Module interne de la bibliothèque RC2D.
 * Pour ne pas exposer certaines fonctions, struct, enums et variables à l'utilisateur final.
 */

/* Configuration pour les définitions de fonctions C, même lors de l'utilisation de C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Structure représentant une entrée de shader dans le moteur RC2D.
 *
 * Cette structure est utilisée pour gérer les shaders chargés par le moteur,
 * y compris leur nom de fichier, le pointeur vers le shader chargé et le timestamp
 * de la dernière modification du fichier.
 * 
 * \note Cela est utilisé pour le rechargement à chaud des shaders via RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
 * si défini à 1.
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_GraphicsShaderEntry {
    /**
     * Nom du fichier du shader (e.g., "test.vertex" ou "test.fragment").
     * 
     * IMPORTANT:
     * \note Le pointeur doit être libéré en interne dans RC2D.
     */
    const char* filename;
    
    /**
     * Pointeur vers le shader graphique chargé.
     * 
     * IMPORTANT:
     * \note Ce pointeur pointe vers le shader graphique de l'utilisateur,
     * il ne doit pas être libéré manuellement en interne, mais par l'utilisateur
     * lorsqu'il n'est plus nécessaire, via : SDL_ReleaseGPUShader().
     */
    RC2D_GPUShader* shader;

    /**
     * Pointeur vers l'état de rendu GPU associé (pour l'API Renderer GPU).
     * 
     * IMPORTANT:
     * \note Ce pointeur doit être libéré via SDL_DestroyGPURenderState().
     */
    SDL_GPURenderState* gpu_render_state;

    /**
     * Timestamp de la dernière modification du fichier shader.
     */
    SDL_Time lastModified;
} RC2D_GraphicsShaderEntry;

/**
 * \brief Structure regroupant l'état global du moteur RC2D.
 *
 * Cette structure encapsule toutes les variables nécessaires pour gérer l'état du moteur,
 * y compris la configuration, les ressources SDL, et les paramètres d'exécution.
 *
 * \since Cette structure est disponible depuis RC2D 1.0.0.
 */
typedef struct RC2D_EngineState {
    // Pointeur vers la configuration utilisateur
    RC2D_EngineConfig* config;

    // SDL : Window
    SDL_Window* window;

    // SDL : Renderer
    SDL_Renderer* renderer;

    // SDL_mixer
    MIX_Mixer* mixer;

    // SDL_ttf
    TTF_TextEngine* text_engine;

    /**
    * Les données après avoir calculer pour overscan / letterbox +
    * safe area (zone visible et interactive garantie)
    */
    int logical_w, logical_h;
    SDL_FRect visible_safe_rect;

    /**
     * SDL GPU
     * 
     * Cette structure contient :
     * - Pointeur vers le périphérique GPU SDL (SDL_GPUDevice)
     * - Composition de la swapchain GPU (SDL_GPUSwapchainComposition)
     * - Mode de présentation du GPU (SDL_GPUPresentMode)
     */
    SDL_GPUDevice* gpu_device;
    SDL_GPUSwapchainComposition gpu_swapchain_composition;
    SDL_GPUPresentMode gpu_present_mode;

    /**
     * Mise en cache des shaders graphiques
     * 
     * Cette structure contient :
     * - Tableau dynamique des shaders graphics (vertex/fragment) chargés
     * - Nombre de shaders graphics (vertex/fragment) chargés
     * - Mutex pour protéger l'accès aux shaders graphics (vertex/fragment) chargés
     */
    RC2D_GraphicsShaderEntry* gpu_graphics_shaders_cache;
    int gpu_graphics_shader_count;
    SDL_Mutex* gpu_graphics_shader_mutex;

    // RC2D : État d'exécution
    int fps;
    double delta_time;
    bool game_is_running;
    Uint64 last_frame_time;
} RC2D_EngineState;

/**
 * \brief Instance globale du moteur RC2D.
 *
 * Cette instance contient toutes les informations nécessaires pour gérer l'état
 * et le comportement du moteur RC2D tout au long de l'exécution de l'application.
 *
 * \since Cette variable est disponible depuis RC2D 1.0.0.
 */
extern RC2D_EngineState rc2d_engine_state;

/**
 * \brief Récupère le périphérique GPU utilisé par RC2D.
 * 
 * \return {RC2D_GPUDevice*} Pointeur vers le périphérique GPU SDL utilisé par RC2D.
 * 
 * \threadsafety Cette fonction peut être appelée depuis n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
RC2D_GPUDevice* rc2d_gpu_getDevice(void);

/**
 * \brief Crée le moteur de texte SDL_ttf pour le renderer RC2D.
 *
 * Le moteur de texte est nécessaire pour créer et dessiner des objets texte.
 *
 * \return {bool} true si le moteur a été créé avec succès, false sinon.
 *
 * \threadsafety Cette fonction doit être appelée sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_graphics_destroyRendererTextEngine()
 */
bool rc2d_graphics_createRendererTextEngine(void);

/**
 * \brief Détruit le moteur de texte SDL_ttf associé au renderer RC2D.
 *
 * Cette fonction doit être appelée avant la fermeture du moteur RC2D pour libérer
 * correctement les ressources du moteur de texte.
 *
 * \threadsafety Cette fonction doit être appelée sur le thread principal.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 *
 * \see rc2d_graphics_createRendererTextEngine()
 */
void rc2d_graphics_destroyRendererTextEngine(void);

/**
 * \brief Hot-reload des shaders graphiques (fragment).
 * 
 * Cette fonction utilise SDL_GetPathInfo pour obtenir les informations sur le fichier
 * spécifié, si un shader graphique a été modifié depuis sa dernière compilation,
 * alors il doit être recompilé.
 * 
 * \param {const char*} path - Chemin du fichier dont on veut obtenir le temps de modification.
 * \return {SDL_Time} - Temps de modification du fichier, ou 0 en cas d'erreur.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_gpu_hotReloadGraphicsShaders(void);

/**
 * \brief Initialise le moteur RC2D.
 * 
 * Cette fonction initialise les bibliothèques nécessaires, crée la fenêtre et le GPU, 
 * et configure les paramètres de l'application.
 * 
 * \return true si l'engine a été initialisé avec succès, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_engine_init(void);

/**
 * \brief Libère les ressources allouées par le moteur RC2D.
 * 
 * Cette fonction doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_engine_quit(void);

/**
 * \brief Traite les événements SDL3.
 *
 * Cette fonction traite les événements SDL3 et appelle les callbacks appropriés.
 * 
 * \param {SDL_Event*} event - Pointeur vers l'événement SDL à traiter.
 * \return {SDL_AppResult} - Le résultat du traitement des événements.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
SDL_AppResult rc2d_engine_processevent(SDL_Event *event);

/**
 * \brief Capture le temps au début de la frame actuelle et donc calcule 
 * le delta time à chaque frame.
 * 
 * \note Cette fonction doit être appelée en premier dans la callback principale 
 * de l'application (SDL_AppIterate) pour calculer le delta time.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_engine_deltatime_start(void);

/**
 * \brief Permet de faire une pause entre chaque frame pour atteindre le FPS cible.
 * 
 * Elle permet de fixer le tickrate de la callback principale de l'application (SDL_AppIterate).
 * 
 * \note Si SDL_HINT_MAIN_CALLBACK_RATE est défini, cette fonction ne fait rien.
 * Sinon, elle attend le temps nécessaire pour atteindre le FPS cible.
 * SDL_HINT_MAIN_CALLBACK_RATE peux ne pas marcher sur toutes les plateformes.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_engine_deltatime_end(void);

/**
 * \brief Configure le moteur RC2D avec les paramètres spécifiés.
 * 
 * Cette fonction configure le moteur RC2D en utilisant la structure de configuration fournie.
 * Elle doit être appelée avant d'initialiser le moteur. 
 * 
 * \note si config == NULL, la configuration par défaut sera utilisée.
 * 
 * \param {RC2D_Config*} config - Pointeur vers la structure de configuration à utiliser.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_engine_configure(const RC2D_EngineConfig* config);

/**
 * \brief Initialisation de l'assertion RC2D.
 * 
 * Cette fonction initialise le module d'assertion de RC2D. 
 * Elle doit être appelée avant d'utiliser les macros d'assertion.
 * 
 * \threadsafety Il est possible d'appeler cette fonction en toute sécurité à partir de n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_assert_init(void);

/**
 * \brief Initialise le timer de haute précision utilisé pour mesurer le temps écoulé. 
 * 
 * Cette fonction est appelée automatiquement lors de l'initialisation du système de timer de RC2D 
 * et ne doit généralement pas être appelée directement.
 * 
 * \threadsafety Il est possible d'appeler cette fonction en toute sécurité à partir de n'importe quel thread.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_timer_init(void);

#if RC2D_ONNX_MODULE_ENABLED
/**
 * \brief Initialise le module ONNX de RC2D.
 * 
 * Cette fonction initialise le module ONNX de RC2D. Elle doit être appelée avant d'utiliser les fonctionnalités ONNX.
 * 
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
bool rc2d_onnx_init(void);

/**
 * \brief Libère les ressources allouées par le module ONNX de RC2D.
 * 
 * Cette fonction doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_onnx_cleanup(void);
#endif

/**
 * Libère les ressources allouées par le module de système de fichiers.
 * 
 * Cette fonction doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_filesystem_quit(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // RC2D_INTERNAL_H