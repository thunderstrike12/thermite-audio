#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class TutorialButton : public tmt::GameComponent<TutorialButton> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "TutorialButton"; }
    void start() override;
    void update(const tmt::FrameData& time) override {};
    void end() override {}
    void disable_entity(tmt::Button::Context context);
    std::string saved_name { "tutorial" };
    std::vector<tmt::Entity> entities_to_disable {};
    std::vector<tmt::Entity> entities_to_enable {};
};

}  // namespace game
TMT_GAME_COMPONENT(game::TutorialButton, (saved_name, entities_to_disable, entities_to_enable));
