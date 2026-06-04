#include "steam_api.hpp"

#include "achievements.hpp"
#include "engine.hpp"
#include "core/logger.hpp"
#include "tools/player_data.hpp"
#define _ACH_ID(id, name) { id, #id, name, "", 0, 0 }

tmt::SteamAPI::SteamAPI() {
    if (SteamAPI_RestartAppIfNecessary(APP_ID)) {
        success_init = false;
        exit(0);
    }
    if (!SteamAPI_Init()) {
        success_init = false;
    }
    if (success_init) {
        account_name = SteamFriends()->GetPersonaName();
    }
}
tmt::SteamAPI::~SteamAPI() {
    SteamAPI_Shutdown();

    delete achievement;
}
void tmt::SteamAPI::update() {
    SteamAPI_RunCallbacks();
}
void tmt::SteamAPI::init_achievements() {
    if (success_init == false) {
        return;
    }
    auto& achievement_data { tmt::engine.player_data.get<std::vector<AchievementSteamData>>("steam_achievements", { _ACH_ID(ACH_COMPLETE_ONE_RUN, "Rookie") }) };
    achievement = new SteamAchievements(achievement_data.data(), static_cast<int32_t>(achievement_data.size()));
}
