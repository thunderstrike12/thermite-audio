#pragma once
#include "engine/systems/gameplay/game_component.hpp"
namespace game {

class CompassIcon : public tmt::GameComponent<CompassIcon> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "CompassIcon"; }
    void start() override {}
    void follow_relative_transform(glm::vec3 relative_vector);
    void follow_world_direction(glm::vec3 world_dir);
    void update(const tmt::FrameData& time) override;

    void move_to_position(float factor);
    void end() override {}

    tmt::Entity relative_entity { entt::null };
    float radius { 50.f };
    float min_threshold { 0.8f };
    float max_threshold { glm::pi<float>() };
    float move_increments { glm::pi<float>() / 4.0f };
    glm::vec2 default_placement { 1.0f, 0.0f };
    tmt::Entity enable_above { entt::null };
    tmt::Entity enable_below { entt::null };

   private:
};

}  // namespace game
TMT_GAME_COMPONENT(game::CompassIcon, (relative_entity, radius, default_placement, enable_above, enable_below, min_threshold, max_threshold, move_increments));
