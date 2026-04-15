#pragma once
#include "ore_properties.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

struct Currencies {
    uint64_t dollars = 0u;

    std::unordered_map<OreProperties::OreResources, uint64_t> resource_counts = {
        { OreProperties::OreResources::NONE, 0 },     { OreProperties::OreResources::SCRAP, 0 },    { OreProperties::OreResources::COPPER, 0 },
        { OreProperties::OreResources::THERMITE, 0 }, { OreProperties::OreResources::TITANIUM, 0 },
    };

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
class Wallet : public tmt::GameComponent<Wallet> {
   public:
    using GameComponent::GameComponent;

    static std::string_view get_name() { return "Wallet"; }

    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    // Currencies;
    Currencies currencies;
};

}  // namespace game
TMT_OBJECT(game::Currencies, (dollars, resource_counts));
TMT_OBJECT(game::Wallet, (currencies));
