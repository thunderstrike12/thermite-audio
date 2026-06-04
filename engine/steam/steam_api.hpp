#pragma once
#include <string>
#include <steam/steam_api.h>
namespace tmt {

class SteamAchievements;

constexpr auto APP_ID = 4515200u;
class SteamAPI {
   public:
    SteamAPI();
    ~SteamAPI();
    void update();
    void init_achievements();
    bool get_success_init() const { return success_init; }
    bool is_user_logged_in() { return SteamUser()->BLoggedOn(); }
    std::string_view get_persona_name() const { return account_name; }

    // Do not free this pointer, it is owned by the steam api.
    SteamAchievements* achievement { nullptr };

   private:
    bool success_init { true };
    std::string account_name { "Invalid Steam Init" };
};

}  // namespace tmt
