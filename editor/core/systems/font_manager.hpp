#pragma once
#include "engine/core/io.hpp"
#include <unordered_map>
#include <queue>
#include <imgui.h>

struct ImFont;
struct ImFontConfig;

namespace tmt {

class FontManager {
   public:
    FontManager() = default;

    struct FontData {
        IO::FileLocation file_location;
        ImFont* im_font;
        float size;
        ImFontConfig config;
        uint16_t glyph_ranges[3];
        std::string name;
    };

    void init();
    void load(const std::string& name, const IO::FileLocation& location, const float size, const ImFontConfig& config = {}, const uint16_t* glyph_ranges = nullptr);
    void queue_reload() { pending_reload = true; };

    std::vector<FontData> m_fonts;
    bool pending_reload = false;

    void reload_fonts();
};

}  // namespace tmt