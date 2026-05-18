#include "steam_api.hpp"

#include "core/logger.hpp"
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
}
void tmt::SteamAPI::update() {
    SteamAPI_RunCallbacks();
}