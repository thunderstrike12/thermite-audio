#pragma once
#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <ImReflect.hpp>
#include <unordered_set>
#include <algorithm>
#include <cctype>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"

#include "editor/font/icon_lookups.hpp"
#include "editor/windows/hierarchy.hpp"

/* Custom response type for entity */
template <>
struct ImReflect::type_response<entt::entity> : ImReflect::Detail::required_response<entt::entity> {
   private:
    bool is_dropped = false;

   public:
    void dropped() { is_dropped = true; }
    bool is_entity_dropped() const { return is_dropped; }
};

namespace ImReflect::Detail {

/* Forward declaration for recursive entity display */
inline void display_entity_recursive(entt::entity entity, entt::entity& selected_value, type_response<entt::entity>& response, std::string& search_filter, int depth = 0);

/* Check if entity or any of its descendants match the search filter */
inline bool entity_matches_search(entt::entity entity, const std::string& filter) {
    if (!tmt::engine.ecs.valid(entity)) return false;
    if (!tmt::engine.ecs.has_component<tmt::Name>(entity)) return false;

    const auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity);

    /* Case-insensitive search */
    std::string name_lower = name.name;
    std::string filter_lower = filter;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
    std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);

    if (name_lower.find(filter_lower) != std::string::npos) {
        return true;
    }

    /* Check children recursively */
    if (tmt::engine.ecs.has_component<tmt::Transform>(entity)) {
        const auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
        for (const auto& child : transform.get_children()) {
            if (entity_matches_search(child, filter)) {
                return true;
            }
        }
    }

    return false;
}

/* Recursive entity display helper */
inline void display_entity_recursive(entt::entity entity, entt::entity& selected_value, type_response<entt::entity>& response, std::string& search_filter, int depth) {
    if (!tmt::engine.ecs.valid(entity)) return;
    if (!tmt::engine.ecs.has_component<tmt::Name>(entity)) return;
    if (!tmt::engine.ecs.has_component<tmt::Transform>(entity)) return;

    const auto& name = tmt::engine.ecs.get_component<tmt::Name>(entity);
    const auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    /* When filtering, check if this entity or descendants match */
    if (!search_filter.empty()) {
        if (!entity_matches_search(entity, search_filter)) {
            return;
        }
    }

    const auto entity_id = static_cast<uint32_t>(entity);
    const bool is_selected = (entity == selected_value);

    /* Display entity with ID */
    char label[256];
    snprintf(label, sizeof(label), "%s [%u]", name.name.c_str(), entity_id);

    if (ImGui::Selectable(label, is_selected)) {
        selected_value = entity;
        response.changed();
        search_filter.clear();
        ImGui::CloseCurrentPopup();
    }

    /* Recursively show children */
    if (transform.has_children()) {
        ImGui::Indent();
        for (const auto& child : transform.get_children()) {
            display_entity_recursive(child, selected_value, response, search_filter, depth + 1);
        }
        ImGui::Unindent();
    }
}

/* Entity selector popup implementation */
inline void entity_selector_popup(const char* popup_id, entt::entity& value, type_response<entt::entity>& response) {
    if (!ImGui::BeginPopup(popup_id)) return;

    static std::string search_filter;

    /* Search bar */
    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputTextWithHint("##EntitySearch", ICON_MS_SEARCH " Search entities...", &search_filter);
    ImGui::Separator();

    /* Null option */
    if (ImGui::Selectable("None (null)", value == entt::null)) {
        value = entt::null;
        response.changed();
        search_filter.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::Separator();

    /* Entity list */
    auto view = tmt::engine.ecs.get_registry().view<tmt::Transform, tmt::Name>();

    ImGui::BeginChild("EntityList", ImVec2(250, 300), true);
    for (auto [entity, transform, name] : view.each()) {
        /* Only show root entities - children are displayed recursively */
        if (transform.has_parent()) {
            continue;
        }

        display_entity_recursive(entity, value, response, search_filter, 0);
    }
    ImGui::EndChild();

    ImGui::EndPopup();
}

/* Drag drop target for hierarchy entities */
inline bool entity_drag_drop_target(entt::entity& value, type_response<entt::entity>& response) {
    if (!ImGui::BeginDragDropTarget()) return false;

    const ImGuiPayload* payload = ImGui::GetDragDropPayload();
    if (payload != nullptr && payload->IsDataType("HIERARCHY_ENTITY")) {
        if (ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY") != nullptr) {
            const auto* payload_data = static_cast<const tmt::Hierarchy::DragNDropPayload*>(payload->Data);
            value = payload_data->entity;
            response.dropped();
            response.changed();
            ImGui::EndDragDropTarget();
            return true;
        }
    }

    ImGui::EndDragDropTarget();
    return false;
}

}  // namespace ImReflect::Detail

/* Main input implementation for mutable entity reference */
inline void tag_invoke(ImReflect::ImInput_t, const char* label, entt::entity& value, ImSettings& settings, ImResponse& response) {
    auto& entity_response = response.get<entt::entity>();
    const auto entity_id = static_cast<uint32_t>(value);
    const bool is_null = (value == entt::null);
    const bool is_valid = is_null || tmt::engine.ecs.valid(value);

    /* Log error for invalid (non-null) entities - rate limited per entity */
    if (!is_valid) {
        static std::unordered_set<uint32_t> logged_invalid_entities;
        if (!logged_invalid_entities.contains(entity_id)) {
            tmt::Log::error(tmt::Log::Scope::EDITOR, "Invalid entity reference: {} - entity no longer exists in registry", entity_id);
            logged_invalid_entities.insert(entity_id);
        }
    }

    ImGui::PushID(label);

    /* Label */
    if (label && label[0] != '\0') {
        ImReflect::Detail::text_label(label);
        ImGui::SameLine();
    }

    /* Error styling for invalid entities */
    if (!is_valid) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    /* Begin group for drag drop target */
    ImGui::BeginGroup();

    /* Display text */
    std::string display_text;
    if (is_null) {
        display_text = "None (null)";
    } else if (!is_valid) {
        display_text = "INVALID [" + std::to_string(entity_id) + "]";
    } else {
        const auto* name_component = tmt::engine.ecs.try_get_component<tmt::Name>(value);
        if (name_component) {
            display_text = name_component->name + " [" + std::to_string(entity_id) + "]";
        } else {
            display_text = "Entity [" + std::to_string(entity_id) + "]";
        }
    }

    /* Calculate widths */
    const float button_width = ImGui::CalcTextSize(ICON_MS_TARGET).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float available_width = ImGui::CalcItemWidth();
    const float input_width = available_width - button_width - ImGui::GetStyle().ItemSpacing.x;

    /* Selectable input field (shows entity info) */
    ImGui::SetNextItemWidth(input_width > 50.0f ? input_width : 50.0f);
    ImGui::InputText("##EntityDisplay", &display_text, ImGuiInputTextFlags_ReadOnly);

    /* Tooltip with more info */
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (is_null) {
            ImGui::Text("No entity assigned");
        } else if (!is_valid) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Entity %u is invalid!", entity_id);
            ImGui::Text("The entity no longer exists in the registry.");
        } else {
            ImGui::Text("Entity ID: %u", entity_id);
            const auto* name = tmt::engine.ecs.try_get_component<tmt::Name>(value);
            if (name) ImGui::Text("Name: %s", name->name.c_str());
        }
        ImGui::Separator();
        ImGui::TextDisabled("Drag an entity from Hierarchy to assign");
        ImGui::EndTooltip();
    }

    ImGui::EndGroup();

    /* Handle drag drop on the input field */
    ImReflect::Detail::entity_drag_drop_target(value, entity_response);

    /* Selector button */
    ImGui::SameLine();
    const char* popup_id = "##EntitySelectorPopup";
    if (ImGui::Button(ICON_MS_TARGET)) {
        ImGui::OpenPopup(popup_id);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Select entity from list");
    }

    /* Entity selector popup */
    ImReflect::Detail::entity_selector_popup(popup_id, value, entity_response);

    /* Pop error styling */
    if (!is_valid) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    ImGui::PopID();

    /* Check input states */
    ImReflect::Detail::check_input_states(entity_response);
}

/* Const version - read only display */
inline void tag_invoke(ImReflect::ImInput_t, const char* label, const entt::entity& value, ImSettings& settings, ImResponse& response) {
    auto& entity_response = response.get<entt::entity>();
    const auto entity_id = static_cast<uint32_t>(value);
    const bool is_null = (value == entt::null);
    const bool is_valid = is_null || tmt::engine.ecs.valid(value);

    ImGui::PushID(label);

    /* Label */
    if (label && label[0] != '\0') {
        ImReflect::Detail::text_label(label);
        ImGui::SameLine();
    }

    /* Error styling for invalid entities */
    if (!is_valid) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    }

    /* Display text */
    std::string display_text;
    if (is_null) {
        display_text = "None (null)";
    } else if (!is_valid) {
        display_text = "INVALID [" + std::to_string(entity_id) + "]";
    } else {
        const auto* name_component = tmt::engine.ecs.try_get_component<tmt::Name>(value);
        if (name_component) {
            display_text = name_component->name + " [" + std::to_string(entity_id) + "]";
        } else {
            display_text = "Entity [" + std::to_string(entity_id) + "]";
        }
    }

    ImGui::TextUnformatted(display_text.c_str());

    /* Tooltip */
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (is_null) {
            ImGui::Text("No entity assigned");
        } else if (!is_valid) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Entity %u is invalid!", entity_id);
        } else {
            ImGui::Text("Entity ID: %u", entity_id);
            const auto* name = tmt::engine.ecs.try_get_component<tmt::Name>(value);
            if (name) ImGui::Text("Name: %s", name->name.c_str());
        }
        ImGui::TextDisabled("(Read-only)");
        ImGui::EndTooltip();
    }

    if (!is_valid) {
        ImGui::PopStyleColor();
    }

    ImGui::PopID();

    ImReflect::Detail::check_input_states(entity_response);
}
