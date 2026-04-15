#pragma once
#include "engine/engine.hpp"
#include "engine/tools/player_data.hpp"
#include "projects/game/components/gameplay_functionality_components/ore_collector.hpp"
#include "projects/game/data_headers/save_entries.hpp"
#include "projects/game/data_headers/wallet.hpp"
namespace game {

static void save_resources_on_run(float multiplier_percentage) {
    auto* wallet = tmt::engine.ecs.try_get_component<game::Wallet>(tmt::engine.ecs.view<OreCollector>().front().entity);
    if (wallet == nullptr) {
        tmt::Log::error("No Wallet component found");

        return;
    }

    tmt::Log::info("Penalty of {:.2f}, applied when saving current run resources", multiplier_percentage);
    for (auto& pair : wallet->currencies.resource_counts) {
        pair.second = static_cast<uint64_t>(static_cast<float>(pair.second) * multiplier_percentage);
    }
    wallet->currencies.dollars = static_cast<uint64_t>(static_cast<float>(wallet->currencies.dollars) * multiplier_percentage);

    wallet->currencies.print_values();

    auto& persistent_curr { tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES) };

    persistent_curr += wallet->currencies;
    wallet->currencies = Currencies {};
}

}  // namespace game
