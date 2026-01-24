/**
 * RC2D Steamworks Integration (Achievements)
 * - 1 achievement: ACH_TEST_1
 * - Requires Steamworks SDK (steam_api.h)
 *
 * IMPORTANT:
 * - Call rc2d_steam_init() after Steam client is running
 * - Call rc2d_steam_run_callbacks() every frame
 * - Wait until rc2d_steam_achievements_ready() == true before unlocking
 */

#include <RC2D/RC2D_steamworks.h>
#include <RC2D/RC2D_logger.h>

#include <steam_api.h>
#include <cstdint>

namespace
{
    // ---- Your single achievement ----
    static constexpr const char* kAchTest1 = "ACH_TEST_1";

    class RC2D_SteamAchievements
    {
    public:
        RC2D_SteamAchievements()
            : m_appId(0),
              m_initialized(false),
              m_cbUserStatsReceived(this, &RC2D_SteamAchievements::OnUserStatsReceived),
              m_cbUserStatsStored(this, &RC2D_SteamAchievements::OnUserStatsStored),
              m_cbAchievementStored(this, &RC2D_SteamAchievements::OnAchievementStored)
        {
            m_appId = SteamUtils()->GetAppID();
        }

        bool RequestStats()
        {
            if (!SteamUserStats() || !SteamUser())
                return false;

            if (!SteamUser()->BLoggedOn())
                return false;

            return SteamUserStats()->RequestCurrentStats();
        }

        bool IsReady() const { return m_initialized; }

        bool IsUnlocked(const char* achId) const
        {
            if (!m_initialized || !SteamUserStats())
                return false;

            bool achieved = false;
            SteamUserStats()->GetAchievement(achId, &achieved);
            return achieved;
        }

        bool Unlock(const char* achId)
        {
            if (!m_initialized || !SteamUserStats())
                return false;

            // Safe to call multiple times.
            SteamUserStats()->SetAchievement(achId);

            // Needed to persist + show popup
            return SteamUserStats()->StoreStats();
        }

    private:
        void OnUserStatsReceived(UserStatsReceived_t* p)
        {
            if (!p) return;

            if (static_cast<uint64>(m_appId) != p->m_nGameID)
                return; // ignore other games

            if (p->m_eResult == k_EResultOK)
            {
                m_initialized = true;

                bool achieved = false;
                SteamUserStats()->GetAchievement(kAchTest1, &achieved);

                RC2D_log(RC2D_LOG_INFO,
                    "Steam stats received. Achievements ready. %s=%s",
                    kAchTest1, achieved ? "UNLOCKED" : "LOCKED");
            }
            else
            {
                RC2D_log(RC2D_LOG_WARNING,
                    "Steam RequestCurrentStats failed. EResult=%d",
                    (int)p->m_eResult);
            }
        }

        void OnUserStatsStored(UserStatsStored_t* p)
        {
            if (!p) return;
            if (static_cast<uint64>(m_appId) != p->m_nGameID)
                return;

            if (p->m_eResult == k_EResultOK)
            {
                RC2D_log(RC2D_LOG_INFO, "Steam stats stored OK.");
            }
            else
            {
                RC2D_log(RC2D_LOG_WARNING,
                    "Steam StoreStats failed. EResult=%d",
                    (int)p->m_eResult);
            }
        }

        void OnAchievementStored(UserAchievementStored_t* p)
        {
            if (!p) return;
            if (static_cast<uint64>(m_appId) != p->m_nGameID)
                return;

            // p->m_rgchAchievementName contains the API name of the achievement
            RC2D_log(RC2D_LOG_INFO,
                "Steam achievement stored: %s (cur=%d / max=%d)",
                p->m_rgchAchievementName,
                (int)p->m_nCurProgress,
                (int)p->m_nMaxProgress);
        }

    private:
        AppId_t m_appId;
        bool m_initialized;

        STEAM_CALLBACK(RC2D_SteamAchievements, OnUserStatsReceived, UserStatsReceived_t, m_cbUserStatsReceived);
        STEAM_CALLBACK(RC2D_SteamAchievements, OnUserStatsStored, UserStatsStored_t, m_cbUserStatsStored);
        STEAM_CALLBACK(RC2D_SteamAchievements, OnAchievementStored, UserAchievementStored_t, m_cbAchievementStored);
    };

    static RC2D_SteamAchievements* g_ach = nullptr;
}

// -----------------------------------------------------------------------------
// C API exposed by RC2D
// -----------------------------------------------------------------------------
extern "C"
{
    bool rc2d_steam_init(void)
    {
        // In dev: steam_appid.txt next to exe (you already know)
        if (!SteamAPI_Init())
        {
            RC2D_log(RC2D_LOG_CRITICAL, "SteamAPI_Init failed (Steam client running?)");
            return false;
        }

        uint32_t appID = SteamUtils()->GetAppID();
        RC2D_log(RC2D_LOG_INFO, "Steam initialized. AppID=%u", appID);

        if (!g_ach)
            g_ach = new RC2D_SteamAchievements();

        const bool ok = g_ach->RequestStats();
        RC2D_log(RC2D_LOG_INFO, "RequestCurrentStats() => %s", ok ? "true" : "false");
        return true;
    }

    void rc2d_steam_run_callbacks(void)
    {
        SteamAPI_RunCallbacks();
    }

    void rc2d_steam_cleanup(void)
    {
        if (g_ach)
        {
            delete g_ach;
            g_ach = nullptr;
        }

        SteamAPI_Shutdown();
    }

    bool rc2d_steam_achievements_ready(void)
    {
        return g_ach && g_ach->IsReady();
    }

    bool rc2d_steam_is_achievement_unlocked_test1(void)
    {
        return g_ach && g_ach->IsUnlocked(kAchTest1);
    }

    bool rc2d_steam_unlock_achievement_test1(void)
    {
        if (!g_ach)
            return false;

        if (!g_ach->IsReady())
        {
            RC2D_log(RC2D_LOG_WARNING,
                "Achievements not ready yet. Call RunCallbacks until UserStatsReceived arrives.");
            return false;
        }

        const bool ok = g_ach->Unlock(kAchTest1);
        RC2D_log(RC2D_LOG_INFO, "Unlock(%s) => %s", kAchTest1, ok ? "true" : "false");
        return ok;
    }
}
