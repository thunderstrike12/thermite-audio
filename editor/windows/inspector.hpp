#pragma once
#include <unordered_set>

#include "editor/core/window.hpp"
#include "engine/core/entity.hpp"
#include "engine/tools/serializer.hpp"

namespace tmt {
class Inspector : public IWindow {
    // Inherited via IWindow
    std::string get_title() const override { return ICON_MS_FRAME_INSPECT "  Inspector"; }
    constexpr bool default_open() const override { return true; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    struct MenuContext {
        const Entity primary_entity;
        const std::vector<Entity>& selected_entities;
    };

    void display_entity_info(const MenuContext& menu_context);

    void display_runtime_components(const MenuContext& menu_context);
    void display_compile_time_components(const tmt::Inspector::MenuContext& menu_context);

    void add_component(const MenuContext& menu_context);
    void add_compile_time_component(const tmt::Inspector::MenuContext& menu_context);
    void add_runtime_component(const tmt::Inspector::MenuContext& menu_context);

    void paste_compile_time_component(const json& deserialized, const MenuContext& menu_context);
    void paste_runtime_component(const json& deserialized, const MenuContext& menu_context);
    void paste_component(const auto& name, const MenuContext& menu_context);

    void paste_runtime_values(const json& deserialized, const MenuContext& menu_context);
    void paste_compile_time_values(const json& deserialized, const MenuContext& menu_context);
    void paste_values(const auto& name, const MenuContext& menu_context);

    struct HeaderResponse {
        const bool open = false;
        const bool right_clicked = false;
    };
    HeaderResponse component_header(const std::string name);

    struct ContextMenuResponse {
        bool remove_component = false;
        bool copy_component = false;
        bool paste_component = false;
        bool paste_values = false;
    };
    ContextMenuResponse context_menu(const std::string& name, const bool open);
};
}  // namespace tmt