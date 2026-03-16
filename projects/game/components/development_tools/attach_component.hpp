#pragma once
#include "debug_line_helper.hpp"
#include "projects/game/data_headers/events.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resources/stencil.hpp"
#include "engine/systems/physics/components/voxel_body.hpp"

namespace game {

class AttachComponent : public tmt::GameComponent<AttachComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "AttachComponent"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;
    void draw_debug_lines() const override;
    bool check_inside_range(const game::AttachAttemptEvent& event) const;
    void on_check_range_to_attach(const AttachAttemptEvent& event);

    float radius = 1.0f;
    tmt::Entity entity_that_attaches = entt::null;
    DebugLineConfig cfg;

   private:
    bool is_attached = false;
};

}  // namespace game
TMT_OBJECT(game::AttachComponent, (radius, entity_that_attaches, cfg));
