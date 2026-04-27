#include <RC2D/RC2D_engine.h>

#if RC2D_STEAMWORKS_SDK_ENABLED
#include <RC2D/RC2D_steamworks.h>
#endif // RC2D_STEAMWORKS_SDK_ENABLED

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_atomic.h>

#include <SDL3_ttf/SDL_ttf.h>

#include <SDL3_mixer/SDL_mixer.h>

#if RC2D_DATA_MODULE_ENABLED
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif // RC2D_DATA_MODULE_ENABLED

#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
#include <SDL3_shadercross/SDL_shadercross.h>
#endif // RC2D_GPU_SHADER_HOT_RELOAD_ENABLED

#if RC2D_NET_MODULE_ENABLED
#include <rcenet/RCENET_enet.h>
#include <sodium.h>
#include <curl/curl.h>
#endif // RC2D_NET_MODULE_ENABLED

// Internal headers
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_assert.h>
#include <RC2D/RC2D_math.h>
#include <RC2D/RC2D_filesystem.h>
#include <RC2D/RC2D_platform_defines.h>
#include <RC2D/RC2D_memory.h>
#include <RC2D/RC2D_config.h>
#include <RC2D/RC2D_engine.h>
#include <RC2D/RC2D_gpu.h>
#include <RC2D/RC2D_storage.h>

RC2D_EngineState rc2d_engine_state = {0};

static bool rc2d_engine_is_valid_texture_scale_mode(RC2D_TextureScaleMode mode)
{
    return mode == RC2D_TEXTURE_SCALE_NEAREST ||
           mode == RC2D_TEXTURE_SCALE_LINEAR ||
           mode == RC2D_TEXTURE_SCALE_PIXELART;
}

static bool rc2d_engine_is_valid_logical_presentation_mode(RC2D_LogicalPresentationMode mode)
{
    return mode == RC2D_LOGICAL_PRESENTATION_INTEGER_SCALE ||
           mode == RC2D_LOGICAL_PRESENTATION_LETTERBOX ||
           mode == RC2D_LOGICAL_PRESENTATION_OVERSCAN;
}

static SDL_ScaleMode rc2d_engine_to_sdl_scale_mode(RC2D_TextureScaleMode mode)
{
    switch (mode)
    {
        case RC2D_TEXTURE_SCALE_NEAREST:
            return SDL_SCALEMODE_NEAREST;
        case RC2D_TEXTURE_SCALE_LINEAR:
            return SDL_SCALEMODE_LINEAR;
        case RC2D_TEXTURE_SCALE_PIXELART:
            return SDL_SCALEMODE_PIXELART;
        default:
            return SDL_SCALEMODE_LINEAR;
    }
}

static SDL_RendererLogicalPresentation rc2d_engine_to_sdl_logical_presentation_mode(RC2D_LogicalPresentationMode mode)
{
    switch (mode)
    {
        case RC2D_LOGICAL_PRESENTATION_INTEGER_SCALE:
            return SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
        case RC2D_LOGICAL_PRESENTATION_OVERSCAN:
            return SDL_LOGICAL_PRESENTATION_OVERSCAN;
        case RC2D_LOGICAL_PRESENTATION_LETTERBOX:
        default:
            return SDL_LOGICAL_PRESENTATION_LETTERBOX;
    }
}

bool rc2d_engine_setTextureScaleMode(RC2D_TextureScaleMode mode)
{
    if (!rc2d_engine_is_valid_texture_scale_mode(mode))
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid texture scale mode provided.\n");
        return false;
    }

    if (rc2d_engine_state.config == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "Engine config is NULL. Cannot change texture scale mode.\n");
        return false;
    }

    rc2d_engine_state.config->textureScaleMode = mode;

    if (rc2d_engine_state.renderer != NULL &&
        !SDL_SetDefaultTextureScaleMode(
            rc2d_engine_state.renderer,
            rc2d_engine_to_sdl_scale_mode(mode)))
    {
        RC2D_log(RC2D_LOG_WARN, "Erreur : impossible de configurer le mode de mise à l'échelle des textures (SDL_SetDefaultTextureScaleMode) : %s\n", SDL_GetError());
        return false;
    }

    return true;
}

/**
 * \brief Initialise les valeurs par défaut de l'état global du moteur RC2D.
 *
 * Cette fonction configure les valeurs par défaut pour toutes les variables de la structure RC2D_EngineState.
 * Elle est appelée avant toute autre opération pour garantir que l'état du moteur est correctement initialisé.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_stateInit(void) {
    // Configuration de l'application (mettre toutes les valeurs par défaut)
    rc2d_engine_state.config = rc2d_engine_getDefaultConfig();

    // SDL : Fenêtre et événements
    rc2d_engine_state.window = NULL;
    // rc2d_engine_state.rc2d_event est déjà zéro-initialisé

    // SDL : Renderer
    rc2d_engine_state.renderer = NULL;

    // SDL_mixer
    rc2d_engine_state.mixer = NULL;

    // SDL GPU
    rc2d_engine_state.gpu_device = NULL;

    // Initialiser le cache des shaders graphiques
    rc2d_engine_state.gpu_graphics_shader_count = 0;
    rc2d_engine_state.gpu_graphics_shaders_cache = NULL;
    rc2d_engine_state.gpu_graphics_shader_mutex = SDL_CreateMutex();
    if (!rc2d_engine_state.gpu_graphics_shader_mutex) {
        RC2D_assert_release(false, RC2D_LOG_CRITICAL, "Erreur lors de la création du mutex pour les shaders : %s", SDL_GetError());
        return;
    }

    // État d'exécution de la boucle de jeu
    rc2d_engine_state.fps = 60;
    rc2d_engine_state.delta_time = 0.0;
    rc2d_engine_state.game_is_running = true;
    rc2d_engine_state.last_frame_time = 0;

#if RC2D_NET_MODULE_ENABLED
    SDL_SetAtomicInt(&rc2d_engine_state.worker_threads_should_run, 0);
    rc2d_engine_state.simulation_thread = NULL;
    rc2d_engine_state.http_thread = NULL;
    rc2d_engine_state.websocket_thread = NULL;
    rc2d_engine_state.simulation_tick_id = 0;
    rc2d_engine_state.network_thread = NULL;
    rc2d_engine_state.network_client_host = NULL;
    rc2d_engine_state.network_server_peer = NULL;
    rc2d_engine_state.network_is_connected = false;
    rc2d_engine_state.network_control_mutex = SDL_CreateMutex();
    if (!rc2d_engine_state.network_control_mutex)
    {
        RC2D_assert_release(false, RC2D_LOG_CRITICAL, "Erreur lors de la creation du mutex reseau : %s", SDL_GetError());
        return;
    }
    SDL_SetAtomicInt(&rc2d_engine_state.network_connect_desired, 0);
    SDL_SetAtomicInt(&rc2d_engine_state.network_disconnect_requested, 0);
    rc2d_engine_state.network_runtime_endpoint_is_set = false;
    rc2d_engine_state.network_runtime_server_address[0] = '\0';
    rc2d_engine_state.network_runtime_server_port = 0;
#endif
}

RC2D_EngineConfig* rc2d_engine_getDefaultConfig(void)
{    
    static RC2D_AppInfo default_app_info = {
        .name = "RC2D Game",
        .organization = "Crzgames",
        .version = "1.0.0",
        .identifier = "com.example.rc2dgame"
    };

    static RC2D_GPUAdvancedOptions default_gpu_options = {
        .debugMode = true,
        .verbose = true,
        .preferLowPower = false,
        .driver = RC2D_GPU_DRIVER_DEFAULT
    };

    static RC2D_EngineCallbacks default_callbacks = {0};

#if RC2D_NET_MODULE_ENABLED
    static RC2D_NetworkClientConfig default_network_client_config = {
        .serverAddress = "127.0.0.1",
        .serverPort = 12345,
        .channelCount = 4,
        .simulationTickRateHz = 128,
        .maxConnections = 1,
        .incomingBandwidth = 0,
        .outgoingBandwidth = 0,
        .incomingPollTimeoutMs = 1,
        .outgoingTickRateHz = 64,
        .autoConnectOnStart = false,
        .autoReconnectOnDisconnect = false,
        .maxReconnectAttempts = 5,
        .connectTimeoutMs = 5000
    };
#endif

    static RC2D_EngineConfig default_config = {
        .callbacks = &default_callbacks,
#if RC2D_NET_MODULE_ENABLED
        .networkClientConfig = &default_network_client_config,
#endif
        .windowWidth = 800,
        .windowHeight = 600,
        .logicalWidth = 1920,
        .logicalHeight = 1080,
        .logicalPresentationMode = RC2D_LOGICAL_PRESENTATION_LETTERBOX,
        .textureScaleMode = RC2D_TEXTURE_SCALE_LINEAR,
        .appInfo = &default_app_info,
        .gpuFramesInFlight = RC2D_GPU_FRAMES_BALANCED,
        .gpuOptions = &default_gpu_options
    };

    return &default_config;
}

/**
 * \brief Charge la base de données de contrôleurs de jeu intégrée : https://github.com/mdqinc/SDL_GameControllerDB
 *
 * Cette fonction charge une base de données de mappages de contrôleurs de jeu
 * intégrée dans SDL3 pour assurer une compatibilité maximale avec divers contrôleurs.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_gamepad_load_embedded_db(void)
{
    // Charger la base de données intégrée des contrôleurs de jeu en mémoire
    SDL_IOStream *io = SDL_IOFromConstMem(rc2d_gamecontrollerdb_data,
                                         rc2d_gamecontrollerdb_size);
    if (!io) 
    {
        // Erreur lors de la création du flux IO à partir de la mémoire
        RC2D_log(RC2D_LOG_ERROR, "SDL_IOFromConstMem failed: %s", SDL_GetError());
        return false;
    }

    // Ajouter les mappages de contrôleurs de jeu à partir du flux IO
    const int added = SDL_AddGamepadMappingsFromIO(io, true);
    if (added == -1) 
    {
        RC2D_log(RC2D_LOG_ERROR, "SDL_AddGamepadMappingsFromIO failed: %s", SDL_GetError());
        return false;
    }
    else
    {
        // `added` = nombre de profils de manettes disponibles qui ont été ajoutés à partir de la base embarquée (RC2D_gamecontrollerdb_embedded.c)
        RC2D_log(RC2D_LOG_INFO, "Gamepad database loaded — %d controller mappings available", added);
    }

    // Retourne true si tout s'est bien passé
    return true;
}

/**
 * \brief Affiche la liste des pilotes GPU supportés par SDL3.
 *
 * Cette fonction vérifie si au moins un backend GPU est supporté par SDL3.
 * Elle affiche également la liste des pilotes GPU disponibles.
 * 
 * \return {bool} - true si au moins un backend GPU est supporté, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_supported_gpu_backends(void)
{
    int count = SDL_GetNumGPUDrivers();
    if (count <= 0) {
        RC2D_log(RC2D_LOG_CRITICAL, "Aucun backend GPU compatible pour SDL3 détecté.");
        return false;
    }

    RC2D_log(RC2D_LOG_INFO, "Pilotes GPU disponibles pour SDL3 (%d détecté%s) :", count, count > 1 ? "s" : "");
    for (int i = 0; i < count; ++i) 
    {
        const char* name = SDL_GetGPUDriver(i);
        if (name != NULL)
        {
            RC2D_log(RC2D_LOG_INFO, "  - %d : %s", i, name);
        }
    }

    return true;
}

/**
 * \brief Convertit le mode de présentation SDL_GPU en chaîne de caractères.
 *
 * Cette fonction convertit un mode de présentation SDL_GPU en une chaîne de caractères lisible.
 * 
 * \param {SDL_GPUPresentMode} mode - Le mode de présentation à convertir.
 * \return {const char*} - La chaîne de caractères représentant le mode de présentation.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
const char* rc2d_present_mode_to_string(SDL_GPUPresentMode mode) {
    switch (mode) {
        case SDL_GPU_PRESENTMODE_MAILBOX:   return "RC2D_GPU_PRESENTMODE_MAILBOX";
        case SDL_GPU_PRESENTMODE_VSYNC:     return "RC2D_GPU_PRESENTMODE_VSYNC";
        case SDL_GPU_PRESENTMODE_IMMEDIATE: return "RC2D_GPU_PRESENTMODE_IMMEDIATE";
        default: return "RC2D_GPU_PRESENTMODE_UNKNOWN";
    }
}

/**
 * \brief Convertit la composition de swapchain SDL_GPU en chaîne de caractères.
 *
 * Cette fonction convertit une composition de swapchain SDL_GPU en une chaîne de caractères lisible.
 * 
 * \param {SDL_GPUSwapchainComposition} comp - La composition de swapchain à convertir.
 * \return {const char*} - La chaîne de caractères représentant la composition de swapchain.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
const char* rc2d_composition_to_string(SDL_GPUSwapchainComposition comp) {
    switch (comp) {
        case SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084:      return "RC2D_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084";
        case SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR:return "RC2D_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR";
        case SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR:        return "RC2D_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR";
        case SDL_GPU_SWAPCHAINCOMPOSITION_SDR:               return "RC2D_GPU_SWAPCHAINCOMPOSITION_SDR";
        default: return "RC2D_GPU_SWAPCHAINCOMPOSITION_UNKNOWN";
    }
}

/**
 * \brief Configure le swapchain GPU avec la meilleure combinaison de mode de présentation et de composition.
 *
 * Cette fonction tente de trouver et d'appliquer la meilleure combinaison supportée de mode de présentation
 * et de composition de swapchain pour le dispositif GPU et la fenêtre donnés.
 *
 * \return true si une configuration valide a été appliquée, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_configure_swapchain(void)
{
    // SDL3 : Configurer le mode de présentation du GPU
    /**
     * SDL_GPU_PRESENTMODE_MAILBOX est utiliser par défaut,
     * car il est généralement le meilleur choix pour la plupart des applications.
     * Il offre un bon équilibre entre la latence et la fluidité de l'affichage, 
     * mais il n'est pas toujours disponible sur tous les systèmes.
     * 
     * SDL_GPU_PRESENTMODE_VSYNC est un bon choix si vous voulez éviter le tearing,
     * mais il peut introduire une latence supplémentaire, mais il est toujours disponible et sûr.
     * 
     * SDL_GPU_PRESENTMODE_IMMEDIATE est le moins recommandé, car il peut entraîner du tearing,
     * mais il peut être utilisé si vous avez besoin de la latence la plus basse possible.
     */
    SDL_GPUPresentMode present_modes[] = {
        SDL_GPU_PRESENTMODE_MAILBOX,
        SDL_GPU_PRESENTMODE_VSYNC,
        SDL_GPU_PRESENTMODE_IMMEDIATE
    };

    // Configurer le swapchain pour le GPU
    /**
     * SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084 est le meilleur choix pour les écrans HDR,
     * mais il n'est pas toujours disponible sur tous les systèmes.
     * 
     * SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR est un bon choix pour les écrans HDR,
     * mais il n'est pas toujours disponible sur tous les systèmes.
     * 
     * SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR est un bon choix pour les écrans SDR,
     * mais il n'est pas toujours disponible sur tous les systèmes.
     * 
     * SDL_GPU_SWAPCHAINCOMPOSITION_SDR est toujours disponible sur tous les systèmes.
     */
    SDL_GPUSwapchainComposition compositions[] = {
        SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084,
        SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR
    };
    
    /**
     * Appliquer le swapchain pour le GPU
     * Rechercher la meilleure combinaison supportée entre le mode de présentation et la composition.
     */
    bool swapchain_combo_found = false;
    for (int i = 0; i < SDL_arraysize(present_modes); ++i)
    {
        for (int j = 0; j < SDL_arraysize(compositions); ++j) 
        {
            SDL_GPUPresentMode pm = present_modes[i];
            SDL_GPUSwapchainComposition sc = compositions[j];

            // Vérifie si la combinaison est supportée individuellement
            if (SDL_WindowSupportsGPUPresentMode(rc2d_engine_state.gpu_device, rc2d_engine_state.window, pm) &&
                SDL_WindowSupportsGPUSwapchainComposition(rc2d_engine_state.gpu_device, rc2d_engine_state.window, sc)) 
            {
                // Essaye la combinaison
                if (SDL_SetGPUSwapchainParameters(rc2d_engine_state.gpu_device, rc2d_engine_state.window, sc, pm)) 
                {
                    // Si la combinaison est supportée, on l'applique
                    rc2d_engine_state.gpu_present_mode = pm;
                    rc2d_engine_state.gpu_swapchain_composition = sc;
                    RC2D_log(RC2D_LOG_INFO, "GPU swapchain configuré avec succès : present_mode = %s, composition = %s", rc2d_present_mode_to_string(pm), rc2d_composition_to_string(sc));
                    swapchain_combo_found = true;
                    break;
                }
                else
                {
                    /**
                     * Si la combinaison de mode de présentation et de composition est supportée individuellement
                     * mais qu'elle échoue lors de l'application avec SDL_SetGPUSwapchainParameters, on loggue un avertissement
                     * en précisant les noms lisibles de la combinaison qui a échoué.
                     */
                    RC2D_log(RC2D_LOG_WARN, "La combinaison de mode de présentation et de composition a échoué : present_mode = %s, composition = %s", rc2d_present_mode_to_string(pm), rc2d_composition_to_string(sc));
                }
            }
        }

        // Si une combinaison valide a été trouvée, on sort de la boucle externe
        if (swapchain_combo_found) break;
    }

    if (!swapchain_combo_found) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Could not find any valid swapchain configuration.");
        return false;
    }

    return true;
}

/**
 * \brief Initialise la bibliothèque RCENet pour le module réseau.
 * 
 * Cette fonction initialise la bibliothèque RCENet si le module RC2D_net est activé.
 * Elle doit être appelée avant d'utiliser les fonctionnalités réseau.
 * 
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_rcenet(void)
{
#if RC2D_NET_MODULE_ENABLED
    if (enet_initialize() < 0) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de l'initialisation de RCEnet.");
        return false;
    }
    else 
    {
        RC2D_log(RC2D_LOG_INFO, "RCENet initialiser avec succes.");
        return true;
    }
#endif

    // Si le module RC2D_net n'est pas activé, on retourne true par défaut
    return true;
}

/**
 * \brief Libère les ressources RCNet.
 * 
 * Cette fonction libère les ressources allouées par RCENet.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_rcenet(void)
{
#if RC2D_NET_MODULE_ENABLED
    enet_deinitialize();
    RC2D_log(RC2D_LOG_INFO, "RCENet nettoyer avec succes.");
#endif
}

/**
 * \brief Initialise la bibliothèque libsodium pour le module réseau.
 * 
 * Cette fonction initialise la bibliothèque libsodium si le module RC2D_net est activé.
 * Elle doit être appelée avant d'utiliser les fonctionnalités de cryptographie réseau.
 * 
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_libsodium(void)
{
#if RC2D_NET_MODULE_ENABLED
    // Initialise libsodium.
    if (sodium_init() < 0)
    {
        // Log erreur en cas d'échec.
        RC2D_log(RC2D_LOG_ERROR, "Erreur lors de l initialisation de libsodium.");
        return false;
    }
    else
    {
        RC2D_log(RC2D_LOG_INFO, "Libsodium initialiser avec succes.");
        return true;
    }
#endif

    // Si le module RC2D_net n'est pas activé, on retourne true par défaut
    return true;
}

/**
 * \brief Initialise la bibliothèque libcurl pour le module réseau.
 * 
 * Cette fonction initialise la bibliothèque libcurl si le module RC2D_net est activé.
 * Elle doit être appelée avant d'utiliser les fonctionnalités de requêtes HTTP ou de WebSockets.
 * 
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_curl(void)
{
#if RC2D_NET_MODULE_ENABLED
    // Initialise libcurl.
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de l initialisation de libcurl : %s", curl_easy_strerror(res));
        return false;
    }
    else
    {
        RC2D_log(RC2D_LOG_INFO, "libcurl initialiser avec succes.");
        return true;
    }
#endif

    // Si le module RC2D_net n'est pas activé, on retourne true par défaut
    return true;
}

/**
 * \brief Libère les ressources libcurl.
 * 
 * Cette fonction libère les ressources allouées par libcurl.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_curl(void)
{
#if RC2D_NET_MODULE_ENABLED
    curl_global_cleanup();
    RC2D_log(RC2D_LOG_INFO, "libcurl nettoyer avec succes.");
#endif
}

/**
 * \brief Initialise la bibliothèque SDL3_shadercross.
 * 
 * Cette fonction initialise la bibliothèque SDL3_shadercross pour le rechargement à chaud des shaders.
 * Elle doit être appelée avant d'utiliser les fonctionnalités de rechargement à chaud des shaders.
 * 
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_sdlshadercross(void)
{
#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
    if (!SDL_ShaderCross_Init()) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de l'initialisation de SDL_shadercross.");
        return false;
    }
    else 
    {
        RC2D_log(RC2D_LOG_INFO, "SDL_shadercross initialisé avec succès.");
        return true;
    }
#endif

    // Si le rechargement à chaud des shaders n'est pas activé, on retourne true par défaut
    return true;
}

/**
 * \brief Libère les ressources SDL3_shadercross.
 * 
 * Cette fonction libère les ressources allouées par SDL3_shadercross.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_sdlshadercross(void)
{
#if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
    SDL_ShaderCross_Quit();
    RC2D_log(RC2D_LOG_INFO, "SDL_shadercross nettoyé avec succès.");
#endif
}

/**
 * \brief Initialise la bibliothèque OpenSSL avec options de log.
 * 
 * Cette fonction appelle OPENSSL_init_ssl() avec les options standards de chargement
 * des chaînes d’erreur et d’algorithmes. Elle loggue et quitte le programme si 
 * l’initialisation échoue.
 * 
 * \return true si l'initialisation a réussi, false sinon.
 *
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_openssl(void) 
{
#if RC2D_DATA_MODULE_ENABLED
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL) == 0)
    {
        RC2D_assert_release(false, RC2D_LOG_CRITICAL, "Erreur lors de l'initialisation d'OpenSSL : %s", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
    else 
    {
        RC2D_log(RC2D_LOG_INFO, "OpenSSL initialisé avec succès.");
        return true;
    }
#endif

    // Si le module RC2D_data n'est pas activé, on retourne true par défaut
    return true;
}

/**
 * \brief Libère les ressources OpenSSL.
 *
 * Cette fonction libère les ressources allouées par OpenSSL et nettoie les chaînes d'erreur.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_openssl(void)
{
#if RC2D_DATA_MODULE_ENABLED
    ERR_free_strings();
    EVP_cleanup();
    RC2D_log(RC2D_LOG_INFO, "OpenSSL nettoyé avec succès.");
#endif
}

/**
 * \brief Initialise la bibliothèque SDL3_ttf.
 *
 * Cette fonction initialise la bibliothèque SDL3_ttf pour le rendu de polices.
 * Elle doit être appelée avant d'utiliser les fonctions de rendu de texte.
 *
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_sdlttf(void) 
{
    if (!TTF_Init()) 
    {
		RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de l'initialisation de SDL3_ttf : %s\n", SDL_GetError());
		return false;
    }
    else
    {
        if (!rc2d_graphics_createRendererTextEngine()) 
        {
            TTF_Quit();
            return false;
        }

        RC2D_log(RC2D_LOG_INFO, "SDL3_ttf initialisé avec succès.\n");
        return true;
    }
}

/**
 * \brief Libère les ressources SDL3_ttf.
 *
 * Cette fonction libère les ressources allouées par SDL3_ttf.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_sdlttf(void)
{
    TTF_Quit();
    RC2D_log(RC2D_LOG_INFO, "SDL3_ttf nettoyé avec succès.\n");
}

/**
 * \brief Initialise la bibliothèque SDL3_mixer.
 *
 * Cette fonction initialise la bibliothèque SDL3_mixer pour le rendu audio.
 * Elle doit être appelée avant d'utiliser les fonctions de rendu audio.
 *
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_sdlmixer(void) 
{
    if (!MIX_Init())
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de l'initialisation de SDL3_mixer : %s\n", SDL_GetError());
        return false;
    }

    rc2d_engine_state.mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (rc2d_engine_state.mixer == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de la création du périphérique de mixage audio : %s\n", SDL_GetError());
        MIX_Quit();
        return false;
    }

    return true;
}

/**
 * \brief Libère les ressources SDL3_mixer.
 *
 * Cette fonction libère les ressources allouées par SDL3_mixer.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_sdlmixer(void)
{
    MIX_Quit();
    RC2D_log(RC2D_LOG_INFO, "SDL3_mixer nettoyé avec succès.\n");
}

/**
 * \brief Initialise la bibliothèque SDL3.
 *
 * Cette fonction initialise les sous-systèmes SDL3 nécessaires au moteur RC2D.
 *
 * \return true si l'initialisation a réussi, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_init_sdl(void)
{
    /**
     * Une variable contrôlant les orientations autorisées sur iOS/Android.
     */
#if defined(RC2D_PLATFORM_IOS) || defined(RC2D_PLATFORM_ANDROID)
    SDL_SetHintWithPriority(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight", SDL_HINT_OVERRIDE);
#endif

    /**
     * IMPORTANT:
     * Obligation pour le Steam Deck, sans le getenv("WAYLAND_DISPLAY") ou getenv("DISPLAY"),
     * l'application ne démarre pas.
     * Doit être fait avant l'initialisation de SDL3.
     */
    getenv("DISPLAY");
    getenv("WAYLAND_DISPLAY");

    /**
     * Liste des sous-systèmes SDL3 à initialiser.
     */
    int subsystems[] = {
        SDL_INIT_AUDIO,
        SDL_INIT_VIDEO,
        SDL_INIT_JOYSTICK,
        SDL_INIT_HAPTIC,
        SDL_INIT_GAMEPAD,
        SDL_INIT_EVENTS,
        SDL_INIT_SENSOR,
        SDL_INIT_CAMERA
    };

    /**
     * Liste des noms des sous-systèmes SDL3 pour le logging.
     * Doit être dans le même ordre que la liste des flags ci-dessus.
     */
    const char* names[] = {
        "AUDIO", 
        "VIDEO", 
        "JOYSTICK", 
        "HAPTIC", 
        "GAMEPAD", 
        "EVENTS", 
        "SENSOR", 
        "CAMERA"
    };

    /**
     * Initialisation de SDL3 avec tous les sous-systèmes nécessaires.
     * On vérifie si chaque sous-système s'initialise correctement.
     * Si un sous-système échoue, on loggue l'erreur et on continue.
     */
    for (int i = 0; i < sizeof(subsystems) / sizeof(subsystems[0]); ++i) 
    {
        if (!SDL_InitSubSystem(subsystems[i])) 
        {
            RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de l'initialisation du sous-système SDL3 %s : %s\n", names[i], SDL_GetError());
        } 
        else {
            RC2D_log(RC2D_LOG_INFO, "Initialise le sous-système SDL3 %s avec succès.\n", names[i]);
        }
    }

    return true;
}

/**
 * \brief Libère les ressources SDL3.
 *
 * Cette fonction libère les ressources allouées par SDL3.
 * Elle doit être appelée avant de quitter l'application pour éviter les fuites de mémoire.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_cleanup_sdl(void)
{
    SDL_Quit();
    RC2D_log(RC2D_LOG_INFO, "SDL3 nettoyé avec succès.\n");
}

/**
 * \brief Crée la fenêtre principale de l'application RC2D.
 * 
 * Cette fonction configure et crée la fenêtre SDL3 avec les propriétés spécifiées dans la configuration du moteur.
 * La fenêtre est initialement cachée pour éviter des artefacts visuels jusqu'à ce que le rendu GPU soit prêt.
 * 
 * \note La fenetre sera visible juste avant la première frame de la boucle de jeu.
 * 
 * \return true si la fenêtre a été créée avec succès, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_create_window(void)
{
    SDL_PropertiesID window_props = SDL_CreateProperties();
    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, rc2d_engine_state.config->appInfo->name);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, rc2d_engine_state.config->windowWidth);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, rc2d_engine_state.config->windowHeight);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);

    /**
     * IMPORTANT :
     * Cache la fenêtre tant que le rendu GPU n'est pas prêt pour éviter des artefacts visuels.
     * On l'affichera plus tard juste avant la première frame de la boucle de jeu.
     */
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);

    rc2d_engine_state.window = SDL_CreateWindowWithProperties(window_props);
    SDL_DestroyProperties(window_props);
    if (!rc2d_engine_state.window) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de la création de la fenêtre : %s", SDL_GetError());
        return false;
    }
    else
    {
        RC2D_log(RC2D_LOG_INFO, "La fenêtre est créée avec succès, mais temporairement masquée le temps que tout soit prêt.");
    }

    return true;
}

/**
 * \brief Initialise le dispositif de Renderer GPU pour le rendu graphique dans RC2D.
 * 
 * Cette fonction configure et crée le dispositif GPU SDL3, vérifie les formats de shaders supportés,
 * configure les modes de présentation et les paramètres de swapchain, et associe la fenêtre au GPU.
 * 
 * \return true si le GPU a été initialisé avec succès, false sinon.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static bool rc2d_engine_create_renderergpu(void)
{
    /**
     * Active le mode de débogage pour le rendu GPU si demandé dans la configuration.
     * Utile pour le développement et le débogage des shaders.
    */
    SDL_SetHintWithPriority(SDL_HINT_RENDER_GPU_DEBUG, rc2d_engine_state.config->gpuOptions->debugMode ? "1" : "0", SDL_HINT_OVERRIDE);

    /**
    * Une variable contrôlant s'il faut préférer un GPU basse consommation sur les systèmes multi-GPU.
    */ 
    SDL_SetHintWithPriority(SDL_HINT_RENDER_GPU_LOW_POWER, rc2d_engine_state.config->gpuOptions->preferLowPower ? "1" : "0", SDL_HINT_OVERRIDE);

    /**
     * Force le backend GPU si demandé dans la configuration.
     */
    switch (rc2d_engine_state.config->gpuOptions->driver) {
        case RC2D_GPU_DRIVER_VULKAN:
            SDL_SetHintWithPriority(SDL_HINT_GPU_DRIVER, "vulkan", SDL_HINT_OVERRIDE);
            break;
        case RC2D_GPU_DRIVER_METAL:
            SDL_SetHintWithPriority(SDL_HINT_GPU_DRIVER, "metal", SDL_HINT_OVERRIDE);
            break;
        case RC2D_GPU_DRIVER_DIRECT3D12:
            SDL_SetHintWithPriority(SDL_HINT_GPU_DRIVER, "direct3d12", SDL_HINT_OVERRIDE);
            break;
        case RC2D_GPU_DRIVER_PRIVATE:
            // Rien d'équivalent en public pour un backend privé.
            break;
        case RC2D_GPU_DRIVER_DEFAULT:
        default:
            // Laisser SDL choisir
            break;
    }

    /**
    * Créez un contexte de rendu GPU 2D pour une fenêtre,
    * avec prise en charge du format de shader spécifié.
    */
    rc2d_engine_state.renderer = SDL_CreateGPURenderer(NULL, rc2d_engine_state.window);
    if (!rc2d_engine_state.renderer) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Erreur lors de la création du renderer GPU : %s", SDL_GetError());
        return false;
    }
    else
    {
        RC2D_log(RC2D_LOG_INFO, "Renderer GPU créé avec succès.");
        rc2d_engine_state.gpu_device = SDL_GetGPURendererDevice(rc2d_engine_state.renderer);
    }

    /**
    * Renvoie true si tout s'est bien passé.
    */
    return true;
}

/**
 * \brief Met à jour le rectangle de la zone visible et interactive en coordonnées logiques.
 *
 * Cette fonction calcule et met à jour le rectangle représentant la zone de l'écran
 * qui est garantie d'être visible et interactive, en tenant compte des marges de sécurité
 * et du mode de présentation.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
void rc2d_engine_presentationUpdate(void)
{
    // 1) Récupère la logique et le mode courant
    int LW = 0, LH = 0; 
    SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
    SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &LW, &LH, &mode);

    rc2d_engine_state.logical_w = LW;
    rc2d_engine_state.logical_h = LH;

    // 2) Rectangle de présentation final (en pixels de sortie)
    SDL_FRect pres_px = {0,0,0,0};
    if (!SDL_GetRenderLogicalPresentationRect(rc2d_engine_state.renderer, &pres_px))
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur lors de la récupération du rectangle de présentation logique : %s", SDL_GetError());
        return;
    }

    // 3) Portion LOGIQUE réellement visible (crop si OVERSCAN)
    SDL_FRect visible;
    if (mode == SDL_LOGICAL_PRESENTATION_OVERSCAN) 
    {
        // facteur d’échelle appliqué par SDL (pixels sortants par unité logique)
        float sx = pres_px.w / (float)LW;
        float sy = pres_px.h / (float)LH;
        float s  = SDL_max(sx, sy);            // OVERSCAN = "cover" => on prend le plus grand

        float vis_w = pres_px.w / s;           // largeur VISIBLE en unités logiques
        float vis_h = pres_px.h / s;           // hauteur VISIBLE en unités logiques

        visible.x = (float)LW * 0.5f - vis_w * 0.5f;
        visible.y = (float)LH * 0.5f - vis_h * 0.5f;
        visible.w = vis_w;
        visible.h = vis_h;
    } 
    else 
    {
        // LETTERBOX / INTEGER / DISABLED => tout le logique est visible
        visible = (SDL_FRect){0,0,(float)LW,(float)LH};
    }

    // 4) Safe area (déjà en coords logiques pour le renderer)
    SDL_Rect s = {0,0,LW,LH};
    SDL_GetRenderSafeArea(rc2d_engine_state.renderer, &s);
    SDL_FRect safe = { (float)s.x, (float)s.y, (float)s.w, (float)s.h };

    // 5) Intersection visible ∩ safe => garanti visible + interactif
    float x1 = SDL_max(visible.x, safe.x);
    float y1 = SDL_max(visible.y, safe.y);
    float x2 = SDL_min(visible.x + visible.w, safe.x + safe.w);
    float y2 = SDL_min(visible.y + visible.h, safe.y + safe.h);

    rc2d_engine_state.visible_safe_rect = (SDL_FRect){
        x1, y1,
        (x2 > x1) ? (x2 - x1) : 0.0f,
        (y2 > y1) ? (y2 - y1) : 0.0f
    };
}

SDL_FRect rc2d_engine_getVisibleSafeRectRender(void) 
{
    return rc2d_engine_state.visible_safe_rect;
}

RC2D_LogicalPresentationMode rc2d_engine_getLogicalPresentationMode(void)
{
    if (rc2d_engine_state.config != NULL)
    {
        return rc2d_engine_state.config->logicalPresentationMode;
    }

    if (rc2d_engine_state.renderer != NULL)
    {
        int logicalWidth = 0;
        int logicalHeight = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        if (SDL_GetRenderLogicalPresentation(rc2d_engine_state.renderer, &logicalWidth, &logicalHeight, &mode))
        {
            (void)logicalWidth;
            (void)logicalHeight;
            if (mode == SDL_LOGICAL_PRESENTATION_OVERSCAN)
            {
                return RC2D_LOGICAL_PRESENTATION_OVERSCAN;
            }
            if (mode == SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)
            {
                return RC2D_LOGICAL_PRESENTATION_INTEGER_SCALE;
            }
        }
    }

    return RC2D_LOGICAL_PRESENTATION_LETTERBOX;
}

bool rc2d_engine_setLogicalPresentationMode(RC2D_LogicalPresentationMode mode)
{
    if (!rc2d_engine_is_valid_logical_presentation_mode(mode))
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid logical presentation mode provided.\n");
        return false;
    }

    if (rc2d_engine_state.config == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "Engine config is NULL. Cannot change logical presentation mode.\n");
        return false;
    }

    rc2d_engine_state.config->logicalPresentationMode = mode;

    if (rc2d_engine_state.renderer != NULL &&
        !SDL_SetRenderLogicalPresentation(
            rc2d_engine_state.renderer,
            rc2d_engine_state.config->logicalWidth,
            rc2d_engine_state.config->logicalHeight,
            rc2d_engine_to_sdl_logical_presentation_mode(mode)))
    {
        RC2D_log(RC2D_LOG_WARN, "Erreur : impossible de configurer le mode de presentation logique : %s\n", SDL_GetError());
        return false;
    }

    if (rc2d_engine_state.renderer != NULL)
    {
        rc2d_engine_presentationUpdate();
    }

    return true;
}

/**
 * \brief Met à jour le FPS en fonction du moniteur.
 *
 * Cette fonction met à jour le FPS en fonction du taux de rafraîchissement du moniteur associé à la fenêtre.
 * Elle doit être appelée après la création de la fenêtre et avant le rendu.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_update_fps_based_on_monitor(void) 
{
    // Récupére le moniteur associé a la fenetre.
    SDL_DisplayID displayID = SDL_GetDisplayForWindow(rc2d_engine_state.window);
    if (displayID == 0) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Could not get display index for window: %s", SDL_GetError());
        return;
    }

    // Obtient le mode d'affichage actuel du moniteur.
    const SDL_DisplayMode* currentDisplayMode = SDL_GetCurrentDisplayMode(displayID);
    if (currentDisplayMode == NULL) 
    {
        RC2D_log(RC2D_LOG_ERROR, "Could not get current display mode for display #%d: %s", displayID, SDL_GetError());
        return;
    }

    // Met à jour les FPS selon le taux de rafraîchissement du moniteur.
    if (currentDisplayMode->refresh_rate_numerator > 0 && currentDisplayMode->refresh_rate_denominator > 0) 
    {
        rc2d_engine_state.fps = (double)currentDisplayMode->refresh_rate_numerator / currentDisplayMode->refresh_rate_denominator;
    } 
    else if (currentDisplayMode->refresh_rate > 0.0f) 
    {
        rc2d_engine_state.fps = (double)currentDisplayMode->refresh_rate;
    } 
    else 
    {
        rc2d_engine_state.fps = 60.0; // fallback
    }

    /**
     * Permet définir le tickrate de la callback SDL_AppIterate qui est appelé par SDL3,
     * par rapport au taux de rafraîchissement du moniteur.
     */
    char fps_str[32];
    SDL_snprintf(fps_str, sizeof(fps_str), "%.6g", (double)rc2d_engine_state.fps);
    if (!SDL_SetHintWithPriority(SDL_HINT_MAIN_CALLBACK_RATE, fps_str, SDL_HINT_OVERRIDE)) 
    {
        RC2D_log(RC2D_LOG_WARN, "Failed to set SDL_HINT_MAIN_CALLBACK_RATE to %s Hz with OVERRIDE priority: %s", fps_str, SDL_GetError());
    }
}

void rc2d_engine_deltatime_start(void)
{
    // Capture le temps au debut de la frame actuelle
    Uint64 now = SDL_GetPerformanceCounter();

    // Calcule le delta time depuis la derniere frame
    rc2d_engine_state.delta_time = (double)(now - rc2d_engine_state.last_frame_time) / (double)SDL_GetPerformanceFrequency();
    
    // Met a jour 'lastFrameTime' pour la prochaine frame
    rc2d_engine_state.last_frame_time = now;
}

void rc2d_engine_deltatime_end(void)
{
    /**
     * Vérifie si la hint SDL_HINT_MAIN_CALLBACK_RATE est active
     * Fallback : utilise SDL_DelayPrecise si la hint n'est pas définie ou définie à 0
     * puis que c'est possible que SDL_HINT_MAIN_CALLBACK_RATE ne sois pas pris en compte sur certaines plateformes.
     */
    const char* callback_rate = SDL_GetHint(SDL_HINT_MAIN_CALLBACK_RATE);
    if (callback_rate == NULL || SDL_strcmp(callback_rate, "0") == 0)
    {
        // Capture le temps a la fin de la frame actuelle
        Uint64 frameEnd = SDL_GetPerformanceCounter();

        // Calcule le temps de la frame actuelle en millisecondes
        double frameTimeMs = (double)(frameEnd - rc2d_engine_state.last_frame_time) * 1000.0 / (double)SDL_GetPerformanceFrequency();

        // Attendre le temps necessaire pour atteindre le FPS cible
        double targetFrameMs = 1000.0 / rc2d_engine_state.fps;
        if (frameTimeMs < targetFrameMs) 
        {
            Uint64 delayNs = (Uint64)((targetFrameMs - frameTimeMs) * 1e6);
            SDL_DelayPrecise(delayNs);
        } 
    }
}

#if RC2D_NET_MODULE_ENABLED
/**
 * \brief Retourne true si les threads workers doivent continuer a tourner.
 */
static bool rc2d_engine_workerThreadsShouldRun(void)
{
    // 1 = actif ; 0 = stop demande.
    return SDL_GetAtomicInt(&rc2d_engine_state.worker_threads_should_run) != 0;
}

/**
 * \brief Copie la cible reseau courante (runtime ou config) dans un buffer local.
 */
static bool rc2d_engine_networkTryResolveConnectTarget(
    char* outServerAddress,
    size_t outServerAddressCapacity,
    uint16_t* outServerPort)
{
    if (outServerAddress == NULL || outServerAddressCapacity == 0 || outServerPort == NULL)
    {
        return false;
    }

    if (rc2d_engine_state.config == NULL || rc2d_engine_state.config->networkClientConfig == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] Missing networkClientConfig.");
        return false;
    }

    const RC2D_NetworkClientConfig* networkClientConfig = rc2d_engine_state.config->networkClientConfig;
    const char* serverAddress = networkClientConfig->serverAddress;
    uint16_t serverPort = networkClientConfig->serverPort;

    if (rc2d_engine_state.network_control_mutex != NULL)
    {
        SDL_LockMutex(rc2d_engine_state.network_control_mutex);
        if (rc2d_engine_state.network_runtime_endpoint_is_set &&
            rc2d_engine_state.network_runtime_server_address[0] != '\0')
        {
            serverAddress = rc2d_engine_state.network_runtime_server_address;
            serverPort = rc2d_engine_state.network_runtime_server_port;
        }
        SDL_UnlockMutex(rc2d_engine_state.network_control_mutex);
    }

    if (serverAddress == NULL || serverAddress[0] == '\0')
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] serverAddress is NULL or empty.");
        return false;
    }

    if (serverPort == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] serverPort must be > 0.");
        return false;
    }

    SDL_strlcpy(outServerAddress, serverAddress, outServerAddressCapacity);
    *outServerPort = serverPort;
    return true;
}

bool rc2d_engine_networkConnectToServer(const char* serverAddress, uint16_t serverPort)
{
    if (serverAddress == NULL || serverAddress[0] == '\0')
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] networkConnectToServer failed: serverAddress is NULL or empty.");
        return false;
    }

    if (serverPort == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] networkConnectToServer failed: serverPort must be > 0.");
        return false;
    }

    if (rc2d_engine_state.network_control_mutex == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] networkConnectToServer failed: network_control_mutex is NULL.");
        return false;
    }

    SDL_LockMutex(rc2d_engine_state.network_control_mutex);
    SDL_strlcpy(
        rc2d_engine_state.network_runtime_server_address,
        serverAddress,
        sizeof(rc2d_engine_state.network_runtime_server_address));
    rc2d_engine_state.network_runtime_server_port = serverPort;
    rc2d_engine_state.network_runtime_endpoint_is_set = true;
    SDL_UnlockMutex(rc2d_engine_state.network_control_mutex);

    // Activer le mode connecte puis demander un refresh de session reseau.
    SDL_SetAtomicInt(&rc2d_engine_state.network_connect_desired, 1);
    SDL_SetAtomicInt(&rc2d_engine_state.network_disconnect_requested, 1);

    RC2D_log(
        RC2D_LOG_INFO,
        "[RC2D][NET] Connect requested to %s:%u.",
        serverAddress,
        (unsigned)serverPort);

    return true;
}

void rc2d_engine_networkDisconnectFromServer(void)
{
    // Passer en mode idle reseau, puis demander une fermeture propre.
    SDL_SetAtomicInt(&rc2d_engine_state.network_connect_desired, 0);
    SDL_SetAtomicInt(&rc2d_engine_state.network_disconnect_requested, 1);
}

/**
 * \brief Retourne la duree de base d'un tick en nanosecondes.
 */
static uint64_t rc2d_engine_getTickBaseDurationNs(uint32_t tickRateHz)
{
    // Si tickRateHz est invalide, on force 1 pour eviter la division par zero.
    const uint32_t effectiveTickRateHz = (tickRateHz > 0) ? tickRateHz : 1;

    // Conversion Hz -> duree de base d'un tick.
    return 1000000000ull / (uint64_t)effectiveTickRateHz;
}

/**
 * \brief Consomme un pas de temps exact (base + redistribution du reste).
 */
static uint64_t rc2d_engine_consumeTickStepNs(
    uint32_t tickRateHz,
    uint64_t tickBaseNs,
    uint64_t tickRemainderNs,
    uint64_t* tickRemainderAccumulator)
{
    // Si tickRateHz est invalide, on force 1 pour eviter la division par zero.
    const uint32_t effectiveTickRateHz = (tickRateHz > 0) ? tickRateHz : 1;

    // On part de la duree de base.
    uint64_t stepNs = tickBaseNs;

    // On accumule le reste pour conserver une cadence exacte dans le temps.
    *tickRemainderAccumulator += tickRemainderNs;

    // Si assez de reste est accumule, on ajoute 1ns et on retire un quantum.
    if (*tickRemainderAccumulator >= (uint64_t)effectiveTickRateHz)
    {
        *tickRemainderAccumulator -= (uint64_t)effectiveTickRateHz;
        stepNs += 1ull;
    }

    // Duree exacte du tick courant.
    return stepNs;
}

/**
 * \brief Attends jusqu'a une deadline monotone (ns) avec stop thread-safe.
 */
static void rc2d_engine_sleepUntilNs(uint64_t targetTimeNs)
{
    // Tant que les workers sont autorises a tourner.
    while (rc2d_engine_workerThreadsShouldRun())
    {
        // Lire l'horloge monotone courante.
        const uint64_t nowNs = SDL_GetTicksNS();

        // Sortie immediate si la deadline est atteinte.
        if (nowNs >= targetTimeNs)
        {
            return;
        }

        // Temps restant avant la deadline.
        const uint64_t remainingNs = targetTimeNs - nowNs;

        // Sur gros restant: on dort avec SDL_DelayPrecise pour viser une cadence plus stable.
        if (remainingNs > 2000000ull)
        {
            const Uint64 coarseSleepNs = remainingNs - 1000000ull;
            SDL_DelayPrecise((coarseSleepNs > 0ull) ? coarseSleepNs : 1ull);
        }
        else
        {
            // Sur la fin: on reste egalement en SDL_DelayPrecise.
            SDL_DelayPrecise(remainingNs);
        }
    }
}

/**
 * \brief Point d'entree du thread simulation client.
 */
static int rc2d_engine_simulationThreadMain(void* userData)
{
    // Argument de thread SDL non utilise.
    (void)userData;

    // Gardes de securite: configuration/callbacks obligatoires.
    if (rc2d_engine_state.config == NULL ||
        rc2d_engine_state.config->callbacks == NULL ||
        rc2d_engine_state.config->networkClientConfig == NULL ||
        rc2d_engine_state.config->callbacks->rc2d_simulation_update == NULL)
    {
        return 0;
    }

    // Raccourci sur la config reseau/simulation.
    const RC2D_NetworkClientConfig* networkClientConfig =
        rc2d_engine_state.config->networkClientConfig;

    // Frequence cible de simulation.
    const uint32_t simulationTickRateHz =
        (networkClientConfig->simulationTickRateHz > 0)
            ? networkClientConfig->simulationTickRateHz
            : 60u;

    // Parametres de pas de temps exact sans drift.
    const uint64_t tickBaseNs = rc2d_engine_getTickBaseDurationNs(simulationTickRateHz);
    const uint64_t tickRemainderNs = 1000000000ull % (uint64_t)simulationTickRateHz;
    uint64_t tickRemainderAccumulator = 0;

    // Limite anti spirale de la mort.
    const uint32_t maxCatchUpTicks = 5u;

    // Planifier la premiere echeance.
    uint64_t nextStepNs = rc2d_engine_consumeTickStepNs(
        simulationTickRateHz,
        tickBaseNs,
        tickRemainderNs,
        &tickRemainderAccumulator);
    uint64_t nextTickDeadlineNs = SDL_GetTicksNS() + nextStepNs;

    RC2D_log(RC2D_LOG_INFO, "Simulation thread started (tickRate=%u Hz).", simulationTickRateHz);

    // Boucle principale du thread simulation.
    while (rc2d_engine_workerThreadsShouldRun())
    {
        // Horloge courante.
        uint64_t nowNs = SDL_GetTicksNS();
        uint32_t catchUpCount = 0;

        // Rattrapage tant qu'on est en retard, borne par maxCatchUpTicks.
        while (nowNs >= nextTickDeadlineNs &&
               catchUpCount < maxCatchUpTicks &&
               rc2d_engine_workerThreadsShouldRun())
        {
            // Incremente le tick logique de simulation.
            rc2d_engine_state.simulation_tick_id += 1ull;

            // Appel du callback utilisateur.
            rc2d_engine_state.config->callbacks->rc2d_simulation_update(
                rc2d_engine_state.simulation_tick_id,
                nextStepNs,
                (double)nextStepNs / 1000000000.0);

            // Planifier le tick suivant.
            nextStepNs = rc2d_engine_consumeTickStepNs(
                simulationTickRateHz,
                tickBaseNs,
                tickRemainderNs,
                &tickRemainderAccumulator);
            nextTickDeadlineNs += nextStepNs;
            catchUpCount += 1u;
            nowNs = SDL_GetTicksNS();
        }

        // Si encore en retard apres la limite, on rebaseline.
        if (nowNs >= nextTickDeadlineNs)
        {
            nextStepNs = rc2d_engine_consumeTickStepNs(
                simulationTickRateHz,
                tickBaseNs,
                tickRemainderNs,
                &tickRemainderAccumulator);
            nextTickDeadlineNs = nowNs + nextStepNs;
        }

        // Attendre proprement la prochaine deadline.
        rc2d_engine_sleepUntilNs(nextTickDeadlineNs);
    }

    RC2D_log(RC2D_LOG_INFO, "Simulation thread stopped.");
    return 0;
}

/**
 * \brief Point d'entree du thread HTTP client.
 */
static int rc2d_engine_httpThreadMain(void* userData)
{
    // Argument de thread SDL non utilise.
    (void)userData;

    // Gardes de securite: config + callback HTTP.
    if (rc2d_engine_state.config == NULL ||
        rc2d_engine_state.config->callbacks == NULL ||
        rc2d_engine_state.config->callbacks->rc2d_http_update == NULL)
    {
        return 0;
    }

    RC2D_log(RC2D_LOG_INFO, "HTTP thread started.");

    // Boucle principale HTTP.
    while (rc2d_engine_workerThreadsShouldRun())
    {
        // Le contenu HTTP est pilote par l'utilisateur.
        rc2d_engine_state.config->callbacks->rc2d_http_update();
    }

    RC2D_log(RC2D_LOG_INFO, "HTTP thread stopped.");
    return 0;
}

/**
 * \brief Point d'entree du thread WebSocket client.
 */
static int rc2d_engine_websocketThreadMain(void* userData)
{
    // Argument de thread SDL non utilise.
    (void)userData;

    // Gardes de securite: config + callback WebSocket.
    if (rc2d_engine_state.config == NULL ||
        rc2d_engine_state.config->callbacks == NULL ||
        rc2d_engine_state.config->callbacks->rc2d_websocket_update == NULL)
    {
        return 0;
    }

    RC2D_log(RC2D_LOG_INFO, "WebSocket thread started.");

    // Boucle principale WebSocket.
    while (rc2d_engine_workerThreadsShouldRun())
    {
        // Le contenu WebSocket est pilote par l'utilisateur.
        rc2d_engine_state.config->callbacks->rc2d_websocket_update();
    }

    RC2D_log(RC2D_LOG_INFO, "WebSocket thread stopped.");
    return 0;
}

/**
 * \brief Dispatch un evenement ENet a la callback utilisateur incoming.
 *
 * Cette fonction centralise:
 * - l'appel de la callback utilisateur (si definie),
 * - la destruction automatique des packets RECEIVE apres callback.
 */
static void rc2d_engine_networkDispatchIncomingEvent(ENetHost* host, ENetEvent* event)
{
    // Gardes de securite: event obligatoires.
    if (event == NULL)
    {
        return;
    }

    // Appel de la callback utilisateur si elle existe.
    if (rc2d_engine_state.config != NULL &&
        rc2d_engine_state.config->callbacks != NULL &&
        rc2d_engine_state.config->callbacks->rc2d_network_incoming_update != NULL)
    {
        rc2d_engine_state.config->callbacks->rc2d_network_incoming_update(host, event);
    }

    // Destruction automatique des packets RECEIVE apres callback pour eviter les fuites de memoire.
    if (event->type == ENET_EVENT_TYPE_RECEIVE && event->packet != NULL)
    {
        enet_packet_destroy(event->packet);
        event->packet = NULL;
    }
}

/**
 * \brief Coupe la connexion ENet puis detruit le host client.
 */
static void rc2d_engine_networkDisconnectAndDestroy(void)
{
    // Si aucun host client n'existe, reset des etats reseau puis sortie.
    if (rc2d_engine_state.network_client_host == NULL)
    {
        rc2d_engine_state.network_server_peer = NULL;
        rc2d_engine_state.network_is_connected = false;
        return;
    }

    // Si un peer serveur existe, tenter une deconnexion propre.
    if (rc2d_engine_state.network_server_peer != NULL)
    {
        enet_peer_disconnect(rc2d_engine_state.network_server_peer, 0);

        ENetEvent event;
        while (enet_host_service(rc2d_engine_state.network_client_host, &event, 100) > 0)
        {
            rc2d_engine_networkDispatchIncomingEvent(
                rc2d_engine_state.network_client_host,
                &event);

            if (event.type == ENET_EVENT_TYPE_DISCONNECT ||
                event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
            {
                break;
            }
        }

        // Forcer la remise a zero du peer.
        enet_peer_reset(rc2d_engine_state.network_server_peer);
        rc2d_engine_state.network_server_peer = NULL;
    }

    // Flush + destruction du host client.
    enet_host_flush(rc2d_engine_state.network_client_host);
    enet_host_destroy(rc2d_engine_state.network_client_host);
    rc2d_engine_state.network_client_host = NULL;
    rc2d_engine_state.network_is_connected = false;
}

/**
 * \brief Tente une connexion ENet client -> serveur avec timeout.
 */
static bool rc2d_engine_networkTryConnect(void)
{
    // Config reseau obligatoire.
    if (rc2d_engine_state.config == NULL || rc2d_engine_state.config->networkClientConfig == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] Missing networkClientConfig.");
        return false;
    }

    // Raccourci local sur la config reseau.
    const RC2D_NetworkClientConfig* networkClientConfig = rc2d_engine_state.config->networkClientConfig;

    if (networkClientConfig->channelCount == 0)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] channelCount must be > 0.");
        return false;
    }

    // Cible effective de connexion (runtime prioritaire, sinon config).
    char serverAddressText[256];
    uint16_t serverPort = 0;
    if (!rc2d_engine_networkTryResolveConnectTarget(
            serverAddressText,
            sizeof(serverAddressText),
            &serverPort))
    {
        return false;
    }

    // Construire l'adresse serveur.
    ENetAddress serverAddress;
    SDL_memset(&serverAddress, 0, sizeof(serverAddress));
    enet_address_set_host(&serverAddress, ENET_ADDRESS_TYPE_ANY, serverAddressText);
    serverAddress.port = (enet_uint16)serverPort;

    // Nombre max de connexions cote host client.
    const uint32_t maxConnections =
        (networkClientConfig->maxConnections > 0) ? networkClientConfig->maxConnections : 1u;

    // Creer le host client ENet.
    rc2d_engine_state.network_client_host = enet_host_create(
        serverAddress.type,
        NULL,
        (size_t)maxConnections,
        (size_t)networkClientConfig->channelCount,
        (enet_uint32)networkClientConfig->incomingBandwidth,
        (enet_uint32)networkClientConfig->outgoingBandwidth);

    // Echec creation host.
    if (rc2d_engine_state.network_client_host == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] Failed to create ENet client host.");
        return false;
    }

    // Exécuter immédiatement après la création de l'hôte.
    // Ceci permet au code client d'installer au niveau de l'hôte (enet_host_encrypt / enet_host_compress) 
    // avant que la connexion ne démarre, et avant le premier enet_host_service().
    if (rc2d_engine_state.config != NULL &&
        rc2d_engine_state.config->callbacks != NULL &&
        rc2d_engine_state.config->callbacks->rc2d_network_host_setup != NULL)
    {
        rc2d_engine_state.config->callbacks->rc2d_network_host_setup(rc2d_engine_state.network_client_host);
    }

    // Demarrer la connexion vers le serveur.
    rc2d_engine_state.network_server_peer = enet_host_connect(
        rc2d_engine_state.network_client_host,
        &serverAddress,
        (size_t)networkClientConfig->channelCount,
        0);

    // Echec creation peer.
    if (rc2d_engine_state.network_server_peer == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] Failed to connect ENet peer.");
        enet_host_destroy(rc2d_engine_state.network_client_host);
        rc2d_engine_state.network_client_host = NULL;
        return false;
    }

    // Calcul de la deadline de connexion.
    const uint32_t connectTimeoutMs =
        (networkClientConfig->connectTimeoutMs > 0) ? networkClientConfig->connectTimeoutMs : 5000u;
    const uint64_t connectDeadlineNs =
        SDL_GetTicksNS() + ((uint64_t)connectTimeoutMs * 1000000ull);

    ENetEvent event;
    while (rc2d_engine_workerThreadsShouldRun() && SDL_GetTicksNS() < connectDeadlineNs)
    {
        // Poll ENet pendant la phase de connexion.
        const int serviceResult = enet_host_service(rc2d_engine_state.network_client_host, &event, 1);
        if (serviceResult < 0)
        {
            RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] enet_host_service failed during connect.");
            rc2d_engine_networkDisconnectAndDestroy();
            return false;
        }
        if (serviceResult == 0)
        {
            continue;
        }

        if (event.type == ENET_EVENT_TYPE_CONNECT)
        {
            rc2d_engine_state.network_is_connected = true;
        }

        rc2d_engine_networkDispatchIncomingEvent(
            rc2d_engine_state.network_client_host,
            &event);

        // Connexion validee.
        if (event.type == ENET_EVENT_TYPE_CONNECT)
        {
            return true;
        }

        if (event.type == ENET_EVENT_TYPE_DISCONNECT ||
            event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
        {
            rc2d_engine_networkDisconnectAndDestroy();
            return false;
        }
    }

    // Timeout de connexion.
    rc2d_engine_networkDisconnectAndDestroy();
    return false;
}

/**
 * \brief Point d'entree du thread reseau client (incoming + outgoing).
 */
static int rc2d_engine_networkThreadMain(void* userData)
{
    // Argument de thread SDL non utilise.
    (void)userData;

    // Config reseau obligatoire.
    if (rc2d_engine_state.config == NULL || rc2d_engine_state.config->networkClientConfig == NULL)
    {
        RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] Missing config, network thread stopped.");
        return 0;
    }

    // Raccourci local sur la config reseau.
    const RC2D_NetworkClientConfig* networkClientConfig = rc2d_engine_state.config->networkClientConfig;

    // Parametres de cadence reseau incoming/outgoing.
    const uint32_t incomingPollTimeoutMs =
        (networkClientConfig->incomingPollTimeoutMs > 0) ? networkClientConfig->incomingPollTimeoutMs : 1u;
    const uint32_t outgoingTickRateHz =
        (networkClientConfig->outgoingTickRateHz > 0) ? networkClientConfig->outgoingTickRateHz : 32u;
    const bool autoReconnectOnDisconnect = networkClientConfig->autoReconnectOnDisconnect;
    const uint32_t maxReconnectAttempts = networkClientConfig->maxReconnectAttempts;

    // Parametres de pas de temps outgoing exact.
    const uint64_t tickBaseNs = rc2d_engine_getTickBaseDurationNs(outgoingTickRateHz);
    const uint64_t tickRemainderNs = 1000000000ull % (uint64_t)outgoingTickRateHz;
    uint64_t tickRemainderAccumulator = 0;

    uint64_t nextStepNs = rc2d_engine_consumeTickStepNs(
        outgoingTickRateHz,
        tickBaseNs,
        tickRemainderNs,
        &tickRemainderAccumulator);
    uint64_t nextOutgoingDeadlineNs = SDL_GetTicksNS() + nextStepNs;
    uint64_t nextReconnectAttemptNs = 0;
    bool reconnectingAfterDisconnect = false;
    uint32_t reconnectAttemptCount = 0;

    // Marge finale avant la deadline outgoing.
    const uint64_t finalOutMarginNs = 200000ull; // 200 us

    // Budget maximum de travail incoming par iteration.
    const uint64_t maxIncomingWorkBudgetNs = 500000ull; // 500 us

    RC2D_log(RC2D_LOG_INFO, "Network thread started.");

    // Boucle principale du thread reseau.
    while (rc2d_engine_workerThreadsShouldRun())
    {
        // Traiter une demande explicite de deconnexion.
        if (SDL_GetAtomicInt(&rc2d_engine_state.network_disconnect_requested) != 0)
        {
            rc2d_engine_networkDisconnectAndDestroy();
            SDL_SetAtomicInt(&rc2d_engine_state.network_disconnect_requested, 0);
            nextReconnectAttemptNs = 0;
            reconnectingAfterDisconnect = false;
            reconnectAttemptCount = 0;
        }

        // Si aucune connexion n'est desiree, le thread reseau reste idle.
        if (SDL_GetAtomicInt(&rc2d_engine_state.network_connect_desired) == 0)
        {
            SDL_DelayPrecise(1000000ull);
            continue;
        }

        // Si pas connecte, tenter une reconnexion periodique.
        if (!rc2d_engine_state.network_is_connected ||
            rc2d_engine_state.network_client_host == NULL ||
            rc2d_engine_state.network_server_peer == NULL)
        {
            const uint64_t nowNs = SDL_GetTicksNS();
            if (nowNs >= nextReconnectAttemptNs)
            {
                if (rc2d_engine_networkTryConnect())
                {
                    nextOutgoingDeadlineNs = nowNs + nextStepNs;
                    nextReconnectAttemptNs = 0;
                    reconnectingAfterDisconnect = false;
                    reconnectAttemptCount = 0;
                }
                else
                {
                    if (reconnectingAfterDisconnect &&
                        maxReconnectAttempts > 0 &&
                        reconnectAttemptCount < maxReconnectAttempts)
                    {
                        reconnectAttemptCount += 1u;
                        if (reconnectAttemptCount >= maxReconnectAttempts)
                        {
                            RC2D_log(
                                RC2D_LOG_WARN,
                                "[RC2D][NET] Max reconnect attempts reached (%u). Switching to idle mode.",
                                (unsigned)maxReconnectAttempts);
                            SDL_SetAtomicInt(&rc2d_engine_state.network_connect_desired, 0);
                            reconnectingAfterDisconnect = false;
                            reconnectAttemptCount = 0;
                            nextReconnectAttemptNs = 0;
                            continue;
                        }
                    }

                    nextReconnectAttemptNs = nowNs + 3000000000ull;
                }
            }

            // Petit backoff de reconnexion (1 ms) avec attente precise.
            SDL_DelayPrecise(1000000ull);
            continue;
        }

        // Calculer un timeout incoming borne par la prochaine deadline outgoing.
        uint32_t timeoutMs = incomingPollTimeoutMs;
        const uint64_t nowBeforePollNs = SDL_GetTicksNS();
        const uint64_t outRemainingBeforePollNs =
            (nowBeforePollNs < nextOutgoingDeadlineNs)
                ? (nextOutgoingDeadlineNs - nowBeforePollNs)
                : 0ull;

        if (outRemainingBeforePollNs <= finalOutMarginNs)
        {
            timeoutMs = 0u;
        }
        else
        {
            const uint64_t safeWaitNs = outRemainingBeforePollNs - finalOutMarginNs;
            const uint32_t safeWaitMs = (uint32_t)(safeWaitNs / 1000000ull);

            if (safeWaitMs < timeoutMs)
            {
                timeoutMs = safeWaitMs;
            }

            if (safeWaitMs == 0u)
            {
                timeoutMs = 0u;
            }
        }

        // Poll incoming ENet.
        ENetEvent event;
        int serviceResult = enet_host_service(
            rc2d_engine_state.network_client_host,
            &event,
            (enet_uint32)timeoutMs);

        // Erreur ENet incoming.
        if (serviceResult < 0)
        {
            RC2D_log(RC2D_LOG_ERROR, "[RC2D][NET] enet_host_service failed.");
            rc2d_engine_networkDisconnectAndDestroy();
            continue;
        }

        // Drain des evenements disponibles avec budget temps incoming.
        while (serviceResult > 0)
        {
            rc2d_engine_networkDispatchIncomingEvent(
                rc2d_engine_state.network_client_host,
                &event);

            if (event.type == ENET_EVENT_TYPE_DISCONNECT ||
                event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
            {
                if (autoReconnectOnDisconnect)
                {
                    reconnectingAfterDisconnect = true;
                    reconnectAttemptCount = 0;
                }
                else
                {
                    SDL_SetAtomicInt(&rc2d_engine_state.network_connect_desired, 0);
                }
                rc2d_engine_networkDisconnectAndDestroy();
                break;
            }

            const uint64_t nowDuringDrainNs = SDL_GetTicksNS();
            const uint64_t incomingBudgetUsedNs = nowDuringDrainNs - nowBeforePollNs;

            // Stop drain si budget incoming consomme.
            if (incomingBudgetUsedNs >= maxIncomingWorkBudgetNs)
            {
                break;
            }

            // Stop drain si on entre dans la marge finale outgoing.
            const uint64_t outRemainingDuringDrainNs =
                (nowDuringDrainNs < nextOutgoingDeadlineNs)
                    ? (nextOutgoingDeadlineNs - nowDuringDrainNs)
                    : 0ull;
            if (outRemainingDuringDrainNs <= finalOutMarginNs)
            {
                break;
            }

            serviceResult = enet_host_service(rc2d_engine_state.network_client_host, &event, 0);
        }

        // Cadence outgoing avec rattrapage borne.
        uint64_t nowNs = SDL_GetTicksNS();
        uint32_t catchUpCount = 0;
        while (nowNs >= nextOutgoingDeadlineNs &&
               catchUpCount < 5u &&
               rc2d_engine_workerThreadsShouldRun())
        {
            if (rc2d_engine_state.config != NULL &&
                rc2d_engine_state.config->callbacks != NULL &&
                rc2d_engine_state.config->callbacks->rc2d_network_outgoing_update != NULL)
            {
                rc2d_engine_state.config->callbacks->rc2d_network_outgoing_update(
                    rc2d_engine_state.network_client_host);
            }

            nextStepNs = rc2d_engine_consumeTickStepNs(
                outgoingTickRateHz,
                tickBaseNs,
                tickRemainderNs,
                &tickRemainderAccumulator);
            nextOutgoingDeadlineNs += nextStepNs;
            catchUpCount += 1u;
            nowNs = SDL_GetTicksNS();
        }

        // Si toujours en retard: rebaseline outgoing.
        if (nowNs >= nextOutgoingDeadlineNs)
        {
            nextStepNs = rc2d_engine_consumeTickStepNs(
                outgoingTickRateHz,
                tickBaseNs,
                tickRemainderNs,
                &tickRemainderAccumulator);
            nextOutgoingDeadlineNs = nowNs + nextStepNs;
        }
    }

    // Nettoyage reseau a la sortie du thread.
    rc2d_engine_networkDisconnectAndDestroy();
    RC2D_log(RC2D_LOG_INFO, "Network thread stopped.");
    return 0;
}
#endif

/**
 * \brief Convertit les coordonnées des événements d'entrée en coordonnées de rendu.
 *
 * Cette fonction convertit les coordonnées des événements d'entrée (souris, tactile, stylet..etc)
 * en coordonnées de rendu en utilisant le renderer GPU actuel.
 * 
 * \param {SDL_Event*} event - L'événement SDL à convertir.
 * 
 * \since Cette fonction est disponible depuis RC2D 1.0.0.
 */
static void rc2d_engine_convertEventToRender(SDL_Event* event)
{
    switch (event->type) {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL: 
        case SDL_EVENT_FINGER_DOWN: 
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_CANCELED:
            if(!SDL_ConvertEventToRenderCoordinates(rc2d_engine_state.renderer, event)) 
            {
                RC2D_log(RC2D_LOG_ERROR, "Erreur lors de la conversion des coordonnées de l'événement en coordonnées de rendu : %s", SDL_GetError());
            }
            break;
        default: break;
    }
}

SDL_AppResult rc2d_engine_processevent(SDL_Event *event) 
{
    /**
     * Convertit les coordonnées de l'événement en coordonnées de rendu.
     * Utile pour les événements d'entrée (souris, tactile, stylet..etc)
     * afin qu'ils correspondent correctement à la zone de rendu actuelle.
     * \note Doit être appelé avant de traiter l'événement.
     */
    rc2d_engine_convertEventToRender(event);

    // Quit program
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    // Le presse-papiers ou la sélection principale a changé
    else if (event->type == SDL_EVENT_CLIPBOARD_UPDATE)
    {
        if (rc2d_engine_state.config != NULL &&
            rc2d_engine_state.config->callbacks != NULL &&
            rc2d_engine_state.config->callbacks->rc2d_clipboardupdated != NULL)
        {
            const SDL_ClipboardEvent* e = &event->clipboard;

            RC2D_ClipboardEventInfo info = {
                .is_owner = e->owner,
                .num_mime_types = e->num_mime_types,
                .mime_types = e->mime_types
            };

            rc2d_engine_state.config->callbacks->rc2d_clipboardupdated(&info);
        }
    }

    else if (event->type == SDL_EVENT_CAMERA_DEVICE_ADDED ||
            event->type == SDL_EVENT_CAMERA_DEVICE_REMOVED ||
            event->type == SDL_EVENT_CAMERA_DEVICE_APPROVED ||
            event->type == SDL_EVENT_CAMERA_DEVICE_DENIED)
    {
        const SDL_CameraDeviceEvent* e = &event->cdevice;

        RC2D_CameraEventInfo info = {
            .deviceID = e->which
        };

        if (rc2d_engine_state.config && rc2d_engine_state.config->callbacks) 
        {
            switch (event->type) 
            {
                case SDL_EVENT_CAMERA_DEVICE_ADDED:
                    if (rc2d_engine_state.config->callbacks->rc2d_cameraadded)
                        rc2d_engine_state.config->callbacks->rc2d_cameraadded(&info);
                    break;
                case SDL_EVENT_CAMERA_DEVICE_REMOVED:
                    if (rc2d_engine_state.config->callbacks->rc2d_cameraremoved)
                        rc2d_engine_state.config->callbacks->rc2d_cameraremoved(&info);
                    break;
                case SDL_EVENT_CAMERA_DEVICE_APPROVED:
                    if (rc2d_engine_state.config->callbacks->rc2d_cameraapproved)
                        rc2d_engine_state.config->callbacks->rc2d_cameraapproved(&info);
                    break;
                case SDL_EVENT_CAMERA_DEVICE_DENIED:
                    if (rc2d_engine_state.config->callbacks->rc2d_cameradenied)
                        rc2d_engine_state.config->callbacks->rc2d_cameradenied(&info);
                    break;
            }
        }
    }

    else if (event->type == SDL_EVENT_WILL_ENTER_FOREGROUND) 
    {
/**
 * Appelez SDL_GDKResumeGPU pour reprendre le fonctionnement du GPU sur Xbox 
 * lorsqu'on recoit l'événement : SDL_EVENT_WILL_ENTER_FOREGROUND.
 * 
 * IMPORTANT: Lors de la reprise, cette fonction (SDL_GDKResumeGPU) DOIT être appelée 
 * avant d'appeler toute autre fonction SDL_GPU .
 */
#if defined(RC2D_PLATFORM_XBOXSERIES) || defined(RC2D_PLATFORM_XBOXONE)
    SDL_GDKResumeGPU(rc2d_gpu_getDevice());
#endif 
    }

    else if (event->type == SDL_EVENT_DID_ENTER_BACKGROUND) 
    {
/**
 * Appelez SDL_GDKSuspendGPU pour suspendre le fonctionnement du GPU sur Xbox 
 * lorsqu'on recoit l'événement : SDL_EVENT_DID_ENTER_BACKGROUND.
 * 
 * IMPORTANT: N'appelez aucune fonction SDL_GPU après avoir appelé cette fonction (SDL_GDKSuspendGPU) ! 
 * Celle-ci doit également être appelée avant SDL_GDKSuspendComplete .
 */
#if defined(RC2D_PLATFORM_XBOXSERIES) || defined(RC2D_PLATFORM_XBOXONE)
    SDL_GDKSuspendGPU(rc2d_gpu_getDevice());
#endif
    }

    // La préférence de la langue locale a changé
    else if (event->type == SDL_EVENT_LOCALE_CHANGED)
    {
        if (rc2d_engine_state.config != NULL &&
            rc2d_engine_state.config->callbacks != NULL &&
            rc2d_engine_state.config->callbacks->rc2d_localechanged != NULL) 
        {
            RC2D_Locale* locales = rc2d_local_getPreferredLocales();
            rc2d_engine_state.config->callbacks->rc2d_localechanged(locales);
            rc2d_local_freeLocales(locales);
        }
    }

    // Quand l'orientation de l'affichage change
    else if (event->type == SDL_EVENT_DISPLAY_ORIENTATION) 
    {
		/**
		 * Recalculer le viewport GPU et le render scale, puisque l'orientation de l'affichage a changé.
		 * Cela est nécessaire pour s'assurer que le rendu s'adapte correctement à la nouvelle orientation.
		 */
		rc2d_engine_presentationUpdate();
		rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_monitororientationchanged != NULL) 
        {
            RC2D_DisplayOrientation newOrientation = rc2d_window_getDisplayOrientation();
            rc2d_engine_state.config->callbacks->rc2d_monitororientationchanged(event->display.displayID, newOrientation);
        }
    }

        // Monitor Added
    else if (event->type == SDL_EVENT_DISPLAY_ADDED)
    {
		rc2d_engine_presentationUpdate();
		rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_monitoradded != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_monitoradded(event->display.displayID);
        }
    }

    // Monitor Removed
    else if (event->type == SDL_EVENT_DISPLAY_REMOVED)
    {
		rc2d_engine_presentationUpdate();
		rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_monitorremoved != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_monitorremoved(event->display.displayID);
        }
    }

    // Monitor Moved
    else if (event->type == SDL_EVENT_DISPLAY_MOVED)
    {
		rc2d_engine_presentationUpdate();
		rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_monitormoved != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_monitormoved(event->display.displayID);
        }
    }

    // Monitor Desktop Mode Changed
    else if (event->type == SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED)
    {
		rc2d_engine_presentationUpdate();
		rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_monitordesktopmodechanged != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_monitordesktopmodechanged(event->display.displayID);
        }
    }

    // Monitor Current Mode Changed
    else if (event->type == SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED)
    {
		rc2d_engine_presentationUpdate();
		rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_monitorcurrentmodechanged != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_monitorcurrentmodechanged(event->display.displayID);
        }
    }

    // Monitor Content Scale Changed
    else if (event->type == SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED)
    {
        rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
    }

    // Window HDR State changed
    else if (event->type == SDL_EVENT_WINDOW_HDR_STATE_CHANGED ||
            event->type == SDL_EVENT_WINDOW_ICCPROF_CHANGED)
    {
		rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
		
        // Re-set le meilleur swapchain disponible
        if (!rc2d_engine_configure_swapchain())
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to update swapchain on HDR state change: %s", SDL_GetError());
        }
    }

    else if (event->type == SDL_EVENT_FINGER_DOWN ||
            event->type == SDL_EVENT_FINGER_UP ||
            event->type == SDL_EVENT_FINGER_MOTION ||
            event->type == SDL_EVENT_FINGER_CANCELED)
    {
        const SDL_TouchFingerEvent* e = &event->tfinger;

        RC2D_TouchEventInfo info = {
            .touchID = e->touchID,
            .fingerID = e->fingerID,
            .x = e->x,
            .y = e->y,
            .dx = e->dx,
            .dy = e->dy,
            .pressure = e->pressure
        };

        if (rc2d_engine_state.config && rc2d_engine_state.config->callbacks)
        {
            switch (event->type) 
            {
                case SDL_EVENT_FINGER_DOWN:
                    if (rc2d_engine_state.config->callbacks->rc2d_touchpressed)
                        rc2d_engine_state.config->callbacks->rc2d_touchpressed(&info);
                    break;
                case SDL_EVENT_FINGER_UP:
                    if (rc2d_engine_state.config->callbacks->rc2d_touchreleased)
                        rc2d_engine_state.config->callbacks->rc2d_touchreleased(&info);
                    break;
                case SDL_EVENT_FINGER_MOTION:
                    if (rc2d_engine_state.config->callbacks->rc2d_touchmoved)
                        rc2d_engine_state.config->callbacks->rc2d_touchmoved(&info);
                    break;
                case SDL_EVENT_FINGER_CANCELED:
                    if (rc2d_engine_state.config->callbacks->rc2d_touchcanceled)
                        rc2d_engine_state.config->callbacks->rc2d_touchcanceled(&info);
                    break;
            }
        }

        // Mise à jour de l’état du toucher :
        rc2d_touch_updateState(info.touchID, info.fingerID, event->type, info.pressure, info.x, info.y);
    }

    // Window safe area changed
    else if (event->type == SDL_EVENT_WINDOW_SAFE_AREA_CHANGED) 
    {
        /**
         * Quand la zone de sécurité de la fenêtre change,
         * on indique que le viewport du gpu et render scale doit être recalculé.
         */
        rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
    }

    // Window enter fullscreen
    else if (event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN) 
    {
        /**
         * Quand la fenêtre entre en mode plein écran, 
         * on met à jour la largeur et la hauteur de la fenêtre
         * on met à jour les FPS en fonction du moniteur actuel
         * et on indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
        rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowenterfullscreen != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowenterfullscreen();
        }
    }

    // Window Shown
    else if (event->type == SDL_EVENT_WINDOW_SHOWN)
    {
		rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowshown != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowshown();
        }
    }

    // Window usable bounds changed
    else if (event->type == SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED)
    {
        rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();
    }

    // Window leave fullscreen
    else if (event->type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN) 
    {
        /**
         * Quand la fenêtre quitte le mode plein écran, 
         * on met à jour la largeur et la hauteur de la fenêtre
         * on met à jour les FPS en fonction du moniteur actuel
         * et on indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
        rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowleavefullscreen != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowleavefullscreen();
        }
    }

	// Window Metal view resized
    else if (event->type == SDL_EVENT_WINDOW_METAL_VIEW_RESIZED)
    {
        rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
    }

    // Window pixel size changed
    else if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) 
    {
        /**
         * En cas de changement de taille de pixels de la fenêtre (ex: changement de DPI), 
         * On indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
        rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
    }

    // Window display scale changed
    else if (event->type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED) 
    {
        /** 
         * En cas de changement d'échelle d'affichage de la fenêtre, 
         * On indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
        rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
    }

    // Window Occluded
    else if (event->type == SDL_EVENT_WINDOW_OCCLUDED)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowoccluded != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowoccluded();
        }
    }

    // Window resized
    else if (event->type == SDL_EVENT_WINDOW_RESIZED) 
    {
        /** 
         * En cas de changement de la taille de la fenêtre,
         * on met à jour la largeur et la hauteur de la fenêtre 
         * et on indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
        rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowresized != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowresized(event->window.data1, event->window.data2);
        }
    }

    // Window moved
    else if (event->type == SDL_EVENT_WINDOW_MOVED) 
    {
		rc2d_engine_presentationUpdate();
        rc2d_engine_update_fps_based_on_monitor();
		
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowmoved != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowmoved(event->window.data1, event->window.data2);
        }
    }

    // Window display changed
    else if (event->type == SDL_EVENT_WINDOW_DISPLAY_CHANGED) 
    {
        /**
         * Quand la fenêtre change de moniteur,
         * on met à jour la largeur et la hauteur de la fenêtre
         * on met à jour les FPS en fonction du moniteur actuel
         * et on indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
        rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowdisplaychanged != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowdisplaychanged(event->window.data1);
        }
    }

    // Window exposed
    else if (event->type == SDL_EVENT_WINDOW_EXPOSED) 
    {
        /**
         * Quand la fenêtre est exposée (par exemple, après avoir été masquée ou minimisée),
         * on indique que le viewport du gpu et le render scale interne doit être recalculé.
         * 
         * Egalement si jamais entre temps on a changé de moniteur,
         * on met à jour les FPS en fonction du moniteur actuel.
         */
        rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowexposed != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowexposed();
        }
    }

    // Window minimized
    else if (event->type == SDL_EVENT_WINDOW_MINIMIZED) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowminimized != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowminimized();
        }
    }

    // Window maximized
    else if (event->type == SDL_EVENT_WINDOW_MAXIMIZED) 
    {
        /** 
         * En cas de changement de la taille de la fenêtre,
         * on met à jour la largeur et la hauteur de la fenêtre 
         * et on indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
		rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowmaximized != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowmaximized();
        }
    }

    // Window restored
    else if (event->type == SDL_EVENT_WINDOW_RESTORED) 
    {
        /** 
         * La fenêtre a été restaurée après avoir été minimisée ou maximisée à son état normal.
         * on met à jour la largeur et la hauteur de la fenêtre 
         * et on indique que le viewport du gpu et le render scale interne doit être recalculé.
         */
		rc2d_engine_update_fps_based_on_monitor();
        rc2d_engine_presentationUpdate();

        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowrestored != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowrestored();
        }
    }

    // Mouse entered window
    else if (event->type == SDL_EVENT_WINDOW_MOUSE_ENTER) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowmouseenter != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowmouseenter();
        }
    }

    // Mouse leave window
    else if (event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowmouseleave != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowmouseleave();
        }
    }

    // Keyboard focus gained
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_GAINED) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowkeyboardfocus != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowkeyboardfocus();
        }
    }

    // Keyboard focus lost
    else if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowkeyboardlost != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_windowkeyboardlost();
        }
    }

    // Window closed
    else if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_windowclosed != NULL)
        {
            rc2d_engine_state.config->callbacks->rc2d_windowclosed();
        }
        
        return SDL_APP_SUCCESS;
    }
    
    // Mouse Moved
    else if (event->type == SDL_EVENT_MOUSE_MOTION) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_mousemoved != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_mousemoved(
                event->motion.x,
                event->motion.y,
                event->motion.xrel,
                event->motion.yrel,
                event->motion.which
            );
        }
    }

    // Mouse Wheel
    else if (event->type == SDL_EVENT_MOUSE_WHEEL) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_mousewheelmoved != NULL) 
        {
            RC2D_MouseWheelDirection direction = RC2D_SCROLL_NONE;
            float x = event->wheel.x;
            float y = event->wheel.y;

            // Ajuster les valeurs en fonction de la direction (normal ou flipped)
            if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) 
            {
                x *= -1.0f;
                y *= -1.0f;
            }

            // Déterminer la direction principale
            if (y > 0.0f) 
            {
                direction = RC2D_SCROLL_UP;
            } 
            else if (y < 0.0f) 
            {
                direction = RC2D_SCROLL_DOWN;
            } 
            else if (x > 0.0f) 
            {
                direction = RC2D_SCROLL_RIGHT;
            } 
            else if (x < 0.0f) 
            {
                direction = RC2D_SCROLL_LEFT;
            }

            rc2d_engine_state.config->callbacks->rc2d_mousewheelmoved(
                direction,
                x,
                y,
                event->wheel.integer_x,
                event->wheel.integer_y,
                event->wheel.mouse_x,
                event->wheel.mouse_y,
                event->wheel.which
            );
        }
    }

    // Mouse Pressed
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_mousepressed != NULL) 
        {
            RC2D_MouseButton button = RC2D_MOUSE_BUTTON_UNKNOWN;

            switch (event->button.button) 
            {
                case SDL_BUTTON_LEFT:   button = RC2D_MOUSE_BUTTON_LEFT; break;
                case SDL_BUTTON_MIDDLE: button = RC2D_MOUSE_BUTTON_MIDDLE; break;
                case SDL_BUTTON_RIGHT:  button = RC2D_MOUSE_BUTTON_RIGHT; break;
                case SDL_BUTTON_X1:     button = RC2D_MOUSE_BUTTON_X1; break;
                case SDL_BUTTON_X2:     button = RC2D_MOUSE_BUTTON_X2; break;
                default:                button = RC2D_MOUSE_BUTTON_UNKNOWN; break;
            }

            rc2d_engine_state.config->callbacks->rc2d_mousepressed(
                event->button.x,
                event->button.y,
                button,
                event->button.clicks,
                event->button.which
            );
        }
    }

    // Mouse Released
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_mousereleased != NULL) 
        {
            RC2D_MouseButton button = RC2D_MOUSE_BUTTON_UNKNOWN;

            switch (event->button.button) 
            {
                case SDL_BUTTON_LEFT:   button = RC2D_MOUSE_BUTTON_LEFT; break;
                case SDL_BUTTON_MIDDLE: button = RC2D_MOUSE_BUTTON_MIDDLE; break;
                case SDL_BUTTON_RIGHT:  button = RC2D_MOUSE_BUTTON_RIGHT; break;
                case SDL_BUTTON_X1:     button = RC2D_MOUSE_BUTTON_X1; break;
                case SDL_BUTTON_X2:     button = RC2D_MOUSE_BUTTON_X2; break;
                default:                button = RC2D_MOUSE_BUTTON_UNKNOWN; break;
            }

            rc2d_engine_state.config->callbacks->rc2d_mousereleased(
                event->button.x,
                event->button.y,
                button,
                event->button.clicks,
                event->button.which
            );
        }
    }

    // Mouse Added
    else if (event->type == SDL_EVENT_MOUSE_ADDED) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_mouseadded != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_mouseadded(event->mdevice.which);
        }
    }

    // Mouse Removed
    else if (event->type == SDL_EVENT_MOUSE_REMOVED) 
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_mouseremoved != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_mouseremoved(event->mdevice.which);
        }
    }

    // Keyboard Pressed
    else if (event->type == SDL_EVENT_KEY_DOWN)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_keypressed != NULL) 
        {
            const char* key_name = SDL_GetKeyName(event->key.key);
            rc2d_engine_state.config->callbacks->rc2d_keypressed(
                key_name,
                event->key.scancode,
                event->key.key,
                event->key.mod,
                event->key.repeat,
                event->key.which
            );
        }
    }

    // Keyboard Released
    else if (event->type == SDL_EVENT_KEY_UP)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_keyreleased != NULL) 
        {
            const char* key_name = SDL_GetKeyName(event->key.key);
            rc2d_engine_state.config->callbacks->rc2d_keyreleased(
                key_name,
                event->key.scancode,
                event->key.key,
                event->key.mod,
                event->key.which
            );
        }
    }

    // Text Editing (IME)
    else if (event->type == SDL_EVENT_TEXT_EDITING)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_textediting != NULL) 
        {
            RC2D_TextEditingEventInfo info = {
                .text = event->edit.text,
                .start = event->edit.start,
                .length = event->edit.length,
                .windowID = event->edit.windowID
            };
            rc2d_engine_state.config->callbacks->rc2d_textediting(&info);
        }
    }

    // Text Editing Candidates (IME)
    else if (event->type == SDL_EVENT_TEXT_EDITING_CANDIDATES)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_texteditingcandidates != NULL) 
        {
            RC2D_TextEditingCandidatesEventInfo info = {
                .candidates = event->edit_candidates.candidates,
                .num_candidates = event->edit_candidates.num_candidates,
                .selected_candidate = event->edit_candidates.selected_candidate,
                .horizontal = event->edit_candidates.horizontal,
                .windowID = event->edit_candidates.windowID
            };
            rc2d_engine_state.config->callbacks->rc2d_texteditingcandidates(&info);
        }
    }

    // Text Input
    else if (event->type == SDL_EVENT_TEXT_INPUT)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_textinput != NULL) 
        {
            RC2D_TextInputEventInfo info = {
                .text = event->text.text,
                .windowID = event->text.windowID
            };
            rc2d_engine_state.config->callbacks->rc2d_textinput(&info);
        }
    }

    // Keymap Changed
    else if (event->type == SDL_EVENT_KEYMAP_CHANGED)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_keymapchanged != NULL) 
        {
            rc2d_engine_state.config->callbacks->rc2d_keymapchanged();
        }
    }

    // Keyboard Added
    else if (event->type == SDL_EVENT_KEYBOARD_ADDED)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_keyboardadded != NULL) 
        {
            RC2D_KeyboardDeviceEventInfo info = {
                .keyboardID = event->kdevice.which,
                .name = SDL_GetKeyboardNameForID(event->kdevice.which)
            };
            rc2d_engine_state.config->callbacks->rc2d_keyboardadded(&info);
        }
    }

    // Keyboard Removed
    else if (event->type == SDL_EVENT_KEYBOARD_REMOVED)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_keyboardremoved != NULL) 
        {
            RC2D_KeyboardDeviceEventInfo info = {
                .keyboardID = event->kdevice.which,
                .name = SDL_GetKeyboardNameForID(event->kdevice.which)
            };
            rc2d_engine_state.config->callbacks->rc2d_keyboardremoved(&info);
        }
    }

    // Sensor Update
    else if (event->type == SDL_EVENT_SENSOR_UPDATE)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_sensorupdate != NULL) 
        {
            RC2D_SensorEventInfo info = {
                .sensorID  = event->sensor.which,
                .type      = SDL_SENSOR_INVALID,
                .name      = NULL,
                .timestamp = event->sensor.sensor_timestamp
            };

            SDL_Sensor *sensor = SDL_GetSensorFromID(event->sensor.which);
            if (sensor) {
                info.type = SDL_GetSensorType(sensor);
                info.name = SDL_GetSensorName(sensor);
            }

            for (int i = 0; i < 6; i++) {
                info.data[i] = event->sensor.data[i];
            }

            rc2d_engine_state.config->callbacks->rc2d_sensorupdate(&info);
        }
    }

    // Drag-and-Drop Events
    else if (event->type == SDL_EVENT_DROP_BEGIN)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_dropbegin != NULL) 
        {
            RC2D_DropEventInfo info = {
                .windowID = event->drop.windowID,
                .x = 0.0f,
                .y = 0.0f,
                .source = event->drop.source,
                .data = NULL,
                .timestamp = event->drop.timestamp
            };
            rc2d_engine_state.config->callbacks->rc2d_dropbegin(&info);
        }
    }
    else if (event->type == SDL_EVENT_DROP_FILE)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_dropfile != NULL) 
        {
            RC2D_DropEventInfo info = {
                .windowID = event->drop.windowID,
                .x = event->drop.x,
                .y = event->drop.y,
                .source = event->drop.source,
                .data = event->drop.data,
                .timestamp = event->drop.timestamp
            };
            rc2d_engine_state.config->callbacks->rc2d_dropfile(&info);
        }
    }
    else if (event->type == SDL_EVENT_DROP_TEXT)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_droptext != NULL) 
        {
            RC2D_DropEventInfo info = {
                .windowID = event->drop.windowID,
                .x = event->drop.x,
                .y = event->drop.y,
                .source = event->drop.source,
                .data = event->drop.data,
                .timestamp = event->drop.timestamp
            };
            rc2d_engine_state.config->callbacks->rc2d_droptext(&info);
        }
    }
    else if (event->type == SDL_EVENT_DROP_COMPLETE)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_dropcomplete != NULL) 
        {
            RC2D_DropEventInfo info = {
                .windowID = event->drop.windowID,
                .x = 0.0f,
                .y = 0.0f,
                .source = event->drop.source,
                .data = NULL,
                .timestamp = event->drop.timestamp
            };
            rc2d_engine_state.config->callbacks->rc2d_dropcomplete(&info);
        }
    }
    else if (event->type == SDL_EVENT_DROP_POSITION)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_dropposition != NULL) 
        {
            RC2D_DropEventInfo info = {
                .windowID = event->drop.windowID,
                .x = event->drop.x,
                .y = event->drop.y,
                .source = event->drop.source,
                .data = NULL,
                .timestamp = event->drop.timestamp
            };
            rc2d_engine_state.config->callbacks->rc2d_dropposition(&info);
        }
    }
    // System Theme Changed
    else if (event->type == SDL_EVENT_SYSTEM_THEME_CHANGED)
    {
        if (rc2d_engine_state.config != NULL && 
            rc2d_engine_state.config->callbacks != NULL && 
            rc2d_engine_state.config->callbacks->rc2d_systemthemechanged != NULL) 
        {
            SDL_SystemTheme theme = SDL_GetSystemTheme();
            rc2d_engine_state.config->callbacks->rc2d_systemthemechanged(theme);
        }
    }

    /**
     * SDL_APP_CONTINUE : Cela indique que l'application 
     * doit continuer à traiter les événements.
     */
    return SDL_APP_CONTINUE;
}

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
static bool rc2d_engine(void)
{
    /**
     * IMPORTANT:
     * 
     * Appel de SDL_GetPath() explicitement, la raison : 
     * 
     * SDL met en cache le résultat de cet appel en interne, mais le premier appel à cette fonction 
     * n'est pas nécessairement rapide, alors planifiez en conséquence.
     */
    SDL_GetBasePath();

    /**
     * Doit être appelé avant tout code pour initialiser les asserts 
     * et les utiliser dès le début de l'application.
     */
    rc2d_assert_init();
    
    /**
     * Set les informations de l'application.
     * Dois toujours etre fait avant d'initialiser SDL3
     */
    if(!SDL_SetAppMetadata(rc2d_engine_state.config->appInfo->name, rc2d_engine_state.config->appInfo->version, rc2d_engine_state.config->appInfo->identifier))
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible de définir les métadonnées de l'application : %s\n", SDL_GetError());
        return false;
    }
    if(!SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, rc2d_engine_state.config->appInfo->organization))
    {
        RC2D_log(RC2D_LOG_ERROR, "Erreur : impossible de définir la propriété 'organization' de l'application : %s\n", SDL_GetError());
        return false;
    }

    /**
     * Initialiser la librairie OpenSSL
     */
    if (!rc2d_engine_init_openssl())
    {
        return false;
    }

    /**
     * Initialiser la librairie SDL3_mixer
     */
    if (!rc2d_engine_init_sdlmixer())
    {
        return false;
    }

	/**
     * Initialiser la librairie SDL3
     */
    if (!rc2d_engine_init_sdl())
    {
        return false;
    }

    /**
     * Initialiser la librairie SDL3_shadercross
     */
    if (!rc2d_engine_init_sdlshadercross())
    {
        return false;
    }

    /**
     * Initialiser la librairie RCENet
     */
    if (!rc2d_engine_init_rcenet())
    {
        return false;
    }

    /**
     * Initialiser la librairie libsodium
     */
    if (!rc2d_engine_init_libsodium())
    {
        return false;
    }

    /**
     * Initialiser la librairie curl
     */
    if (!rc2d_engine_init_curl())
    {
        return false;
    }

    /**
     * Vérifier si le GPU de l'utilisateur est supporté par l'API SDL3_GPU.
     * 
     * Cela permet de s'assurer que le GPU est compatible avec au 
     * moins un des backends supportés par SDL3_GPU.
     * 
     * Si le GPU n'est pas supporté, on ne peut pas continuer.
     */
    if (!rc2d_engine_supported_gpu_backends())
    {
        return false;
    }

    /**
     * Créer la fenêtre principale
     */
    if (!rc2d_engine_create_window())
    {
        return false;
    }

    /**
     * Initialiser et créer le renderer GPU avec SDL3_GPU.
     */
    if (!rc2d_engine_create_renderergpu())
    {
        return false;
    }

    /**
     * Initialiser la librairie SDL3_ttf
     */
    if (!rc2d_engine_init_sdlttf())
    {
        return false;
    }

    /**
     * Charger la base de données des manettes de jeu (gamepads) embarquée, issue de SDL_GameControllerDB.
     */
    if(!rc2d_engine_gamepad_load_embedded_db())
    {
        return false;
    }

#if RC2D_STEAMWORKS_SDK_ENABLED
    /**
     * Initialiser le SDK Steamworks pour l'intégration Steam.
     */
    if(!rc2d_steamworks_init())
    {
        return false;
    }
    rc2d_steamworks_runCallbacks();
#endif // RC2D_STEAMWORKS_SDK_ENABLED

#if RC2D_EOS_SDK_ENABLED
    /**
     * Initialiser le SDK Epic Online Services (EOS) pour l'intégration EOS.
     */
    if(!rc2d_eos_init())
    {
        return false;
    }
    rc2d_eos_platformTick();
#endif // RC2D_EOS_SDK_ENABLED

    /**
     * Activer le blending (alpha) dans SDL3.
     */
	if (!SDL_SetRenderDrawBlendMode(rc2d_engine_state.renderer, SDL_BLENDMODE_BLEND))
    {
        RC2D_log(RC2D_LOG_WARN, "Erreur : impossible d'activer le blending (alpha) : %s\n", SDL_GetError());
    }

    /**
     * Configurer le mode de mise à l'échelle des textures selon le choix utilisateur.
     */
    if (!rc2d_engine_setTextureScaleMode(rc2d_engine_state.config->textureScaleMode))
    {
        RC2D_log(RC2D_LOG_WARN, "Erreur : impossible d'initialiser le mode de mise à l'échelle des textures.\n");
    }

    /**
     * Activer le VSync pour le renderer.
     * Cela permet de synchroniser le rendu avec le taux de rafraîchissement du moniteur,
     * réduisant ainsi les déchirures d'écran (screen tearing).
     */
    if (!SDL_SetRenderVSync(rc2d_engine_state.renderer, 1))
    {
        RC2D_log(RC2D_LOG_WARN, "Erreur : impossible d'activer le VSync : %s\n", SDL_GetError());
    }

    /**
     * Configurer le mode de présentation logique (letterbox ou integer scale).
     * Cela permet de gérer comment le contenu est affiché dans la fenêtre,
     * en fonction des préférences de l'utilisateur.
     */
    if (rc2d_engine_state.config->logicalPresentationMode == RC2D_LOGICAL_PRESENTATION_LETTERBOX)
    {
        /**
         * Si le mode de présentation logique est en "letterbox",
         * on active le letterboxing dans le renderer.
         */
        if (!SDL_SetRenderLogicalPresentation(rc2d_engine_state.renderer,  
            rc2d_engine_state.config->logicalWidth, rc2d_engine_state.config->logicalHeight,
            SDL_LOGICAL_PRESENTATION_LETTERBOX))
        {
            RC2D_log(RC2D_LOG_WARN, "Erreur : impossible d'activer le letterboxing : %s\n", SDL_GetError());
        }
    }
    else if (rc2d_engine_state.config->logicalPresentationMode == RC2D_LOGICAL_PRESENTATION_INTEGER_SCALE)
    {
        /**
         * Si le mode de présentation logique est en "integer scale",
         * on active le integer scaling dans le renderer.
         */
        if (!SDL_SetRenderLogicalPresentation(rc2d_engine_state.renderer, 
            rc2d_engine_state.config->logicalWidth, rc2d_engine_state.config->logicalHeight, 
            SDL_LOGICAL_PRESENTATION_INTEGER_SCALE))
        {
            RC2D_log(RC2D_LOG_WARN, "Erreur : impossible d'activer le integer scale : %s\n", SDL_GetError());
        }
    }
    else if (rc2d_engine_state.config->logicalPresentationMode == RC2D_LOGICAL_PRESENTATION_OVERSCAN)
    {
        /**
         * Si le mode de présentation logique est en "overscan",
         * on active l'overscan dans le renderer.
         */
        if (!SDL_SetRenderLogicalPresentation(rc2d_engine_state.renderer, 
            rc2d_engine_state.config->logicalWidth, rc2d_engine_state.config->logicalHeight, 
            SDL_LOGICAL_PRESENTATION_OVERSCAN))
        {
            RC2D_log(RC2D_LOG_WARN, "Erreur : impossible d'activer l'overscan : %s\n", SDL_GetError());
        }
    }

    /**
     * Calcul initial du viewport GPU et de l'échelle de rendu pour l'ensemble de l'application.
     * Cela permet de s'assurer que le rendu est effectué à la bonne échelle et dans la bonne zone de la fenêtre.
     */
    rc2d_engine_presentationUpdate();

    /**
     * Recupere les donnees du moniteur qui contient la fenetre window pour regarder 
     * le nombre de HZ du moniteur et lui set les FPS.
     * 
	 * Si les hz n'ont pas etait trouve, FPS par default : 60.
     */
    rc2d_engine_update_fps_based_on_monitor();

    /**
     * Initialiser certains modules internes de RC2D
     */
	//rc2d_keyboard_init();
    rc2d_timer_init();

#if RC2D_ONNX_MODULE_ENABLED
    if (!rc2d_onnx_init())
    {
        return false;
    }
#endif

    // Log pour indiquer que tout le moteur a été initialisé avec succès
    RC2D_log(RC2D_LOG_INFO, "RC2D Engine initialized successfully.\n");

    // Retourne true pour indiquer que l'initialisation a réussi
	return true;
}

#if RC2D_NET_MODULE_ENABLED
bool rc2d_engine_start_worker_threads(void)
{
    // Si la config/callbacks est absente, rien a lancer.
    if (rc2d_engine_state.config == NULL || rc2d_engine_state.config->callbacks == NULL)
    {
        return true;
    }

    // Si deja en cours, la fonction est idempotente.
    if (rc2d_engine_workerThreadsShouldRun())
    {
        return true;
    }

    // Autoriser les boucles workers a tourner.
    SDL_SetAtomicInt(&rc2d_engine_state.worker_threads_should_run, 1);
    bool startedAtLeastOneThread = false;

    // Politique de connexion auto au demarrage du thread reseau.
    if (rc2d_engine_state.config->networkClientConfig != NULL &&
        rc2d_engine_state.config->networkClientConfig->autoConnectOnStart)
    {
        rc2d_engine_networkConnectToServer(
            rc2d_engine_state.config->networkClientConfig->serverAddress,
            rc2d_engine_state.config->networkClientConfig->serverPort);
    }

    // Lancer le thread simulation si callback presente.
    if (rc2d_engine_state.config->callbacks->rc2d_simulation_update != NULL)
    {
        rc2d_engine_state.simulation_thread = SDL_CreateThread(
            rc2d_engine_simulationThreadMain,
            "rc2d_simulation_thread",
            NULL);
        if (rc2d_engine_state.simulation_thread == NULL)
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to create simulation thread: %s", SDL_GetError());
            rc2d_engine_stop_worker_threads();
            return false;
        }
        startedAtLeastOneThread = true;
    }

    // Lancer le thread reseau si incoming/outgoing/setup host est utilise.
    if (rc2d_engine_state.config->callbacks->rc2d_network_incoming_update != NULL ||
        rc2d_engine_state.config->callbacks->rc2d_network_outgoing_update != NULL ||
        rc2d_engine_state.config->callbacks->rc2d_network_host_setup != NULL)
    {
        rc2d_engine_state.network_thread = SDL_CreateThread(
            rc2d_engine_networkThreadMain,
            "rc2d_network_thread",
            NULL);
        if (rc2d_engine_state.network_thread == NULL)
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to create network thread: %s", SDL_GetError());
            rc2d_engine_stop_worker_threads();
            return false;
        }
        startedAtLeastOneThread = true;
    }

    // Lancer le thread HTTP uniquement si callback presente.
    if (rc2d_engine_state.config->callbacks->rc2d_http_update != NULL)
    {
        rc2d_engine_state.http_thread = SDL_CreateThread(
            rc2d_engine_httpThreadMain,
            "rc2d_http_thread",
            NULL);
        if (rc2d_engine_state.http_thread == NULL)
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to create HTTP thread: %s", SDL_GetError());
            rc2d_engine_stop_worker_threads();
            return false;
        }
        startedAtLeastOneThread = true;
    }

    // Lancer le thread WebSocket uniquement si callback presente.
    if (rc2d_engine_state.config->callbacks->rc2d_websocket_update != NULL)
    {
        rc2d_engine_state.websocket_thread = SDL_CreateThread(
            rc2d_engine_websocketThreadMain,
            "rc2d_websocket_thread",
            NULL);
        if (rc2d_engine_state.websocket_thread == NULL)
        {
            RC2D_log(RC2D_LOG_ERROR, "Failed to create WebSocket thread: %s", SDL_GetError());
            rc2d_engine_stop_worker_threads();
            return false;
        }
        startedAtLeastOneThread = true;
    }

    // Si aucun worker n'a ete lance, remettre le flag a 0.
    if (!startedAtLeastOneThread)
    {
        SDL_SetAtomicInt(&rc2d_engine_state.worker_threads_should_run, 0);
    }

    return true;
}

void rc2d_engine_stop_worker_threads(void)
{
    // Demander l'arret de toutes les boucles workers.
    SDL_SetAtomicInt(&rc2d_engine_state.worker_threads_should_run, 0);
    SDL_SetAtomicInt(&rc2d_engine_state.network_connect_desired, 0);
    SDL_SetAtomicInt(&rc2d_engine_state.network_disconnect_requested, 1);

    // Donner une chance a l'application de reveiller les attentes bloquantes
    // (ex: queues waitAndPop dans les threads HTTP/WebSocket) avant les join.
    if (rc2d_engine_state.config != NULL &&
        rc2d_engine_state.config->callbacks != NULL &&
        rc2d_engine_state.config->callbacks->rc2d_wake_blocking_threads != NULL)
    {
        rc2d_engine_state.config->callbacks->rc2d_wake_blocking_threads();
    }

    // Attendre la fin du thread simulation.
    if (rc2d_engine_state.simulation_thread != NULL)
    {
        SDL_WaitThread(rc2d_engine_state.simulation_thread, NULL);
        rc2d_engine_state.simulation_thread = NULL;
    }

    // Attendre la fin du thread reseau.
    if (rc2d_engine_state.network_thread != NULL)
    {
        SDL_WaitThread(rc2d_engine_state.network_thread, NULL);
        rc2d_engine_state.network_thread = NULL;
    }

    // Attendre la fin du thread HTTP.
    if (rc2d_engine_state.http_thread != NULL)
    {
        SDL_WaitThread(rc2d_engine_state.http_thread, NULL);
        rc2d_engine_state.http_thread = NULL;
    }

    // Attendre la fin du thread WebSocket.
    if (rc2d_engine_state.websocket_thread != NULL)
    {
        SDL_WaitThread(rc2d_engine_state.websocket_thread, NULL);
        rc2d_engine_state.websocket_thread = NULL;
    }
}
#endif

bool rc2d_engine_init(void)
{
	// Init GameEngine house
	if (!rc2d_engine())
    {
		return false;
    }

	return true;
}

void rc2d_engine_quit(void)
{
#if RC2D_NET_MODULE_ENABLED
    // Demander l'arret des workers avant de liberer les ressources globales.
    rc2d_engine_stop_worker_threads();

    // Liberer le mutex de controle reseau runtime.
    if (rc2d_engine_state.network_control_mutex != NULL)
    {
        SDL_DestroyMutex(rc2d_engine_state.network_control_mutex);
        rc2d_engine_state.network_control_mutex = NULL;
    }
#endif

    // Attendre que le GPU soit inactif avant de libérer les ressources
    SDL_WaitForGPUIdle(rc2d_gpu_getDevice());

    /**
     * Détruire les ressources internes des modules de la lib RC2D.
     */
	rc2d_filesystem_quit();
    rc2d_storage_closeAll();
    rc2d_graphics_destroyRendererTextEngine();
    //rc2d_touch_freeTouchState();
#if RC2D_ONNX_MODULE_ENABLED
    rc2d_onnx_cleanup();
#endif

    // Lib OpenSSL Deinitialize
    rc2d_engine_cleanup_openssl();

    // Lib SDL3_ttf Deinitialize
    rc2d_engine_cleanup_sdlttf();

    // Lib SDL3_mixer Deinitialize
    rc2d_engine_cleanup_sdlmixer();

    // Lib SDL3_shadercross Deinitialize
    rc2d_engine_cleanup_sdlshadercross();

    // Lib RCENet Deinitialize
    rc2d_engine_cleanup_rcenet();

    // lib curl Deinitialize
    rc2d_engine_cleanup_curl();

#if RC2D_STEAMWORKS_SDK_ENABLED
    // Lib Steamworks Deinitialize
    rc2d_steamworks_cleanup();
#endif // RC2D_STEAMWORKS_SDK_ENABLED

#if RC2D_EOS_SDK_ENABLED
    // Lib EOS Deinitialize
    rc2d_eos_cleanup();
#endif // RC2D_EOS_SDK_ENABLED

    // Free command line arguments
    rc2d_cmdline_free();
    
    /* Libérer les shaders graphiques (vertex/fragment) */
    if (rc2d_engine_state.gpu_graphics_shader_mutex) 
    {
        SDL_LockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
        for (int i = 0; i < rc2d_engine_state.gpu_graphics_shader_count; i++)
        {
            if (rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states)
            {
                for (int j = 0; j < rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_state_count; ++j)
                {
                    RC2D_safe_free(rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states[j].sampler_bindings);
                    rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states[j].sampler_bindings = NULL;
                    rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states[j].state_handle = NULL;
                    rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states[j].num_sampler_bindings = 0;
                }

                RC2D_safe_free(rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states);
                rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_states = NULL;
                rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_state_count = 0;
                rc2d_engine_state.gpu_graphics_shaders_cache[i].gpu_render_state = NULL;
            }

            if (rc2d_engine_state.gpu_graphics_shaders_cache[i].filename) 
            {
                RC2D_safe_free(rc2d_engine_state.gpu_graphics_shaders_cache[i].filename);
                rc2d_engine_state.gpu_graphics_shaders_cache[i].filename = NULL;
            }
        }
        RC2D_safe_free(rc2d_engine_state.gpu_graphics_shaders_cache);
        rc2d_engine_state.gpu_graphics_shaders_cache = NULL;
        rc2d_engine_state.gpu_graphics_shader_count = 0;
        SDL_UnlockMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
        SDL_DestroyMutex(rc2d_engine_state.gpu_graphics_shader_mutex);
        rc2d_engine_state.gpu_graphics_shader_mutex = NULL;
    }

    // Forcer le contexte de rendu à vider toutes les commandes et l'état en attente.
    if (rc2d_engine_state.renderer)
    {
        SDL_FlushRenderer(rc2d_engine_state.renderer);
    }

    // Libérer le renderer
    if (rc2d_engine_state.renderer)
    {
        SDL_DestroyRenderer(rc2d_engine_state.renderer);
    }

    /* Détruire la fenêtre */
    if (rc2d_engine_state.window) 
    {
        SDL_DestroyWindow(rc2d_engine_state.window);
        rc2d_engine_state.window = NULL;
    }

    // Cleanup SDL3
	rc2d_engine_cleanup_sdl();

    /**
     * Affiche un rapport des fuites mémoire détectées.
     * Cela est utile pour identifier les fuites de mémoire dans l'application.
     * 
     * Note : 
     * - Ce rapport est affiché uniquement si RC2D_MEMORY_DEBUG_ENABLED est défini à 1.
     * - Il est recommandé de l'utiliser uniquement en mode développement pour éviter les ralentissements en production.
     */
    rc2d_memory_report();
}

void rc2d_engine_configure(const RC2D_EngineConfig* config)
{
    /**
     * Cela permet de s'assurer que l'état du moteur est dans un 
     * état valide avant de le configurer.
     */
    rc2d_engine_stateInit();

    /**
     * Vérifie si l'état du moteur RC2D est valide.
     * Si l'état du moteur est NULL, alors on ne peut pas le configurer.
     */
    if (rc2d_engine_state.config == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Engine state config is NULL. Cannot configure.\n");
        return;
    }

    /**
     * Vérifie si le pointeur de configuration du framework RC2D est valide.
     * 
     * Si le pointeur est NULL, alors on utilisera toutes les valeurs 
     * par défaut pour la configuration.
     */
    if (config == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "No RC2D_Config provided. Using default values.\n");
        return;
    }

    /**
     * Vérifie si la structure concernant les informations de l'application est valide.
     * 
     * Si les informations de l'application sont NULL, on peut continuer,
     * puisque les valeurs par défaut seront utilisées.
     */
    if (config->appInfo != NULL)
    {
        rc2d_engine_state.config->appInfo = config->appInfo;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "No RC2D_AppInfo provided. Using default values.\n");
    }

    /**
     * Vérifie si la structure concernant les callbacks est valide.
     * 
     * On peut aussi choisir de ne pas les utiliser, mais dans ce cas,
     * on ne pourrait pas utiliser les callbacks de la librairie RC2D, 
     * comme dessiner, charger, etc.
     * 
     * Donc cela serait simplement une fenetre SDL3 noir sans rien d'autre.
     */
    if (config->callbacks != NULL)
    {
        rc2d_engine_state.config->callbacks = config->callbacks;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "No RC2D_Callbacks provided. Some events may not be handled.\n");
    }

#if RC2D_NET_MODULE_ENABLED
    /**
     * Verifie si la structure concernant la configuration reseau client ENet est valide.
     *
     * Si la configuration est NULL, on conserve la configuration reseau par defaut.
     */
    if (config->networkClientConfig != NULL)
    {
        rc2d_engine_state.config->networkClientConfig = config->networkClientConfig;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "No RC2D_NetworkClientConfig provided. Using default values.\n");
    }
#endif

    /**
     * Vérifie si la propriété concernant l'enumération du nombre d'images en vol pour le GPU est valide.
     * 
     * Sinon on utilise la valeur par défaut de 2 images en vol (RC2D_GPU_FRAMES_BALANCED).
     */
    if (config->gpuFramesInFlight == RC2D_GPU_FRAMES_LOW_LATENCY ||
        config->gpuFramesInFlight == RC2D_GPU_FRAMES_BALANCED ||
        config->gpuFramesInFlight == RC2D_GPU_FRAMES_HIGH_THROUGHPUT)
    {
        rc2d_engine_state.config->gpuFramesInFlight = config->gpuFramesInFlight;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid RC2D_GPUFramesInFlight value. Using default.\n");
        rc2d_engine_state.config->gpuFramesInFlight = RC2D_GPU_FRAMES_BALANCED;
    }

    /**
     * Vérifie si la propriété concernant la configuration avancée du GPU est valide.
     * 
     * Si la configuration avancée est valide, on l'utilise, sinon on utilise les valeurs par défaut.
     */
    if (config->gpuOptions != NULL)
    {
        rc2d_engine_state.config->gpuOptions = config->gpuOptions;

// Plateformes qui ne permettent pas le mode GPU débogage
#if defined(RC2D_PLATFORM_ANDROID) || defined(RC2D_PLATFORM_IOS)   || \
    defined(RC2D_PLATFORM_TVOS)    || defined(RC2D_PLATFORM_VISIONOS) || \
    defined(RC2D_PLATFORM_XBOXONE) || defined(RC2D_PLATFORM_XBOXSERIES)
    RC2D_log(RC2D_LOG_WARN, "Debug GPU mode forcé OFF sur cette plateforme, car non supportée.\n");
    rc2d_engine_state.config->gpuOptions->debugMode = false;
    rc2d_engine_state.config->gpuOptions->verbose = false;
#endif // Plateformes qui ne permettent pas le mode GPU débogage
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "No RC2D_GPUAdvancedOptions provided. Using default GPU settings.\n");
    }

    /**
     * Vérifie si la propriété concernant la taille de la fenêtre en largeur de l'application est valide.
     * 
     * Si la taille de la fenêtre est valide (> 0), on l'utilise, sinon on utilise les valeurs par défaut.
     */
    if (config->windowWidth > 0)
    {
        rc2d_engine_state.config->windowWidth = config->windowWidth;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid window size width provided. Using default values.\n");
    }

    /**
     * Vérifie si la propriété concernant la taille de la fenêtre en hauteur de l'application est valide.
     * 
     * Si la taille de la fenêtre est valide (> 0), on l'utilise, sinon on utilise les valeurs par défaut.
     */
    if (config->windowHeight > 0)
    {
        rc2d_engine_state.config->windowHeight = config->windowHeight;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid window size height provided. Using default values.\n");
    }

    /**
     * Vérifie si la propriété concernant la taille logique en largeur de l'application est valide.
     * 
     * Si la taille logique est valide (> 0), on l'utilise, sinon on utilise les valeurs par défaut.
     */
    if (config->logicalWidth > 0)
    {
        rc2d_engine_state.config->logicalWidth = config->logicalWidth;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid logical size width provided. Using default values.\n");
    }

    /**
     * Vérifie si la propriété concernant la taille logique en hauteur de l'application est valide.
     * 
     * Si la taille logique est valide (> 0), on l'utilise, sinon on utilise les valeurs par défaut.
     */
    if (config->logicalHeight > 0)
    {
        rc2d_engine_state.config->logicalHeight = config->logicalHeight;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid logical size height provided. Using default values.\n");
    }
    
    /**
     * Vérifie si la propriété concernant le mode de présentation est valide.
     * 
     * Si le mode de présentation est valide, on l'utilise, sinon on utilise les valeurs par défaut.
     */
    if (config->logicalPresentationMode == RC2D_LOGICAL_PRESENTATION_INTEGER_SCALE ||
        config->logicalPresentationMode == RC2D_LOGICAL_PRESENTATION_LETTERBOX ||
        config->logicalPresentationMode == RC2D_LOGICAL_PRESENTATION_OVERSCAN)
    {
        rc2d_engine_state.config->logicalPresentationMode = config->logicalPresentationMode;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid presentation mode provided. Using default values.\n");
        rc2d_engine_state.config->logicalPresentationMode = RC2D_LOGICAL_PRESENTATION_LETTERBOX;
    }

    if (rc2d_engine_is_valid_texture_scale_mode(config->textureScaleMode))
    {
        rc2d_engine_state.config->textureScaleMode = config->textureScaleMode;
    }
    else
    {
        RC2D_log(RC2D_LOG_WARN, "Invalid texture scale mode provided. Using default values.\n");
        rc2d_engine_state.config->textureScaleMode = RC2D_TEXTURE_SCALE_LINEAR;
    }
}

