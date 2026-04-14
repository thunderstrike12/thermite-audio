#pragma once
#include <unordered_set>

#include "editor/core/window.hpp"
#include "engine/core/entity.hpp"
#include "engine/tools/serializer.hpp"

namespace tmt {

class Inspector : public IWindow<> {
    // Inherited via IWindow
    std::string get_title() const override { return ICON_MS_SETTINGS "  Inspector"; }
    constexpr bool default_open() const override { return true; }

    void on_inspect() override;

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

    static bool contains_filter(std::string input, std::string filter);

    struct HeaderResponse {
        const bool open = false;
        const bool right_clicked = false;
    };
    HeaderResponse component_header(const std::string name);
    void component_body_begin();
    void component_body_end();

    struct ContextMenuResponse {
        bool remove_component = false;
        bool copy_component = false;
        bool paste_component = false;
        bool paste_values = false;
    };
    ContextMenuResponse context_menu(const std::string& name, const bool open);

    float component_body_start_y = 0.0f;
    float component_body_x_min = 0.0f;
    float component_body_x_max = 0.0f;

    std::string filter = "";
};

}  // namespace tmt