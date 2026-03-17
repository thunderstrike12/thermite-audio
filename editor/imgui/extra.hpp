#include <imgui.h>

inline void push_tooltip_style() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(16, 16, 16, 255));
}

inline void pop_tooltip_style() {
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

inline void tooltip(const char* text) {
    push_tooltip_style();

    if (ImGui::BeginItemTooltip()) {
        ImGui::Text("%s", text);

        ImGui::EndTooltip();
    }

    pop_tooltip_style();
}

inline void tooltip(const char* title, const char* desc, const float max_width = 16.0f) {
    push_tooltip_style();

    if (ImGui::BeginItemTooltip()) {
        /* Title */
        ImGui::Text("%s", title);

        /* Description */
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * max_width);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();

        ImGui::EndTooltip();
    }

    pop_tooltip_style();
}

inline void tooltip(const char* title, const char* desc, const char* shortcut, const float max_width = 16.0f) {
    push_tooltip_style();

    if (ImGui::BeginItemTooltip()) {
        /* Title */
        ImGui::Text("%s", title);

        /* Description */
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * max_width);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();

        /* Shortcut */
        ImGui::Dummy({ 0, 3 });
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(170, 170, 170, 255));
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * max_width);
        ImGui::Text("Shortcut: %s", shortcut);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        ImGui::EndTooltip();
    }

    pop_tooltip_style();
}
