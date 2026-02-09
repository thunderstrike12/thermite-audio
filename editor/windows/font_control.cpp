#include "font_control.hpp"
#include <imgui.h>
#include "editor.hpp"
#include "editor/imgui/manager.hpp"
#include "editor/core/systems/font_manager.hpp"

void tmt::FontControl::on_editor_start() {
    // if we have valid data saved, set the values
    const auto& font_size = editor.save_data.font_size;

    auto& fonts = editor.imgui_manager.font_manager.m_fonts;
    assert(fonts.size() > 1);

    auto& text_font = fonts[0];
    auto& icon_font = fonts[1];

    if (font_size.text_size > 0.0f) {
        text_font.size = font_size.text_size;
    }
    if (font_size.icon_size > 0.0f) {
        icon_font.size = font_size.icon_size;
    }
    editor.imgui_manager.font_manager.queue_reload();
}

void tmt::FontControl::on_editor_update(const FrameData&) {}

void tmt::FontControl::on_editor_end() {}

void tmt::FontControl::display() {
    auto& fonts = editor.imgui_manager.font_manager.m_fonts;

    if (ImGui::Button("Reload")) {
        editor.imgui_manager.font_manager.queue_reload();
    }

    if (ImGui::BeginTable("font_table", 2, ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Font", ImGuiTableColumnFlags_WidthFixed, 300.f);
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        ImGuiStyle& style = ImGui::GetStyle();

        bool first = true;
        for (size_t i = 0; i < fonts.size(); i++) {
            auto& font = fonts[i];
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
                if (first) {
                    style._NextFrameFontSizeBase = style.FontSizeBase;
                    // save text since it is first
                    editor.save_data.font_size.text_size = style.FontSizeBase;
                } else {
                    tmt::editor.imgui_manager.font_manager.queue_reload();
                    // save icon as second
                    editor.save_data.font_size.icon_size = font.size;

                }  // implicitly reload
            }
            if (ImGui::DragFloat2("Glyph Offset (req. reload)", &font.config.GlyphOffset.x, 0.20f, -100.f, 100.0f, "%.0f")) {
                editor.imgui_manager.font_manager.queue_reload();  // implicitly reload
            }

            first = false;

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}
