#pragma once
#include "ore_properties.hpp"
#include "save_entries.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/tools/player_data.hpp"

namespace game {

struct Currencies {
    uint64_t dollars = 0u;

    std::unordered_map<OreProperties::OreResources, float> resource_counts = { { OreProperties::OreResources::NONE, 0.0f },
                                                                               { OreProperties::OreResources::SCRAP, 0.0f },
                                                                               { OreProperties::OreResources::COPPER, 0.0f },
                                                                               { OreProperties::OreResources::THERMITE, 0.0f },
                                                                               { OreProperties::OreResources::TITANIUM, 0.0f } };

    Currencies& operator+=(const Currencies& other) {
        dollars += other.dollars;
        for (const auto& [ore, count] : other.resource_counts) {
            resource_counts[ore] += count;
        }
        return *this;
    }

    Currencies operator+(const Currencies& other) const {
        Currencies result = *this;
        result += other;
        return result;
    }

    void print_values() {
        tmt::Log::info("Dollars: {}", dollars);
        for (auto& pair : resource_counts) {
            tmt::Log::info("Resource {}, count {}", magic_enum::enum_name(pair.first), pair.second);
        }
    }
};
struct WalletData {
    Currencies limits;
    int total_resource_limit;
};
class Wallet : public tmt::GameComponent<Wallet> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Wallet"; }

    void start() override {
        auto wallet_data { tmt::engine.player_data.try_get<WalletData>(WALLET_DATA) };
        if (wallet_data.has_value()) {
            limits = wallet_data->limits;
            total_resource_limit = wallet_data->total_resource_limit;
        }
    }
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    // Currencies;
    Currencies currencies;
    Currencies limits;

    int total_resource_limit = -1;
};

}  // namespace game
TMT_OBJECT(game::Currencies, (dollars, resource_counts));
TMT_OBJECT(game::Wallet, (currencies, limits, total_resource_limit));
TMT_OBJECT(game::WalletData, (limits, total_resource_limit));
