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

   private:
    glm::vec3 coord_to_world(glm::ivec2 coord, const tmt::LevelConfiguration& level_configuration);
};

}  // namespace game
