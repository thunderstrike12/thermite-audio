#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/button.hpp"

namespace game {

class TutorialFlipbook : public tmt::GameComponent<TutorialFlipbook> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "TutorialFlipbook"; }
    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void next_page(tmt::Button::Context context);
    void previous_page(tmt::Button::Context context);

    void open_flipbook(tmt::Button::Context context);
    void close_flipbook(tmt::Button::Context context);

    void unlock_page(uint8_t page);

    tmt::Entity previous_button {};
    tmt::Entity next_button {};
    tmt::Entity page_text {};
    tmt::Entity open_button {};
    tmt::Entity close_button {};
    std::vector<tmt::Entity> ui_entities {};
    
   private:
    std::set<uint8_t> unlocked_pages {};
    uint8_t current_page = 0;
};

}  // namespace game
TMT_GAME_COMPONENT(game::TutorialFlipbook, (previous_button, next_button, page_text, open_button, close_button, ui_entities));