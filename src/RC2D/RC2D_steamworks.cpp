#if RC2D_STEAMWORKS_SDK_ENABLED

#include <RC2D/RC2D_steamworks.h>
#include <RC2D/RC2D_logger.h>

#include <steam_api.h>

extern "C" {

bool rc2d_steam_init(void)
{
    // Initialisation du SDK Steamworks
    if (!SteamAPI_Init())
    {
        RC2D_log(RC2D_LOG_CRITICAL, "Failed to initialize SDK Steamworks");
        return false;
    }

    // Success
    RC2D_log(RC2D_LOG_INFO, "Steamworks SDK initialized successfully");
    return true;
}

void rc2d_steam_run_callbacks(void)
{
    SteamAPI_RunCallbacks();
}

void rc2d_steam_cleanup(void)
{
    SteamAPI_Shutdown();
}

bool rc2d_steam_unlock_achievement(const char* api_name)
{
    // Check si le nom a été fourni
    if (!api_name)
        return false;

    // Vérifie que SteamUserStats est disponible
    if (!SteamUserStats())
    {
        RC2D_log(RC2D_LOG_ERROR, "SteamUserStats NULL (SteamAPI_Init ok?)");
        return false;
    }

    // Vérifie si le succès est déjà débloqué
    bool alreadyUnlocked = false;
    if (SteamUserStats()->GetAchievement(api_name, &alreadyUnlocked) && alreadyUnlocked)
    {
        RC2D_log(RC2D_LOG_INFO, "Achievement %s already unlocked", api_name);
        return true;
    }

    // Débloque le succès
    bool okSet   = SteamUserStats()->SetAchievement(api_name);
    bool okStore = SteamUserStats()->StoreStats();

    return okSet && okStore;
}

} // extern "C"

#endif // RC2D_STEAMWORKS_SDK_ENABLED