#include <imgui.h>

inline void tooltip(const char* text) {
    if (!ImGui::BeginItemTooltip()) return;

    ImGui::Text("%s", text);

    ImGui::EndTooltip();
}

inline void tooltip(const char* title, const char* desc, const float max_width = 16.0f) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

    if (ImGui::BeginItemTooltip()) {
        /* Title */
        ImGui::Text("%s", title);

        /* Description */
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, 255));
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * max_width);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        ImGui::EndTooltip();
    }

    ImGui::PopStyleVar();
}
