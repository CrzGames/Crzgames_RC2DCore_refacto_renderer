#if RC2D_EOS_SDK_ENABLED

#include <RC2D/RC2D_eos.h>
#include <RC2D/RC2D_logger.h>
#include <RC2D/RC2D_memory.h>

#include <eos_sdk.h>
#include <eos_achievements.h>
#include <eos_achievements_types.h>

/**
 * Définitions par défaut des constantes EOS (peuvent être redéfinies dans le build system) pour le SDK EOS.
*/
#ifndef RC2D_EOS_PRODUCT_NAME
#define RC2D_EOS_PRODUCT_NAME    "SeaTyrants"
#endif
#ifndef RC2D_EOS_PRODUCT_VERSION
#define RC2D_EOS_PRODUCT_VERSION "1.0.0"
#endif
#ifndef RC2D_EOS_PRODUCT_ID
#define RC2D_EOS_PRODUCT_ID      "483a07a4954a482397d73330e91c243c"
#endif
#ifndef RC2D_EOS_SANDBOX_ID
#define RC2D_EOS_SANDBOX_ID      "deb286d018444cfeaf319de2397cd20a"
#endif
#ifndef RC2D_EOS_DEPLOYMENT_ID
#define RC2D_EOS_DEPLOYMENT_ID   "f07271687ac44329868f875060d2b5b1"
#endif
#ifndef RC2D_EOS_CLIENT_ID
#define RC2D_EOS_CLIENT_ID       "xyza7891rhXkFfG7uN4P1Jq8FVuB8BSc"
#endif
#ifndef RC2D_EOS_CLIENT_SECRET
#define RC2D_EOS_CLIENT_SECRET   "OiddLGdCVb8smJJdgUnzIaQnBV5dJwPJMwcm5g12SpU"
#endif

// Indique si le SDK EOS a été initialisé avec succès
static bool eos_initialized = false;
// Stocke la plateforme EOS créée lors de l'initialisation du SDK
static EOS_HPlatform eos_platform = NULL;


static EOS_ProductUserId eos_local_user = NULL; // à remplir après login Connect
static EOS_HAuth         eos_auth = NULL;
static EOS_HConnect      eos_connect = NULL;
static EOS_HAchievements eos_achievements = NULL;

/* ClientData qu'on va libérer dans le callback (ici: juste l'id) */
typedef struct RC2D_EOS_UnlockCtx {
    char* achievement_id;
} RC2D_EOS_UnlockCtx;

// Callback appelé quand l'unlock d'un achievement est terminé (pour libérer le ClientData)
static void EOS_CALL rc2d_eos_on_unlock_complete_freectx(
    const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data)
{
    if (data && data->ClientData)
    {
        RC2D_EOS_UnlockCtx* ctx = (RC2D_EOS_UnlockCtx*)data->ClientData;

        if (data->ResultCode == EOS_Success)
            RC2D_log(RC2D_LOG_INFO, "EOS: Achievement '%s' unlocked.", ctx->achievement_id);
        else
            RC2D_log(RC2D_LOG_WARN, "EOS: Unlock '%s' failed (Result=%d).",
                     ctx->achievement_id, (int)data->ResultCode);

        free(ctx->achievement_id);
        free(ctx);
        return;
    }

    // fallback log si pas de ctx
    if (data)
        RC2D_log(RC2D_LOG_WARN, "EOS: Unlock failed (Result=%d).", (int)data->ResultCode);
}

EOS_HPlatform rc2d_eos_getPlatform(void)
{
    return eos_platform;
}

bool rc2d_eos_init(void)
{
    if (eos_initialized)
    {
        RC2D_log(RC2D_LOG_WARN, "rc2d_eos_init: EOS already initialized. Skipping.");
        return true;
    }

    // -------- EOS_Initialize --------
    EOS_InitializeOptions initOpt;
    SDL_memset(&initOpt, 0, sizeof(initOpt));
    initOpt.ApiVersion = EOS_INITIALIZE_API_LATEST;

    // allocators custom = NULL (OK)
    initOpt.AllocateMemoryFunction = NULL;
    initOpt.ReallocateMemoryFunction = NULL;
    initOpt.ReleaseMemoryFunction = NULL;

    initOpt.ProductName = RC2D_EOS_PRODUCT_NAME;
    initOpt.ProductVersion = RC2D_EOS_PRODUCT_VERSION;
    initOpt.Reserved = NULL;
    initOpt.SystemInitializeOptions = NULL;
    initOpt.OverrideThreadAffinity = NULL;

    EOS_EResult r = EOS_Initialize(&initOpt);
    if (r == EOS_Success)
    {
        RC2D_log(RC2D_LOG_INFO, "EOS_Initialize: success (%s %s).",
                 RC2D_EOS_PRODUCT_NAME, RC2D_EOS_PRODUCT_VERSION);
    }
    else if (r == EOS_AlreadyConfigured)
    {
        // Le SDK est déjà initialisé (par un autre code). On continue quand même.
        RC2D_log(RC2D_LOG_WARN, "EOS_Initialize: EOS_AlreadyConfigured (SDK already initialized).");
    }
    else if (r == EOS_InvalidParameters)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "EOS_Initialize failed: EOS_InvalidParameters.");
        return false;
    }
    else
    {
        RC2D_log(RC2D_LOG_CRITICAL, "EOS_Initialize failed with EOS_EResult=%d.", (int)r);
        return false;
    }

    // -------- EOS_Platform_Create --------
    EOS_Platform_Options platOpt;
    SDL_memset(&platOpt, 0, sizeof(platOpt));
    platOpt.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    platOpt.Reserved = NULL; // Doit être NULL obligatoirement selon doc

    platOpt.ProductId = RC2D_EOS_PRODUCT_ID;
    platOpt.SandboxId = RC2D_EOS_SANDBOX_ID;
    platOpt.DeploymentId = RC2D_EOS_DEPLOYMENT_ID;
    platOpt.ClientCredentials.ClientId = RC2D_EOS_CLIENT_ID;
    platOpt.ClientCredentials.ClientSecret = RC2D_EOS_CLIENT_SECRET;

    platOpt.bIsServer = EOS_FALSE;
    platOpt.EncryptionKey = NULL;
    platOpt.OverrideCountryCode = NULL;
    platOpt.OverrideLocaleCode = NULL;
    platOpt.Flags = 0;
    platOpt.CacheDirectory = NULL;
    platOpt.TickBudgetInMilliseconds = 0; // 0 =  La valeur zéro signifie « exécuter toutes les tâches disponibles ».
    platOpt.RTCOptions = NULL;
    platOpt.IntegratedPlatformOptionsContainerHandle = NULL;
    platOpt.SystemSpecificOptions = NULL;
    platOpt.TaskNetworkTimeoutSeconds = NULL;

    eos_platform = EOS_Platform_Create(&platOpt);
    if (eos_platform == NULL)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "EOS_Platform_Create failed: returned NULL.");

        // rollback best-effort
        EOS_EResult sd = EOS_Shutdown();
        RC2D_log(RC2D_LOG_WARN, "EOS_Shutdown after platform create failure returned EOS_EResult=%d.", (int)sd);

        eos_initialized = false;
        eos_local_user = NULL;
        return false;
    }

    eos_initialized = true;
    eos_local_user = NULL;

    eos_auth = EOS_Platform_GetAuthInterface(eos_platform);
    eos_connect = EOS_Platform_GetConnectInterface(eos_platform);
    eos_achievements = EOS_Platform_GetAchievementsInterface(eos_platform);


    RC2D_log(RC2D_LOG_INFO, "EOS_Platform_Create: success.");
    return true;
}

void rc2d_eos_platformTick(void)
{
    if (!eos_initialized || eos_platform == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "SDK EOS not initialized or eos platform is NULL. Skipping tick.");
        return;
    }

    // Appel régulier pour traiter les tâches EOS en arrière-plan
    EOS_Platform_Tick(eos_platform);
}

void rc2d_eos_cleanup(void)
{
    // 1) Si jamais EOS n'a pas été initialisé, on ne fait rien.
    if (!eos_initialized)
    {
        RC2D_log(RC2D_LOG_DEBUG, "EOS cleanup skipped: EOS was not initialized.");
        return;
    }

    // 2) Libérer la platform avant EOS_Shutdown (recommandé)
    if (eos_platform != NULL)
    {
        EOS_Platform_Release(eos_platform);
        eos_platform = NULL;
        RC2D_log(RC2D_LOG_INFO, "EOS Platform released.");
    }

    // 3) Shutdown global SDK
    EOS_EResult result = EOS_Shutdown();
    if (result == EOS_Success)
    {
        RC2D_log(RC2D_LOG_INFO, "EOS_Shutdown: success.");
        eos_initialized = false;
        return;
    }

    // 4) Gestion des erreurs connues
    switch (result)
    {
        case EOS_NotConfigured:
            // Typiquement: EOS_Initialize n'a jamais été appelé (ou a échoué)
            RC2D_log(RC2D_LOG_WARN, "EOS_Shutdown: EOS_NotConfigured (EOS_Initialize was not successfully called).");
            // On considère quand même le SDK comme non-initialisé côté RC2D
            eos_initialized = false;
            break;

        case EOS_UnexpectedError:
            // Typiquement: EOS_Shutdown déjà appelé
            RC2D_log(RC2D_LOG_WARN, "EOS_Shutdown: EOS_UnexpectedError (EOS_Shutdown may have already been called).");
            eos_initialized = false;
            break;

        default:
            // Autre code: on log en critique
            RC2D_log(RC2D_LOG_CRITICAL, "EOS_Shutdown failed with EOS_EResult=%d.", (int)result);
            // On ne sait pas si l’état est propre, donc on marque non-init pour éviter double call.
            eos_initialized = false;
            break;
    }
}

bool rc2d_eos_unlockAchievement(const char* api_name)
{
    if (!eos_initialized || eos_platform == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "EOS: unlock skipped (SDK not initialized).");
        return false;
    }
    if (eos_local_user == NULL)
    {
        RC2D_log(RC2D_LOG_WARN, "EOS: unlock skipped (no local user logged in yet).");
        return false;
    }
    if (!eos_achievements)
    {
        RC2D_log(RC2D_LOG_CRITICAL, "EOS: Achievements interface is NULL.");
        return false;
    }
    if (api_name == NULL || api_name[0] == '\0')
    {
        RC2D_log(RC2D_LOG_WARN, "EOS: unlock skipped (invalid api_name).");
        return false;
    }

    RC2D_EOS_UnlockCtx* ctx = (RC2D_EOS_UnlockCtx*)RC2D_malloc(sizeof(*ctx));
    if (!ctx) return false;

    ctx->achievement_id = RC2D_strdup(api_name);
    if (!ctx->achievement_id)
    {
        RC2D_safe_free(ctx);
        return false;
    }

    // IMPORTANT: AchievementIds doit rester valide pendant l'async.
    // Ici on utilise un tableau qui pointe vers ctx->achievement_id (heap), OK.
    const char* ids[1];
    ids[0] = ctx->achievement_id;

    EOS_Achievements_UnlockAchievementsOptions opt;
    SDL_memset(&opt, 0, sizeof(opt));
    opt.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
    opt.UserId = eos_local_user;
    opt.AchievementIds = ids;
    opt.AchievementsCount = 1;

    EOS_Achievements_UnlockAchievements(
        eos_achievements,
        &opt,
        ctx,
        rc2d_eos_on_unlock_complete_freectx
    );

    return true; // requête envoyée
}

#endif // RC2D_EOS_SDK_ENABLED