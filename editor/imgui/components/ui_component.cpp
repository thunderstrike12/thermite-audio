#include "ui_component.hpp"
#include <ImReflect.hpp>
#include "editor/imgui/types/glm.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/core/components/image_renderer.hpp"

/* clang-format off */

struct Known { float r; glm::vec2 aspect; };
static constexpr Known table[] = {
    { 16.0f / 9.0f,  {16, 9}  },
    { 4.0f  / 3.0f,  {4,  3}  },
    { 21.0f / 9.0f,  {21, 9}  },
    { 1.0f,          {1,  1}  },
    { 3.0f  / 2.0f,  {3,  2}  },
    { 16.0f / 10.0f, {16, 10} },
};

/* clang-format on */

void tag_invoke(ImReflect::ImInput_t, const char*, tmt::UIComponent& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::UIComponent>();
    auto& type_response = response.get<tmt::UIComponent>();

    const tmt::Entity entity = tmt::engine.ecs.get_entity(value);
    auto image = tmt::engine.ecs.try_get_component<tmt::ImageRenderer>(entity);

    /* If ImageRenderer is present, then set the UI Component Size to Image Size */
    if (value.synced_from_image != true) {
        if (image != nullptr && image->texture != nullptr) {
            const float w = (float)image->texture->width;
            const float h = (float)image->texture->height;

            value.aspect_ratio = { w, h };

            value.synced_from_image = true;
        }
    }

    ImReflect::Input("Size", value.size, type_settings, type_response);
    {
        bool changed = ImGui::SliderFloat2("Pivot", &value.pivot.x, 0.0f, 1.0f);
        if (changed) {
            type_response.changed();
        }
        ImReflect::Detail::check_input_states(type_response);
    }

    ImReflect::Input("Block Raycasts", value.block_raycasts, type_settings, type_response);
    ImReflect::Input("Fill X", value.scale_with_parent_x, type_settings, type_response);
    ImGui::SameLine();
    ImReflect::Input("Fill Y", value.scale_with_parent_y, type_settings, type_response);
    ImReflect::Input("Aspect Mode", value.aspect_mode, type_settings, type_response);
    ImReflect::Input("Aspect Ratio", value.aspect_ratio, type_settings, type_response);
    ImResponse anchor_response = ImReflect::Input("Anchor", value.anchor);

    // Uniform resize: one slider, both X and Y follow the aspect ratio
    if (value.aspect_ratio.x > 0.0f && value.aspect_ratio.y > 0.0f) {
        // Pick the larger component as the reference so scale stays stable
        const bool use_x = value.aspect_ratio.x >= value.aspect_ratio.y;
        float scale = use_x ? value.size.x / value.aspect_ratio.x : value.size.y / value.aspect_ratio.y;

        if (ImGui::DragFloat("Uniform Size", &scale, 1.0f, 0.0f, FLT_MAX, "%.1f")) {
            value.size.x = value.aspect_ratio.x * scale;
            value.size.y = value.aspect_ratio.y * scale;
            type_response.changed();
        }
    }

    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    if (anchor_response.get<tmt::Anchor>().is_changed()) {
        switch (value.anchor) {
            case tmt::Anchor::TOP_LEFT:
                value.pivot = { 0.0f, 0.0f };
                break;
            case tmt::Anchor::TOP_CENTER:
                value.pivot = { 0.5f, 0.0f };
                break;
            case tmt::Anchor::TOP_RIGHT:
                value.pivot = { 1.0f, 0.0f };
                break;
            case tmt::Anchor::MIDDLE_LEFT:
                value.pivot = { 0.0f, 0.5f };
                break;
            case tmt::Anchor::MIDDLE_CENTER:
                value.pivot = { 0.5f, 0.5f };
                break;
            case tmt::Anchor::MIDDLE_RIGHT:
                value.pivot = { 1.0f, 0.5f };
                break;
            case tmt::Anchor::BOTTOM_LEFT:
                value.pivot = { 0.0f, 1.0f };
                break;
            case tmt::Anchor::BOTTOM_CENTER:
                value.pivot = { 0.5f, 1.0f };
                break;
            case tmt::Anchor::BOTTOM_RIGHT:
                value.pivot = { 1.0f, 1.0f };
                break;
        }

        glm::vec3 position = transform.get_local_position();
        transform.set_local_position(glm::vec3(0.0f, 0.0f, position.z));
    }
}
