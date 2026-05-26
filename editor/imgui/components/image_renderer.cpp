#include "image_renderer.hpp"

#include "editor/imgui/types/glm.hpp"

#include "engine/engine.hpp"

#include "engine/core/ecs.hpp"
#include "engine/core/components/image_renderer.hpp"

#include "editor/imgui/types/resource_ref.hpp"
#include "editor/imgui/types/color.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* name, tmt::ImageRenderer& value, ImSettings& settings, ImResponse& response) {
    ImReflect::Detail::imgui_input_visit_field(name, value, settings, response);

    bool needs_reset = false;

    needs_reset |= ImGui::Checkbox("Continuous", &value.continuous_anim);
    needs_reset |= ImGui::DragFloat("Anim Speed", &value.anim_speed, 0.1f, 0.0f, 60.0f);

    if (!value.continuous_anim) {
        int loops = (int)value.loop_count;
        if (ImGui::DragInt("Loop Count", &loops, 1.0f, 1, 1000)) {
            value.loop_count = (uint32_t)std::max(1, loops);
            needs_reset = true;
        }
    }

    if (needs_reset) {
        value.accumulated_time = 0.0f;
        value.current_frame = 0;
        value.completed_loops = 0;
        value.finished = false;
    }
}
