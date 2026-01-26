#if RC2D_EOS_SDK_ENABLED

#include <RC2D/RC2D_eos.h>
#include <RC2D/RC2D_logger.h>

#include <eos_types.h>  // Pour : EOS_HPlatform
#include <eos_sdk.h>    // Pour : EOS_Platform_Tick..etc
#include <eos_result.h> // Pour : EOS_EResult codes (EOS_Success, EOS_NotConfigured, ...)

// Indique si le SDK EOS a été initialisé avec succès
static bool eos_initialized = false;
// Stocke la plateforme EOS créée lors de l'initialisation
static EOS_HPlatform eos_platform = NULL;

EOS_HPlatform rc2d_eos_getPlatform(void)
{
    return eos_platform;
}

bool rc2d_eos_init(void)
{

}

void rc2d_eos_tick(void)
{
    if (!eos_initialized || eos_platform == NULL)
        return;

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
    EOS_EResult r = EOS_Shutdown();
    if (r == EOS_Success)
    {
        RC2D_log(RC2D_LOG_INFO, "EOS_Shutdown: success.");
        eos_initialized = false;
        return;
    }

    // 4) Gestion des erreurs connues
    switch (r)
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
            RC2D_log(RC2D_LOG_CRITICAL, "EOS_Shutdown failed with EOS_EResult=%d.", (int)r);
            // On ne sait pas si l’état est propre, donc on marque non-init pour éviter double call.
            eos_initialized = false;
            break;
    }
}


bool rc2d_eos_unlockAchievement(const char* api_name)
{

}

#endif // RC2D_EOS_SDK_ENABLED