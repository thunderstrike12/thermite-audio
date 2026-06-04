#include "achievements.hpp"

#include "engine.hpp"
#include "steam_api.hpp"
#include "core/logger.hpp"
#include "magic_enum/magic_enum.hpp"

#include <steam/steamclientpublic.h>
tmt::SteamAchievements::SteamAchievements(AchievementSteamData* achievements, int32_t achievement_count) :
    achivements_handle(achievements),
    number_achivement(achievement_count),
    initialized(engine.steam.get_success_init() && engine.steam.is_user_logged_in()),
    callback_user_stats_stored(this, &SteamAchievements::on_user_stats_stored),
    callback_achievement_stored(this, &SteamAchievements::on_achievement_stored) {}
tmt::SteamAchievements::~SteamAchievements() {}
bool tmt::SteamAchievements::set_achievement(const char* id) const {
    if (initialized == false) {
        return false;
    }
    bool already_achieved = false;
    SteamUserStats()->GetAchievement(id, &already_achieved);
    if (already_achieved) {
        return true;
    }
    if (SteamUserStats()->SetAchievement(id) == false) {
        tmt::Log::error("SetAchievement failed for id '{}'", id);
        return false;
    }
    return SteamUserStats()->StoreStats();
}
void tmt::SteamAchievements::on_achievement_stored(UserAchievementStored_t* callback) {
    if (APP_ID == callback->m_nGameID) {
        tmt::Log::info("{}", "Stored Achievement for Steam");
    }
}
void tmt::SteamAchievements::on_user_stats_stored(UserStatsStored_t* callback) {
    // we may get callbacks for other games' stats arriving, ignore them
    if (APP_ID == callback->m_nGameID) {
        if (k_EResultOK == callback->m_eResult) {
            tmt::Log::info("{}", "Stored stats for Steam");
        } else {
            tmt::Log::error("{}", magic_enum::enum_name(callback->m_eResult));
        }
    }
}
