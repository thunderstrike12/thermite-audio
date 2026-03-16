#pragma once
#include <imgui.h>
#include <imgui_internal.h>

namespace ImGui {

bool SquareIconButton(const char* label) {
    auto text_size = ImGui::CalcTextSize(label, nullptr, true);
    text_size.x = std::max(text_size.x, text_size.y);
    text_size.y = std::max(text_size.x, text_size.y);
    auto frame_padding = ImGui::GetStyle().FramePadding;
    frame_padding.x = std::max(frame_padding.x, frame_padding.y);
    frame_padding.y = std::max(frame_padding.x, frame_padding.y);
    const auto button_size = ImVec2(text_size.x + frame_padding.x * 2.0f, text_size.y + frame_padding.y * 2.0f);

    return ImGui::ButtonEx(label, button_size, ImGuiButtonFlags_AlignTextBaseLine);
}

}  // namespace ImGui