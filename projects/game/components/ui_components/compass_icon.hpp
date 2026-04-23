#include "engine/systems/gameplay/game_component.hpp"
namespace game {

class CompassIcon : public tmt::GameComponent<CompassIcon> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "CompassIcon"; }
    void start() override {};
    void update(const tmt::FrameData& time) override;

    void move_to_position(float factor) const;
    void end() override {}

    tmt::Entity relative_entity { entt::null };
    float radius { 50.f };
    tmt::Entity enable_above { entt::null };
    tmt::Entity enable_below { entt::null };
};

}  // namespace game
TMT_OBJECT(game::CompassIcon, (relative_entity, radius));
