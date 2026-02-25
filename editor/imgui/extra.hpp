#include <imgui.h>

inline void tooltip(const char* text) {
    if (!ImGui::BeginItemTooltip()) return;

    ImGui::Text("%s", text);

    ImGui::EndTooltip();
}

inline void tooltip(const char* title, const char* desc) {
    if (!ImGui::BeginItemTooltip()) return;

    ImGui::Text("%s", title);

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(155, 155, 155, 255));
    ImGui::Text("%s", desc);
    ImGui::PopStyleColor();

    ImGui::EndTooltip();
}
