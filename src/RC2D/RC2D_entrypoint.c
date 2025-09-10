/**
 * IMPORTANT:
 * - Doit être inclus avant tout autre fichier d'en-tête SDL / RC2D.
 * 
 * - SDL_MAIN_USE_CALLBACKS et <SDL3/SDL_main.h> doivent être inclus UNE SEULE FOIS
 * dans un fichier source (.c/.cpp).
 * 
 * - A partir du moment ou SDL_MAIN_USE_CALLBACKS est défini et <SDL3/SDL_main.h> est inclus,
 * la fonction main() ne doit pas être définie dans le code de l'application.
 */
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>

#include <RC2D/RC2D_engine.h>
#include <RC2D/RC2D_internal.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>
#include <RC2D/RC2D_graphics.h>

static bool title_storage_is_ready = false;  // SDL_OpenTitleStorage est prêt ?
static bool user_storage_is_ready  = false;  // SDL_OpenUserStorage est prêt ?
static bool rc2d_load_has_been_called = false;  // rc2d_load() a déjà été appelé ?

/**
 * SDL3 Callback: Initialisation
 * 
 * Cette fonction est appelée une seule fois au démarrage de l'application.
 */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) 
{
    /**
     * La définition de la fonction rc2d_setup doit être définie par l'utilisateur.
     * On passe les arguments de la ligne de commande à la fonction rc2d_setup.
     */
    const RC2D_EngineConfig* config = rc2d_engine_setup(argc, argv);

    /**
     * Si la configuration est NULL (const RC2D_EngineConfig* config), 
     * alors on utilisera la configuration par défaut de RC2D.
     */
    rc2d_engine_configure(config);

    /**
     * Initialise le moteur RC2D.
     * 
     * Si l'initialisation échoue, on retourne SDL_APP_FAILURE.
     */
	if(!rc2d_engine_init())
    {
        /**
         * SDL_APP_FAILURE : Cela vas appeler SDL_AppQuit et terminer 
         * le processus avec un code de sortie signalant une erreur 
         * à la plateforme.
         */
        return SDL_APP_FAILURE;
    }

    /**
     * Ouvre le dossier de stockage pour l'application et l'utilisateur.
     *
     * - Le storage "Title" est en lecture seule, pour les assets packagés.
     * - Le storage "User" est en lecture/écriture, pour les données utilisateur.
     *
     * Pour rc2d_storage_openTitle : Si 'override_path' est NULL, cela utilise SDL_GetBasePath() comme racine.
     *
     * Il n'ai pas garanti que les dossiers de stockage soient prêts immédiatement après l'ouverture,
     * donc on doit vérifier leur état dans la boucle principale avant de les utiliser.
    */
    rc2d_storage_openTitle(NULL);
    rc2d_storage_openUser(config->appInfo->organization, config->appInfo->name);

    /**
     * SDL_APP_CONTINUE : Cela vas appeler la fonction SDL_AppIterate 
     * à intervalle régulier, et la boucle principale de l'application commence.
     * 
     * SDL_AppInit est appelée une seule fois au démarrage de l'application.
     */
    return SDL_APP_CONTINUE;
}

/**
 * SDL3 Callback: Boucle principale de l'application
 * 
 * Cette fonction s'exécute une fois par image dans la boucle principale de l'application,
 * à interval régulier via SDL_HINT_MAIN_CALLBACK_RATE, si la plateforme le supporte.
 * 
 * Sinon, si ce n'ai pas le cas, on limite la fréquence d'appel de cette fonction
 * à 60 FPS (16.67 ms par image) pour éviter de surcharger le CPU.
 */
SDL_AppResult SDL_AppIterate(void *appstate) 
{
    /**
     * 0)
     *
     * Si le jeu n'est plus en cours d'exécution, on sort de la boucle principale.
     * 
     * Si elle renvoie SDL_APP_SUCCESS, l'application appelle SDL_AppQuit et 
     * termine avec un code de sortie signalant la réussite à la plateforme.
     */
    if (!rc2d_engine_state.game_is_running) 
    {
        return SDL_APP_SUCCESS;
    }

    /**
     * 1)
     *
     * Vérifie si les dossiers de stockage sont prêts.
     * 
     * Cela est nécessaire car l'ouverture des dossiers de stockage peut être asynchrone
     * sur certaines plateformes (ex: consoles de jeux).
     * 
     * Une fois que les dossiers de stockage sont prêts, on peut commencer à lire/écrire des fichiers.
     */
    const bool was_title_ready = title_storage_is_ready;
    const bool was_user_ready  = user_storage_is_ready;

    // --- STORAGE TITLE ---
    if (rc2d_storage_titleReady()) 
    {
        // Le dossier de stockage Title est prêt
        title_storage_is_ready = true;

        // Log seulement la transition de NOT READY -> READY
        if (!was_title_ready) 
        {
            RC2D_log(RC2D_LOG_INFO, "[Storage] Title storage is ready.");
        }
    }
    else 
    {
        // Le dossier de stockage Title n'est pas prêt
        title_storage_is_ready = false;

        // Log seulement la transition de READY -> NOT READY
        if (was_title_ready) 
        {
            RC2D_log(RC2D_LOG_WARN, "[Storage] Title storage became NOT READY. Re-opening...");
        }

        // Réessaye d'ouvrir le dossier de stockage Title
        rc2d_storage_openTitle(NULL);
    }
    // --- STORAGE USER ---
    if (rc2d_storage_userReady()) 
    {
        // Le dossier de stockage User est prêt
        user_storage_is_ready = true;

        // Log seulement la transition de NOT READY -> READY
        if (!was_user_ready) 
        {
            RC2D_log(RC2D_LOG_INFO, "[Storage] User storage is ready.");
        }
    }
    else 
    {
        // Le dossier de stockage User n'est pas prêt
        user_storage_is_ready = false;

        // Log seulement la transition de READY -> NOT READY
        if (was_user_ready) 
        {
            RC2D_log(RC2D_LOG_WARN, "[Storage] User storage became NOT READY. Re-opening...");
        }

        // Réessaye d'ouvrir le dossier de stockage User
        rc2d_storage_openUser(
            rc2d_engine_state.config->appInfo->organization,
            rc2d_engine_state.config->appInfo->name
        );
    }

    /**
     * 2)
     * 
     * Appelle rc2d_load() une seule fois, une fois que les dossiers de stockage sont prêts.
     * 
     * Cela permet de s'assurer que toutes les ressources nécessaires sont disponibles
     * avant de commencer la boucle principale du jeu.
     * 
     * On affiche la fenêtre principale seulement après que rc2d_load() a été appelé,
     * pour éviter d'afficher une fenêtre vide ou non initialisée.
     */
    if (!rc2d_load_has_been_called) 
    {
        // On vérifie si les deux dossiers de stockage sont prêts
        if (!(title_storage_is_ready && user_storage_is_ready)) 
        {
            /**
             * Si les dossiers de stockage ne sont pas prêts, on attend.
             * On ne peut pas continuer tant que les dossiers de stockage ne sont pas prêts.
             * 
             * On retourne SDL_APP_CONTINUE pour continuer la boucle principale.
            */
            return SDL_APP_CONTINUE;
        }

        /**
         * Appelle la fonction de chargement de l'application.
         * Cela doit être fait une seule fois, une fois que les dossiers de stockage sont prêts.
         * Cela permet de s'assurer que toutes les ressources nécessaires sont disponibles
         * avant de commencer la boucle principale du jeu.
        */
        if (rc2d_engine_state.config &&
            rc2d_engine_state.config->callbacks &&
            rc2d_engine_state.config->callbacks->rc2d_load) 
        {
            rc2d_engine_state.config->callbacks->rc2d_load();
        } 
        else 
        {
            RC2D_log(RC2D_LOG_WARN, "No rc2d_load() function defined, skipping load step.");
        }

        // Indique que rc2d_load() a été appelé pour ne pas le rappeler
        rc2d_load_has_been_called = true;

        /**
        * 1. Affiche enfin la fenêtre après tout l'init (GPU, logique, textures…)
        * 
        * 2. Demandez que la fenêtre soit surélevée au-dessus des autres fenêtres 
        *    et obtenez le focus d'entrée.
        */
        SDL_ShowWindow(rc2d_engine_state.window);
        SDL_RaiseWindow(rc2d_engine_state.window);

        /**
        * Pour rc2d_last_frame_time, nous ne voulons pas que le deltatime de la 
        * premiere image inclue le temps pris par l'init de l'engine, rc2d_load..etc.
        */
        rc2d_engine_state.last_frame_time = SDL_GetPerformanceCounter();
    }

    /**
     * 3)
     * 
     * Ordre de la boucle principale de l'application :
     * 1. Calculer le delta time pour la frame actuelle.
     * 2. Appeler les fonctions internes de hot reload des shaders.
     * 3. Appeler la fonction de mise à jour du jeu.
     * 4. Appeler la fonction de dessin du jeu.
     * 5. Présenter le rendu à l'écran.
     * 6. Terminer le calcul du delta time pour la frame actuelle.
     */
    rc2d_engine_deltatime_start();
    #if RC2D_GPU_SHADER_HOT_RELOAD_ENABLED
    rc2d_gpu_hotReloadGraphicsShaders();
    #endif
    if (rc2d_engine_state.config != NULL && 
        rc2d_engine_state.config->callbacks != NULL && 
        rc2d_engine_state.config->callbacks->rc2d_update != NULL) 
    {
        rc2d_engine_state.config->callbacks->rc2d_update(rc2d_engine_state.delta_time);
    }
    rc2d_graphics_clear();
    if (rc2d_engine_state.config != NULL && 
        rc2d_engine_state.config->callbacks != NULL && 
        rc2d_engine_state.config->callbacks->rc2d_draw != NULL) 
    {
        rc2d_engine_state.config->callbacks->rc2d_draw();
    }
    rc2d_graphics_present();
    rc2d_engine_deltatime_end();

    /**
     * SDL_APP_CONTINUE : La boucle principale de l'application continue.
     */
    return SDL_APP_CONTINUE;
}

/**
 * SDL3 Callback: Gestion des événements
 * 
 * Cette fonction est appelée lorsqu'un nouvel événement (entrée de souris, clavier, etc.) se produit.
 */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) 
{
    return rc2d_engine_processevent(event);
}

/**
 * SDL3 Callback: Nettoyage
 * 
 * Cette fonction est appelée lorsque l'application se termine.
 */
void SDL_AppQuit(void *appstate, SDL_AppResult result) 
{
    /**
     * La boucle de jeu est terminée, appelez la fonction de déchargement si elle est définie
     * Cela peut être utilisé pour libérer des ressources ou effectuer d'autres tâches de nettoyage 
     * avant la fermeture du programme.
     */
    if (rc2d_engine_state.config != NULL && 
        rc2d_engine_state.config->callbacks != NULL && 
        rc2d_engine_state.config->callbacks->rc2d_unload != NULL)
    {
        rc2d_engine_state.config->callbacks->rc2d_unload();
    }

    /**
     * Si le résultat est SDL_APP_FAILURE, cela signifie que l'application a échoué 
     * et doit être terminée avec un code d'erreur.
     */
    if (result == SDL_APP_FAILURE) 
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Application failed: %s", SDL_GetError());
    }

    /**
     * Libére les ressources notamment celles des librairies externes de RC2D 
     * et des modules interne à RC2D.
     */
    rc2d_engine_quit();
}