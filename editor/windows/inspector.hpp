#pragma once
#include "editor/core/window.hpp"
#include "engine/core/entity.hpp"
#include <unordered_set>

namespace tmt {
class Inspector : public IWindow {
    // Inherited via IWindow
    std::string get_title() const { return ICON_MS_FRAME_INSPECT "  Inspector"; }
    constexpr bool default_open() const override { return true; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    struct MenuContext {
        const Entity primary_entity;
        const std::unordered_set<Entity>& selected_entities;
    };

    void add_component(const MenuContext& menu_context);
};
}  // namespace tmt