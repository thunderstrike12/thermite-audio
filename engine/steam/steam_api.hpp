#pragma once
#include <string>
#include <steam/steam_api.h>
namespace tmt {

constexpr auto APP_ID = 4515200u;
class SteamAPI {
   public:
    SteamAPI();
    ~SteamAPI();
    void update();
    bool get_success_init() const { return success_init; }
    std::string_view get_persona_name() const { return account_name; }

   private:
    bool success_init { true };
    std::string account_name { "Invalid Steam Init" };
};

}  // namespace tmt