#pragma once
#include "projects/game/components/development_tools/debug_line_helper.hpp"
#include "engine/systems/gameplay/game_component.hpp"

namespace game {

class TextComponent : public tmt::GameComponent<TextComponent> {
   public:
    using GameComponent::GameComponent;
    static std::string_view name() { return "TextComponent"; }
    void start() override {}
    void draw_text() const;
    void update(const tmt::FrameData& time) override;
    void draw_debug_lines() const override;
    void end() override {}

    std::string text;
    float scale = 1.0f;
    DebugLineConfig config;
};

}  // namespace game
TMT_OBJECT(game::TextComponent, (text, scale, config));
