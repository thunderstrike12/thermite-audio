#if THERMITE_EDITOR
// =============================================================================
// LayerMask ImGui Widget
// =============================================================================
// Generated with AI assistance
// First prompt: "I have a layer mask system and I want to expose it via imgui"
// Date: March 18, 2026
// Model: Claude Opus 4.6
// =============================================================================
    #include "layer_mask.hpp"
    #include "engine/systems/physics/physics_layers.hpp"
    #include "engine/systems/physics/physics_system.hpp"
    #include "projects/game/data_headers/layer_mask.hpp"

    #include "engine/engine.hpp"

void tag_invoke(ImReflect::ImInput_t, const char* name, game::LayerMask& mask, ImSettings& settings, ImResponse& response) {
    constexpr uint32_t count = tmt::MAX_LAYERS;
    constexpr uint32_t cols = 8;
    constexpr uint32_t rows = count / cols;
    constexpr float box_size = 14.0f;
    constexpr float spacing = 2.0f;
    constexpr float group_gap = 6.0f;  // extra gap every 8 columns

    ImGui::TextUnformatted(name);
    ImGui::SameLine();
    if (ImGui::SmallButton("All")) mask.bits = 0xFFFFFFFF;
    ImGui::SameLine();
    if (ImGui::SmallButton("None")) mask.bits = 0;
    ImGui::SameLine();

    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Reserve space for the entire grid
    float total_w = cols * (box_size + spacing) - spacing + group_gap;
    float total_h = rows * (box_size + spacing) - spacing;
    ImGui::InvisibleButton("##layer_grid", ImVec2(total_w, total_h));

    // Colors
    ImU32 col_on = ImGui::GetColorU32(ImGuiCol_CheckMark);
    ImU32 col_off = ImGui::GetColorU32(ImGuiCol_FrameBg);
    ImU32 col_border = ImGui::GetColorU32(ImGuiCol_Border);
    ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);

    ImVec2 mouse = ImGui::GetIO().MousePos;

    for (uint32_t r = 0; r < rows; ++r) {
        for (uint32_t c = 0; c < cols; ++c) {
            uint32_t bit = r * cols + c;
            if (bit >= count) break;

            // Position: add a group gap halfway through the columns
            float extra = (c >= cols / 2) ? group_gap : 0.0f;
            float x = origin.x + c * (box_size + spacing) + extra;
            float y = origin.y + r * (box_size + spacing);

            ImVec2 p0(x, y);
            ImVec2 p1(x + box_size, y + box_size);

            bool hovered = (mouse.x >= p0.x && mouse.x < p1.x && mouse.y >= p0.y && mouse.y < p1.y);
            bool on = mask.test(bit);

            // Click to toggle
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                mask.set(bit, !on);
                on = !on;
            }

            // Draw the box
            ImU32 fill = on ? col_on : (hovered ? col_hovered : col_off);
            draw->AddRectFilled(p0, p1, fill, 2.0f);
            draw->AddRect(p0, p1, col_border, 2.0f);

            // Tooltip on hover
            if (hovered) {
                ImGui::BeginTooltip();
                auto& layers = tmt::engine.ecs.systems.get<tmt::Physics>().layers();

                auto& ln = layers.get_layer_name(bit);
                if (!ln.empty())
                    ImGui::Text("[%u] %s: %s", bit, ln.c_str(), on ? "ON" : "OFF");
                else
                    ImGui::Text("Layer %u: %s", bit, on ? "ON" : "OFF");
                ImGui::EndTooltip();
            }
        }
    }
}
#endif
