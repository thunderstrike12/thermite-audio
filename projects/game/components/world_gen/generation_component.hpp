#pragma once
#include "engine/systems/gameplay/level_config.hpp"
#include "engine/systems/gameplay/game_component.hpp"
namespace game {

struct GenerationComponent : public tmt::GameComponent<GenerationComponent> {
    using GameComponent::GameComponent;

    void clear_children();

    static std::string_view get_name() { return "Generation Component"; }
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    // void draw_debug_lines() const override;
   private:
    bool generated_lights = false;
    tmt::Entity main_sun_entity;
    std::vector<std::tuple<int, tmt::PoissonPoint, tmt::Entity, bool>> lighting_pass_data;
    tmt::LevelConfiguration level_configuration;
    glm::vec3 coord_to_world(glm::ivec2 coord, const tmt::LevelConfiguration& level_configuration);
};

struct ClearanceVolume : public tmt::GameComponent<ClearanceVolume> {
    using GameComponent::GameComponent;

    // Inherited via GameComponent
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    static std::string_view get_name() { return "Clearance Volume Component"; }

    glm::vec3 size = glm::vec3(0.f);

    bool contains(const glm::vec3& pos);
};

}  // namespace game
TMT_GAME_COMPONENT(game::ClearanceVolume, (size));
TMT_GAME_COMPONENT_EMPTY(game::GenerationComponent);
