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

    // Log l'AppID Steam pour vérification
    uint32 appID = SteamUtils()->GetAppID();
    RC2D_log(RC2D_LOG_INFO, "SDK Steamworks initialized for AppID=%u", appID);

    // Success
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
    if (!api_name)
        return false;

    if (!SteamUserStats())
    {
        RC2D_log(RC2D_LOG_ERROR, "SteamUserStats NULL (SteamAPI_Init ok?)");
        return false;
    }

    bool alreadyUnlocked = false;
    if (SteamUserStats()->GetAchievement(api_name, &alreadyUnlocked) && alreadyUnlocked)
    {
        RC2D_log(RC2D_LOG_INFO, "Achievement %s already unlocked", api_name);
        return true;
    }

    bool okSet   = SteamUserStats()->SetAchievement(api_name);
    bool okStore = SteamUserStats()->StoreStats();

    RC2D_log(RC2D_LOG_INFO,
             "Unlock achievement %s → Set=%s Store=%s",
             api_name,
             okSet ? "OK" : "FAIL",
             okStore ? "OK" : "FAIL");

    return okSet && okStore;
}

} // extern "C"
