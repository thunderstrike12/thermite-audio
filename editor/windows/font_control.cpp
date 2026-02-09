#include "font_control.hpp"
#include <imgui.h>
#include "editor.hpp"
#include "editor/imgui/manager.hpp"
#include "editor/core/systems/font_manager.hpp"

void tmt::FontControl::on_editor_start() {}

void tmt::FontControl::on_editor_update(const tmt::FrameData&) {}

void tmt::FontControl::on_editor_end() {}

void tmt::FontControl::display() {
    auto& fonts = tmt::editor.imgui_manager.font_manager.m_fonts;

    if (ImGui::Button("Reload")) {
        tmt::editor.imgui_manager.font_manager.queue_reload();
    }

    if (ImGui::BeginTable("font_table", 2, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Font", ImGuiTableColumnFlags_WidthFixed, 300.f);
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        ImGuiStyle& style = ImGui::GetStyle();

        bool first = true;
        for (auto& font : fonts) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            const char* addition = font.config.MergeMode ? " (merged)" : "";
            float* size_addr = &font.size;
            if (first) {
                addition = " (main)";

                size_addr = &style.FontSizeBase;
            }

            ImGui::TextUnformatted((font.name + addition).c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::PushID(font.name.c_str());  // ensure unique ID per row

            if (ImGui::DragFloat("Size", size_addr, 0.20f, 5.0f, 100.0f, "%.0f")) {
                if (first)
                    style._NextFrameFontSizeBase = style.FontSizeBase;
                else
                    tmt::editor.imgui_manager.font_manager.queue_reload();  // implicitly reload
            }
            if (ImGui::DragFloat2("Glyph Offset (req. reload)", &font.config.GlyphOffset.x, 0.20f, -100.f, 100.0f, "%.0f")) {
                tmt::editor.imgui_manager.font_manager.queue_reload();  // implicitly reload
            }

            first = false;

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}