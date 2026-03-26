#include "editor/core/systems/font_manager.hpp"
#include <imgui.h>
#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"

void tmt::EditorFontManager::init() {
    /* Rubrik */
    load("Rubik", { IO::Location::EDITOR, "fonts/Rubik-Regular.ttf" }, 18.f);

    /* Icons */
    const uint16_t glyph_ranges[] = { ICON_MIN_MS, ICON_MAX_MS, 0 };
    ImFontConfig config {};
    config.MergeMode = true;
    config.GlyphOffset.y = 4.5f;
    load("MaterialSymbols", { IO::Location::EDITOR, "fonts/MaterialSymbolsRounded.ttf" }, 24.f, config, glyph_ranges);
}

void tmt::EditorFontManager::load(const std::string& name, const IO::FileLocation& location, const float size, const ImFontConfig& config, const uint16_t* glyph_ranges) {
    auto& io = ImGui::GetIO();

    const auto& full_path = location.get_absolute_path();
    ImFont* font = io.Fonts->AddFontFromFileTTF(full_path.string().c_str(), size, &config, glyph_ranges);
    if (!font) {
        Log::error(Log::Scope::ENGINE, "Failed to load font from file, {}", full_path.string());
        return;
    }

    FontData data {};
    data.im_font = font;
    data.size = size;
    data.file_location = location;
    data.config = config;
    data.name = name;

    if (glyph_ranges) {
        for (uint8_t i = 0; i < 3; i++) {
            data.glyph_ranges[i] = glyph_ranges[i];
        }
    }
    m_fonts.push_back(data);
}

void tmt::EditorFontManager::reload_fonts() {
    Log::info(Log::Scope::ENGINE, "Reloading fonts...");

    auto& io = ImGui::GetIO();
    io.Fonts->Clear();

    std::vector<FontData> temp_copy = m_fonts;
    m_fonts.clear();
    for (auto& data : temp_copy) {
        load(data.name, data.file_location, data.size, data.config, data.glyph_ranges);
    }

    pending_reload = false;
}
