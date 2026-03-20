#include "pop_up.hpp"

#include "engine/core/logger.hpp"

#include <imgui.h>
#include "editor/font/icon_lookups.hpp"
#include "editor/imgui/tools/center.hpp"

namespace tmt {

static const ImVec4 severity_colors[] = {
    ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  // INFO
    ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // WARNING
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // ERROR
    ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // CRITICAL
};

static const char* severity_icons[] = {
    ICON_MS_INFO,       // INFO
    ICON_MS_WARNING,    // WARNING
    ICON_MS_ERROR,      // ERROR
    ICON_MS_DANGEROUS,  // CRITICAL
};

static const char* severity_names[] = {
    "Info",
    "Warning",
    "Error",
    "Critical",
};

void PopUpManager::update() {
    if (queue.empty()) return;

    const PopUp& first_pop_up = queue.front();

    ImGuiIO& io = ImGui::GetIO();
    /* Center pop-up */
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::OpenPopup(first_pop_up.title_text.c_str());

    auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    bool open = true;
    if (ImGui::BeginPopupModal(first_pop_up.title_text.c_str(), &open, flags)) {
        const auto color = severity_colors[static_cast<int>(first_pop_up.severity_level)];
        const auto icon = severity_icons[static_cast<int>(first_pop_up.severity_level)];

        ImGui::PushFont(NULL, ImGui::GetStyle().FontSizeBase * 2.0f);
        ImGui::TextColored(color, "%s", icon);
        ImGui::PopFont();

        ImGui::SameLine();
        ImGui::Text("%s", first_pop_up.message_text.c_str());

        {
            /* Code to center buttons, copied from Claudia */
            float total_buttons_width = 0.0f;
            const float button_spacing = ImGui::GetStyle().ItemSpacing.x;

            for (const auto& button : first_pop_up.buttons) {
                total_buttons_width += ImGui::CalcTextSize(button.label.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            }

            if (first_pop_up.buttons.size() > 1) total_buttons_width += button_spacing * (first_pop_up.buttons.size() - 1);

            float center_offset = (ImGui::GetContentRegionAvail().x - total_buttons_width) * 0.5f;
            if (center_offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + center_offset);

            for (const auto& button : first_pop_up.buttons) {
                if (ImGui::Button(button.label.c_str())) {
                    if (button.callback) button.callback();
                    if (button.closes_pop_up) open = false;
                }
                ImGui::SameLine();
            }
        }

        ImGui::EndPopup();
    }

    if (open == false) {
        ImGui::CloseCurrentPopup();
        queue.erase(queue.begin());
    }
}

void NotificationManager::update(const float delta_time) {
    if (queue.empty()) return;

    uint32_t index = 0;
    float offset = 0.0f;
    for (auto& notification : queue) {
        const auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoFocusOnAppearing | /*ImGuiWindowFlags_NoBringToFrontOnFocus |*/ ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
                           ImGuiWindowFlags_NoNav;

        const bool has_title = notification.title_text.empty() == false;
        const bool has_message = notification.message_text.empty() == false;
        if (has_title == false && has_message == false) {
            continue;
        }

        const ImVec2 padding(10.0f, 10.0f);
        const float window_spacing = 10.0f;
        const float notification_ratio = 6.0f;
        const float window_rounding = 10.0f;
        const float close_button_padding = 10.0f;
        const float title_margin = 20.0f;

        const auto color = severity_colors[static_cast<int>(notification.severity_level)];
        const auto icon = severity_icons[static_cast<int>(notification.severity_level)];
        const auto severity_name = severity_names[static_cast<int>(notification.severity_level)];

        const auto icon_text = icon;
        const auto title_text = has_title ? notification.title_text.c_str() : severity_name;
        const auto close_button_text = ICON_MS_CLOSE;

        const float icon_width = ImGui::CalcTextSize(icon_text).x;
        const float title_width = ImGui::CalcTextSize(title_text).x;
        const float button_width = ImGui::CalcTextSize(close_button_text).x + ImGui::GetStyle().FramePadding.x * 2;
        const float min_window_width = icon_width + title_width + title_margin + button_width + padding.x * 2 + close_button_padding;

        bool close = false;

        /* Position at the bottom right of the screen, with some padding and offset */
        ImGuiIO& io = ImGui::GetIO();
        const ImVec2 window_size = io.DisplaySize;
        const ImVec2 window_pos = ImVec2(window_size.x - padding.x, window_size.y - padding.y - offset);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));

        /* min width */
        ImGui::SetNextWindowSizeConstraints(ImVec2(min_window_width, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

        /* Rounded corners */
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, window_rounding);

        const std::string window_name = fmt::format("{}##{}", notification.title_text, index);
        ImGui::Begin(window_name.c_str(), nullptr, flags);

        ImGui::PushTextWrapPos(window_size.x / notification_ratio);

        ImGui::TextColored(color, "%s", icon_text);
        ImGui::SameLine();
        ImGui::Text("%s", title_text);

        const float window_width = ImGui::GetWindowWidth();
        const float button_pos = window_width - button_width - close_button_padding;
        ImGui::SameLine();
        ImGui::SetCursorPosX(button_pos);
        if (ImGui::SmallButton(close_button_text)) {
            close = true;
        }

        if (has_message) {
            ImGui::Separator();

            ImGui::TextWrapped("%s", notification.message_text.c_str());
        }

        offset += ImGui::GetWindowHeight() + window_spacing;

        ImGui::PopTextWrapPos();
        ImGui::End();
        ImGui::PopStyleVar();

        notification.duration_seconds -= delta_time;
        close |= (notification.duration_seconds <= 0.0f);
        if (close) {
            queue.erase(queue.begin() + index);
            --index;
        }

        index++;
    }
}

}  // namespace tmt