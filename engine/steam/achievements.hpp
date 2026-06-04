#pragma once
#include "engine/core/reflection.hpp"
#include "steam/isteamuserstats.h"
#include "steam/steam_api.h"
#include "steam/steam_api_common.h"

namespace tmt {

enum AchievementType {
    ACH_COMPLETE_ONE_RUN,
    ACH_PENALTY_10_GAMES = 1,
    ACH_KILL = 2,
    ACH_TRAVEL_FAR_ACCUM = 3,
    ACH_TRAVEL_FAR_SINGLE = 4,
};

struct AchievementSteamData {
    AchievementType ach_type;
    std::string ach_id;
    char name[128];
    char description[256];
    bool achieved;
    int32_t icon_image;
};

class SteamAchievements {
   public:
    SteamAchievements(AchievementSteamData* achievements, int32_t achievement_count);
    ~SteamAchievements();

    bool set_achievement(const char* id) const;

    STEAM_CALLBACK(SteamAchievements, on_user_stats_stored, UserStatsStored_t, callback_user_stats_stored);
    STEAM_CALLBACK(SteamAchievements, on_achievement_stored, UserAchievementStored_t, callback_achievement_stored);

   private:
    AchievementSteamData* achivements_handle { nullptr };  // Achievements data
    int32_t number_achivement {};                          // The number of Achievements
    bool initialized { false };                            // Are we ready to use the API?
};

}  // namespace tmt

TMT_OBJECT(tmt::AchievementSteamData, (ach_type, ach_id, name, description, achieved, icon_image))
