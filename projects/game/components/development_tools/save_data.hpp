#pragma once
#include "engine/engine.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/components/gameplay_functionality_components/fuel.hpp"
#include "projects/game/components/gameplay_functionality_components/ore_collector.hpp"
#include "projects/game/components/gameplay_functionality_components/upgrade.hpp"
#include "projects/game/components/gameplay_functionality_components/weapon_and_tool_components/weapon.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "projects/game/data_headers/wallet.hpp"

namespace game {

template <typename T>
static Wallet* get_wallet(std::string_view label) {
    const auto view = tmt::engine.ecs.view<T>(entt::exclude_t {});
    if (view.empty()) {
        tmt::Log::error("No {} entity found", label);
        return nullptr;
    }
    auto* wallet = tmt::engine.ecs.try_get_component<Wallet>(view.front().entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component on {} entity", label);
    }
    return wallet;
}

static void apply_multiplier(Currencies& currencies, float multiplier) {
    for (auto& [ore, count] : currencies.resource_counts) {
        count = count * multiplier;
    }
}

static void save_resources_on_run(float multiplier_percentage) {
    auto* player_wallet = get_wallet<Player>("Player");
    if (player_wallet == nullptr) return;
    auto* barge_wallet = get_wallet<OreCollector>("Barger");
    if (barge_wallet == nullptr) return;

    tmt::Log::info("Penalty of {:.2f}, applied when saving current run resources", multiplier_percentage);

    apply_multiplier(barge_wallet->currencies, multiplier_percentage);
    // debugging
    barge_wallet->currencies.print_values();
    player_wallet->currencies.print_values();
    player_wallet->currencies += barge_wallet->currencies;
    // saving
    auto& persistent_curr = tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES);
    persistent_curr = player_wallet->currencies;
    player_wallet->currencies = persistent_curr;
}

}  // namespace game
