/**
 * OBLIGATOIRE pour la librairie RC2D :
 * Doit être inclus avant tout autre fichier d'en-tête SDL / RC2D / etc.
 * et dans UN SEUL fichier source (.c/.cpp) de l'application.
 * 
 * Cela concerne :
 * - SDL_MAIN_USE_CALLBACKS
 * - <SDL3/SDL_main.h> 
 * 
 * A partir du moment ou SDL_MAIN_USE_CALLBACKS est défini et <SDL3/SDL_main.h> est inclus,
 * la fonction main() ne doit pas être définie dans le code de l'application.
 */
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <mygame/game.h>

#include <RC2D/RC2D.h>

/**
 * \brief Fonction de configuration du moteur RC2D.
 * 
 * Cette fonction est appelée au démarrage de l'application pour configurer le moteur RC2D.
 * Elle initialise les paramètres par défaut et peut être modifiée par l'utilisateur pour personnaliser le
 * comportement du moteur.
 * 
 * \param {int} argc - Nombre d'arguments de la ligne de commande.
 * \param {char**} argv - Tableau des arguments de la ligne de commande.
 * \return {const RC2D_EngineConfig*} Pointeur vers la configuration du moteur RC2D.
 * 
 * \warning L'implementation de cette fonction doit être définie par l'utilisateur, puisqu'elle est appelée par le moteur RC2D.
 */
const RC2D_EngineConfig* rc2d_engine_setup(int argc, char* argv[])
{
    RC2D_EngineConfig* config = rc2d_engine_getDefaultConfig();
#ifdef NDEBUG // Release mode
    rc2d_logger_set_priority(RC2D_LOG_CRITICAL);
    config->gpuOptions->debugMode = false;
    config->gpuOptions->verbose = false;
#else // Debug mode
    rc2d_logger_set_priority(RC2D_LOG_TRACE);
    // Plateformes qui ne permettent pas le mode GPU débogage
#if defined(RC2D_PLATFORM_ANDROID) || defined(RC2D_PLATFORM_IOS)   || \
    defined(RC2D_PLATFORM_TVOS)    || defined(RC2D_PLATFORM_VISIONOS) || \
    defined(RC2D_PLATFORM_XBOXONE) || defined(RC2D_PLATFORM_XBOXSERIES)
    RC2D_log(RC2D_LOG_WARN, "Debug GPU mode forcé OFF sur cette plateforme, car non supportée.\n");
    config->gpuOptions->debugMode = false;
    config->gpuOptions->verbose = false;
#else
    config->gpuOptions->debugMode = true;
    config->gpuOptions->verbose = true;
#endif // Plateformes qui ne permettent pas le mode GPU débogage
#endif // NDEBUG
    config->gpuOptions->preferLowPower = false;
    config->gpuOptions->driver = RC2D_GPU_DRIVER_DEFAULT;
    config->windowWidth = 800;
    config->windowHeight = 600;
    config->logicalWidth = 1920;
    config->logicalHeight = 1080;
    config->callbacks->rc2d_draw = rc2d_draw;
    config->callbacks->rc2d_update = rc2d_update;
    config->callbacks->rc2d_load = rc2d_load;
    config->callbacks->rc2d_unload = rc2d_unload;
    config->callbacks->rc2d_mousepressed = rc2d_mousepressed;
    config->callbacks->rc2d_keypressed = rc2d_keypressed;
    config->logicalPresentationMode = RC2D_LOGICAL_PRESENTATION_OVERSCAN;
    config->pixelartMode = false;
    config->appInfo->name = "SeaTyrants";
    config->appInfo->organization = "Crzgames";
    config->appInfo->version = "1.0.0";
    config->appInfo->identifier = "com.crzgames.seatyrants";

    RC2D_assert_release(config != NULL, RC2D_LOG_CRITICAL, "RC2D_EngineConfig config is NULL. Cannot setup the engine.");

    return config;
}