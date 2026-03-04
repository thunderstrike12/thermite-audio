#include "inspector.hpp"

#include <imgui.h>
#include <ImReflect.hpp>

#include "engine/engine.hpp"

#include "engine/core/collection.hpp"
#include "engine/core/components/all.hpp"
#include "engine/core/logger.hpp"

#include "engine/tools/serializer/all.hpp"

#include "editor/editor.hpp"

#include "editor/imgui/types/glm.hpp"
#include "editor/imgui/types/entt.hpp"

#include "editor/windows/hierarchy.hpp"

#include "editor/imgui/components/all.hpp"
#include "editor/imgui/types/all.hpp"

#include "editor/events/scene.hpp"
#include "editor/core/systems/undo_redo/component_diff.hpp"

namespace tmt {

void Inspector::on_editor_start() {}

void Inspector::on_editor_update(const tmt::FrameData&) {}

void Inspector::on_editor_end() {}

void Inspector::display() {
    const auto& hierarchy = editor.windows[editor.editor_mode].get<Hierarchy>();

    const MenuContext menu_context {
        .primary_entity = hierarchy.get_first_selected_entity(),
        .selected_entities = hierarchy.get_selected_entities(),
    };

    display_entity_info(menu_context);

    display_compile_time_components(menu_context);

    display_runtime_components(menu_context);

    add_component(menu_context);
}

void Inspector::display_entity_info(const MenuContext& menu_context) {
    if (menu_context.primary_entity == entt::null) {
        return;
    }

    bool marked_disabled = !engine.ecs.has_component<DisableFlag>(menu_context.primary_entity);
    if (ImGui::Checkbox("##Disabled", &marked_disabled)) {
        if (marked_disabled) {
            for (const Entity& entity : menu_context.selected_entities) {
                engine.ecs.enable(entity);
            }
        } else {
            for (const Entity& entity : menu_context.selected_entities) {
                engine.ecs.disable(entity);
            }
        }
    }

    ImGui::SameLine();

    auto entity_str = tmt::EntityHelper::to_string(menu_context.primary_entity);
    ImGui::Text("Entity: %s", entity_str.c_str());
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Click to copy Entity ID");
    }
}

template <typename T>
struct CanBeDeleted : std::true_type {};

template <>
struct CanBeDeleted<Transform> : std::false_type {};
template <>
struct CanBeDeleted<Name> : std::false_type {};
template <>
struct CanBeDeleted<Prefab> : std::false_type {};

void Inspector::display_compile_time_components(const tmt::Inspector::MenuContext& menu_context) {
    InspectComponents::for_each([&menu_context, this](auto type_tag) {
        using T = typename decltype(type_tag)::type;  // Extract type from tag

        const bool has_component = tmt::engine.ecs.has_component<T>(menu_context.primary_entity);
        if (has_component == false) return;

        T& component_instance = tmt::engine.ecs.get_component<T>(menu_context.primary_entity);

        const auto& name = tmt::Component<T>::get_name();
        ImReflect::Detail::scope_id scope_id { name };

        const HeaderResponse header_response = component_header(name);

        const ContextMenuResponse context_response = context_menu(name, header_response.right_clicked);

        if (context_response.remove_component) {
            if constexpr (CanBeDeleted<T>::value == false) {
                tmt::Log::warn("Component '{}' cannot be removed.", name);
                return;
            }
            UndoRedoCollection collection;
            for (const Entity& entity : menu_context.selected_entities) {
                const bool has_comp = engine.ecs.has_component<T>(entity);
                if (has_comp == false) continue;

                ComponentDiff<T> component_diff(entity);
                component_diff.before();
                tmt::engine.ecs.remove_component<T>(entity);
                component_diff.after();
                collection.add_action(component_diff);
            }
            collection.commit("Remove Component: " + std::string(name));
            return;
        }

        if (context_response.copy_component) {
            const json serialized = tmt::Serializer::serialize(component_instance);
            json clipboard_json;
            clipboard_json["runtime"] = false;
            clipboard_json["component_type"] = name;
            clipboard_json["data"] = serialized;
            ImGui::SetClipboardText(clipboard_json.dump().c_str());
        }

        if (context_response.paste_component) {
            paste_component(name, menu_context);
        }

        if (context_response.paste_values) {
            paste_values(name, menu_context);
        }

        if (header_response.open == false) return;

        const json before = tmt::Serializer::serialize(component_instance);

        const ImResponse response = ImReflect::Input("", component_instance);
        const bool changed = response.get<T>().is_changed();
        const bool activated = response.get<T>().is_activated();
        const bool deactivated_after_edit = response.get<T>().is_deactivated_after_edit();

        if (changed) {
            OnSceneModified::dispatch();
        }

        const bool multiple_selection = menu_context.selected_entities.size() > 1;
        if (changed && multiple_selection) {
            auto after = tmt::Serializer::serialize(component_instance);
            const json diff = nlohmann::json::diff(before, after);

            for (const Entity& entity : menu_context.selected_entities) {
                if (entity == menu_context.primary_entity) continue;

                const bool has_component = tmt::engine.ecs.has_component<T>(entity);
                if (has_component == false) continue;

                T& other_instance = tmt::engine.ecs.get_component<T>(entity);
                json other_json = tmt::Serializer::serialize(other_instance);
                other_json.patch_inplace(diff);
                tmt::Serializer::deserialize(other_json, other_instance);
            }
        }

        static std::unordered_map<Entity, ComponentDiff<T>> component_diff;
        if (deactivated_after_edit) {
            printf("Deactivated component editor: %s\n", name);
            UndoRedoCollection collection;
            for (auto& [entity, diff] : component_diff) {
                diff.after();
                collection.add_action(std::move(diff));
            }
            collection.commit("Edit Component: " + std::string(name));
            component_diff.clear();
        }

        if (activated) {
            printf("Activated component editor: %s\n", name);
            for (const Entity& entity : menu_context.selected_entities) {
                const bool has_component_2 = tmt::engine.ecs.has_component<T>(entity);
                if (has_component_2 == false) continue;

                component_diff.emplace(entity, ComponentDiff<T>(entity));
                component_diff.at(entity).before();
            }
        }
    });
}

void Inspector::display_runtime_components(const MenuContext& menu_context) {
    const auto& registered_component = engine.component_registry.get_registered_components();

    for (const auto& [component_index, component_info] : registered_component) {
        const bool has_collection = engine.ecs.has_component<ComponentCollection>(menu_context.primary_entity);
        if (has_collection == false) continue;

        auto& component_collection = engine.ecs.get_component<ComponentCollection>(menu_context.primary_entity);
        const bool has_component = component_collection.has_component(component_index);
        if (has_component == false) continue;

        IGameComponent& component_instance = component_collection.get_component(component_index);

        const auto& name = component_info.name;
        ImReflect::Detail::scope_id scope_id { name.c_str() };

        const HeaderResponse header_response = component_header(name);

        const ContextMenuResponse context_response = context_menu(name, header_response.right_clicked);

        if (context_response.remove_component) {
            UndoRedoCollection collection;
            for (const Entity& entity : menu_context.selected_entities) {
                const bool has_collection_2 = engine.ecs.has_component<ComponentCollection>(entity);
                if (has_collection_2 == false) continue;

                auto& component_collection_2 = engine.ecs.get_component<ComponentCollection>(entity);
                const bool has_component_2 = component_collection_2.has_component(component_index);
                if (has_component_2 == false) continue;

                RuntimeComponentDiff diff(entity, component_index);
                diff.before();
                component_collection_2.remove_component(component_index);
                diff.after();
                collection.add_action(diff);
            }
            collection.commit("Remove Component: " + std::string(name));
            continue;
        }

        if (context_response.copy_component) {
            const json serialized = tmt::Serializer::serialize(component_instance);
            json clipboard_json;
            clipboard_json["runtime"] = true;
            clipboard_json["component_type"] = name;
            clipboard_json["data"] = serialized;
            ImGui::SetClipboardText(clipboard_json.dump().c_str());
        }

        if (context_response.paste_component) {
            paste_component(name, menu_context);
        }

        if (context_response.paste_values) {
            paste_values(name, menu_context);
        }

        if (header_response.open == false) continue;

        const json before = tmt::Serializer::serialize(component_instance);
        const ImResponse response = ImReflect::Input("", component_instance);
        const bool changed = response.get<IGameComponent>().is_changed();

        if (changed == false) continue;
        if (menu_context.selected_entities.size() <= 1) continue;

        auto after = tmt::Serializer::serialize(component_instance);
        const json diff = nlohmann::json::diff(before, after);

        for (const Entity& entity : menu_context.selected_entities) {
            if (entity == menu_context.primary_entity) continue;

            const bool has_collection_2 = engine.ecs.has_component<ComponentCollection>(entity);
            if (has_collection_2 == false) continue;

            auto& other_component_collection = engine.ecs.get_component<ComponentCollection>(entity);
            const bool has_component_2 = other_component_collection.has_component(component_index);
            if (has_component_2 == false) continue;

            IGameComponent& other_instance = other_component_collection.get_component(component_index);
            json other_json = tmt::Serializer::serialize(other_instance);
            other_json.patch_inplace(diff);
            tmt::Serializer::deserialize(other_json, other_instance);
        }
    }
}

void Inspector::add_component(const MenuContext& menu_context) {
    if (menu_context.primary_entity == entt::null) return;

    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        ImGui::SeparatorText("Engine");
        add_compile_time_component(menu_context);

        ImGui::SeparatorText("Game");
        add_runtime_component(menu_context);

        ImGui::EndPopup();
    }
}

void Inspector::add_compile_time_component(const tmt::Inspector::MenuContext& menu_context) {
    InspectComponents::for_each([menu_context](auto type_tag) {
        using T = typename decltype(type_tag)::type;  // Extract type from tag
        const bool has_component = tmt::engine.ecs.has_component<T>(menu_context.primary_entity);
        if (has_component) return;

        const auto name = tmt::Component<T>::get_name();
        if (ImGui::MenuItem(name)) {
            UndoRedoCollection collection;
            for (const Entity& entity : menu_context.selected_entities) {
                ComponentDiff<T> component_diff(entity);
                component_diff.before();
                tmt::engine.ecs.add_or_get_component<T>(entity);
                component_diff.after();
                collection.add_action(component_diff);
            }
            collection.commit("Add Component: " + std::string(name));
            ImGui::CloseCurrentPopup();
        }
    });
}

void Inspector::add_runtime_component(const tmt::Inspector::MenuContext& menu_context) {
    const auto& registered_component = engine.component_registry.get_registered_components();
    for (const auto& [componend_index, component_info] : registered_component) {
        const bool has_collection = engine.ecs.has_component<ComponentCollection>(menu_context.primary_entity);
        bool has_component = false;
        if (has_collection) {
            const auto& component_collection = engine.ecs.get_component<ComponentCollection>(menu_context.primary_entity);
            has_component = component_collection.has_component(componend_index);
        }
        if (has_component) continue;
        if (ImGui::MenuItem(component_info.name.c_str())) {
            UndoRedoCollection collection;
            for (const Entity& entity : menu_context.selected_entities) {
                RuntimeComponentDiff diff(entity, componend_index);
                diff.before();
                auto& component_collection = engine.ecs.add_or_get_component<ComponentCollection>(entity);
                component_collection.add_component(componend_index, entity);
                diff.after();
                collection.add_action(diff);
            }
            collection.commit("Add Component: " + std::string(component_info.name));
            ImGui::CloseCurrentPopup();
        }
    }
}

Inspector::HeaderResponse Inspector::component_header(const std::string name) {
    const bool open = ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    const bool right_clicked = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    return { open, right_clicked };
}

Inspector::ContextMenuResponse Inspector::context_menu(const std::string& name, const bool open) {
    ContextMenuResponse response;

    const std::string popup_id = "ComponentOptionsPopup_" + std::string(name);
    if (open) {
        ImGui::OpenPopup(popup_id.c_str());
    }

    if (ImGui::BeginPopup(popup_id.c_str()) == false) return response;

    if (ImGui::MenuItem("Remove Component")) {
        response.remove_component = true;
        ImGui::CloseCurrentPopup();
    }

    if (ImGui::MenuItem("Copy Component")) {
        response.copy_component = true;
        ImGui::CloseCurrentPopup();
    }

    if (ImGui::BeginMenu("Paste")) {
        if (ImGui::MenuItem("Paste Component")) {
            response.paste_component = true;
            ImGui::CloseCurrentPopup();
        } else if (ImGui::MenuItem("Paste Values")) {
            response.paste_values = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
    return response;
}

void Inspector::paste_compile_time_component(const json& deserialized, const MenuContext& menu_context) {
    const auto name_in_clipboard = deserialized.value("component_type", "");
    SerializeComponents::for_each([&](auto type_tag_inner) {
        using T_inner = typename decltype(type_tag_inner)::type;  // Extract type from tag
        const auto name_of_type = tmt::Component<T_inner>::get_name();
        if (name_in_clipboard != name_of_type) return;

        UndoRedoCollection collection;
        const json data = deserialized.value("data", json::object());
        for (const Entity& entity : menu_context.selected_entities) {
            ComponentDiff<T_inner> component_diff(entity);
            component_diff.before();

            T_inner& target_instance = tmt::engine.ecs.add_or_get_component<T_inner>(entity);
            Serializer::deserialize(data, target_instance);

            component_diff.after();
            collection.add_action(component_diff);
        }
        collection.commit("Paste Component: " + std::string(name_of_type));
    });
}

void Inspector::paste_runtime_component(const tmt::json& deserialized, const tmt::Inspector::MenuContext& menu_context) {
    const auto name_in_clipboard = deserialized.value("component_type", "");
    const auto& registered_components = tmt::engine.component_registry.get_registered_components();
    for (const auto& [component_index, component_info] : registered_components) {
        if (name_in_clipboard != component_info.name) continue;

        UndoRedoCollection collection;
        const json data = deserialized.value("data", json::object());
        for (const Entity& entity : menu_context.selected_entities) {
            auto& component_collection = tmt::engine.ecs.add_or_get_component<ComponentCollection>(entity);
            RuntimeComponentDiff diff(entity, component_index);
            diff.before();

            IGameComponent& target_instance = component_collection.add_or_get_component(component_index, entity);
            Serializer::deserialize(data, target_instance);

            diff.after();
            collection.add_action(diff);
        }
        collection.commit("Paste Component: " + std::string(component_info.name));
    }
}

void Inspector::paste_component(const auto& name, const tmt::Inspector::MenuContext& menu_context) {
    const char* clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text == nullptr) {
        Log::warn("Failed to paste component, clipboard is empty.");
        return;
    }
    const json deserialized = json::parse(clipboard_text, nullptr, false);
    if (deserialized.is_discarded()) {
        Log::error("Failed to parse clipboard JSON for component '{}'. Clipboard is:", name, clipboard_text);
        return;
    }

    bool runtime = deserialized.value("runtime", false);

    if (runtime) {
        paste_runtime_component(deserialized, menu_context);
    } else {
        paste_compile_time_component(deserialized, menu_context);
    }
}

void Inspector::paste_runtime_values(const json& deserialized, const MenuContext& menu_context) {
    const auto name_in_clipboard = deserialized.value("component_type", "");
    const auto& registered_components = tmt::engine.component_registry.get_registered_components();
    for (const auto& [component_index, component_info] : registered_components) {
        if (name_in_clipboard != component_info.name) continue;

        UndoRedoCollection collection;
        const json data = deserialized.value("data", json::object());
        for (const Entity& entity : menu_context.selected_entities) {
            const bool has_collection = tmt::engine.ecs.has_component<ComponentCollection>(entity);
            if (has_collection == false) continue;

            auto& component_collection = tmt::engine.ecs.get_component<ComponentCollection>(entity);
            const bool has_component = component_collection.has_component(component_index);
            if (has_component == false) continue;

            IGameComponent& target_instance = component_collection.get_component(component_index);
            RuntimeComponentDiff diff(entity, component_index);
            diff.before();
            Serializer::deserialize(data, target_instance);
            diff.after();
            collection.add_action(diff);
        }
        collection.commit("Paste Values: " + std::string(component_info.name));
    }
}

void Inspector::paste_compile_time_values(const json& deserialized, const MenuContext& menu_context) {
    const auto name_in_clipboard = deserialized.value("component_type", "");
    SerializeComponents::for_each([&](auto type_tag_inner) {
        using T_inner = typename decltype(type_tag_inner)::type;  // Extract type from tag
        const auto name_of_type = tmt::Component<T_inner>::get_name();
        if (name_in_clipboard != name_of_type) return;

        UndoRedoCollection collection;
        const json data = deserialized.value("data", json::object());
        for (const Entity& entity : menu_context.selected_entities) {
            const bool has_comp = tmt::engine.ecs.has_component<T_inner>(entity);
            if (has_comp == false) continue;
            T_inner& target_instance = tmt::engine.ecs.get_component<T_inner>(entity);
            ComponentDiff<T_inner> component_diff(entity);
            component_diff.before();
            Serializer::deserialize(data, target_instance);
            component_diff.after();
            collection.add_action(component_diff);
        }
        collection.commit("Paste Values: " + std::string(name_of_type));
    });
}

void Inspector::paste_values(const auto& name, const tmt::Inspector::MenuContext& menu_context) {
    const char* clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text == nullptr) {
        Log::warn("Failed to paste values, clipboard is empty.");
        return;
    }

    const json deserialized = json::parse(clipboard_text, nullptr, false);
    if (deserialized.is_discarded()) {
        Log::error("Failed to parse clipboard JSON for component '{}'. Clipboard is:", name, clipboard_text);
        return;
    }

    const bool runtime = deserialized.value("runtime", false);
    if (runtime) {
        paste_runtime_values(deserialized, menu_context);
    } else {
        paste_compile_time_values(deserialized, menu_context);
    }
}

}  // namespace tmt
