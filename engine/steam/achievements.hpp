#pragma once
#include "engine/core/reflection.hpp"
#include "steam/isteamuserstats.h"
#include "steam/steam_api.h"
#include "steam/steam_api_common.h"

namespace tmt {

enum AchievementType { ACH_COMPLETE_ONE_RUN, ACH_KILL_10_ENEMIES, ACH_DRONE_UPGRADES, ACH_DRILL_UPGRADES, ACH_BARGE_UPGRADES, ACH_RIFLE_UPGRADES };

struct AchievementSteamData {
    AchievementType ach_type;
    std::string ach_id;
    char name[128];
    char description[256];
    bool achieved;
    int32_t icon_image;
};
struct GameStats {
    int enemy_killed_count { 0 };
};

class SteamAchievements {
   public:
    SteamAchievements(AchievementSteamData* achievements, int32_t achievement_count);
    ~SteamAchievements();

    bool set_achievement(const char* id) const;

    // no need to update every frame
    void update();
    GameStats stats {};
    STEAM_CALLBACK(SteamAchievements, on_user_stats_stored, UserStatsStored_t, callback_user_stats_stored);
    STEAM_CALLBACK(SteamAchievements, on_achievement_stored, UserAchievementStored_t, callback_achievement_stored);
    bool store_stats { false };

   private:
    void load_user_stats();
    void unlock_achievement(AchievementSteamData& achievement);
    void check_achievement(AchievementSteamData& achievement);
    void store_if_needed();
    AchievementSteamData* achivements_handle { nullptr };  // Achievements data
    int32_t achievement_count {};                          // The number of Achievements
    bool initialized { false };                            // Are we ready to use the API?

    ISteamUserStats* user_stats { nullptr };
    bool valid_stats { false };
};

}  // namespace tmt
TMT_OBJECT(tmt::GameStats, (enemy_killed_count));

TMT_OBJECT(tmt::AchievementSteamData, (ach_type, ach_id, name, description, achieved, icon_image))
