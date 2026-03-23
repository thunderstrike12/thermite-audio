#pragma once
#include "engine/systems/gameplay/game_component.hpp"
namespace game {

class OreProperties : public tmt::GameComponent<OreProperties> {
   public:
    using GameComponent::GameComponent;

    enum class OreResources : uint8_t {
        NONE,
        SCRAP,
        COPPER,
        THERMITE,
        TITANIUM,
    };
    struct MiningOre {
        float value = 1.0f;
        float toughness = 1.0f;
        float weight = 1.0f;
        uint32_t resource_per_voxel = 1u;
        OreResources ore_resource = OreResources::NONE;
    };

    static std::string_view get_name() { return "Ore Properties"; }
    void start() override {}
    void update(const tmt::FrameData& time) override {}
    void end() override {}

    std::unordered_map<tmt::Material::Type, MiningOre> ores;
};

}  // namespace game
TMT_OBJECT(game::OreProperties::MiningOre, (value, toughness, weight, resource_per_voxel, ore_resource));
TMT_OBJECT(game::OreProperties, (ores));
