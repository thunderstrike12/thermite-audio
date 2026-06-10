#include "achievements.hpp"

#include "engine.hpp"
#include "steam_api.hpp"
#include "core/logger.hpp"
#include "magic_enum/magic_enum.hpp"

#include <steam/steamclientpublic.h>
tmt::SteamAchievements::SteamAchievements(AchievementSteamData* achievements, int32_t achievement_count) :
    achivements_handle(achievements),
    achievement_count(achievement_count),
    initialized(engine.steam.get_success_init() && engine.steam.is_user_logged_in()),
    callback_user_stats_stored(this, &SteamAchievements::on_user_stats_stored),
    callback_achievement_stored(this, &SteamAchievements::on_achievement_stored),
    user_stats(SteamUserStats()) {}
tmt::SteamAchievements::~SteamAchievements() {}
bool tmt::SteamAchievements::set_achievement(const char* id) const {
    if (initialized == false) {
        return false;
    }
    bool already_achieved = false;
    user_stats->GetAchievement(id, &already_achieved);
    if (already_achieved) {
        return true;
    }
    if (user_stats->SetAchievement(id) == false) {
        Log::error("SetAchievement failed for id '{}'", id);
        return false;
    }
    return user_stats->StoreStats();
}
void tmt::SteamAchievements::update() {
    if (!initialized) return;
    if (valid_stats == false) {
        load_user_stats();
    }
    for (auto i { 0 }; i < achievement_count; i++) {
        auto& achievement { achivements_handle[i] };
        check_achievement(achievement);
    }
    store_if_needed();
}
void tmt::SteamAchievements::on_user_stats_stored(UserStatsStored_t* callback) {
    // we may get callbacks for other games' stats arriving, ignore them
    if (APP_ID == callback->m_nGameID) {
        if (k_EResultOK == callback->m_eResult) {
            Log::info("{}", "Stored stats for Steam");
        } else if (k_EResultInvalidParam == callback->m_eResult) {
            Log::error("Failed to validate stats");
        } else {
            Log::error("Still failed for this reason:{}", magic_enum::enum_name(callback->m_eResult));
        }
    }
}
void tmt::SteamAchievements::on_achievement_stored(UserAchievementStored_t* callback) {
    if (APP_ID == callback->m_nGameID) {
        if (0 == callback->m_nMaxProgress) {
            Log::info("Achievement {} unlocked!", callback->m_rgchAchievementName);
        } else {
            Log::info("Achievement {} progress callback, ({},{}", callback->m_rgchAchievementName, callback->m_nCurProgress, callback->m_nMaxProgress);
        }
    }
}
void tmt::SteamAchievements::load_user_stats() {
    if (user_stats == nullptr || achivements_handle == nullptr) {
        return;
    }

    for (auto i { 0 }; i < achievement_count; i++) {
        auto& achievement { achivements_handle[i] };
        user_stats->GetAchievement(achievement.ach_id.c_str(), &achievement.achieved);
    }
    // Add other stats here:
    bool get_state_success { true };
    get_state_success &= user_stats->GetStat("enemy_killed_count", &stats.enemy_killed_count);
    get_state_success &= user_stats->GetStat("voxels_mined", &stats.voxels_mined_count);
    get_state_success &= user_stats->GetStat("distance_traveled", &stats.distance_traveled);

    valid_stats = get_state_success;
}
void tmt::SteamAchievements::unlock_achievement(AchievementSteamData& achievement) {
    achievement.achieved = true;

    achievement.icon_image = 0;

    user_stats->SetAchievement(achievement.ach_id.c_str());

    store_stats = true;
}
void tmt::SteamAchievements::check_achievement(AchievementSteamData& achievement) {
    if (achievement.achieved) {
        return;
    }
    switch (achievement.ach_type) {
        case ACH_KILL_10_ENEMIES:
            if (stats.enemy_killed_count >= 10) {
                unlock_achievement(achievement);
            }
            break;
        case ACH_VOXELS_MINED:
            if (stats.voxels_mined_count >= 1'000'000) {
                unlock_achievement(achievement);
            }
            break;
        case ACH_MARATHON:
            if (stats.distance_traveled >= 42'195) {
                unlock_achievement(achievement);
            }
            break;
        case ACH_COMPLETE_ONE_RUN:
            // handled directly right now, could change it to use stats?
            break;
        default:
            break;
    }
}
void tmt::SteamAchievements::store_if_needed() {
    if (store_stats == false) {
        return;
    }
    // set everything here
    user_stats->SetStat("enemy_killed_count", stats.enemy_killed_count);
    user_stats->SetStat("voxels_mined", stats.voxels_mined_count);
    user_stats->SetStat("distance_traveled", stats.distance_traveled);

    bool success = user_stats->StoreStats();
    store_stats = !success;
}
