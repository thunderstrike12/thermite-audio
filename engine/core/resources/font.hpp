#pragma once

#include <graphite/resources/handle.hh>

#include <unordered_map>

#include "engine/core/resource.hpp"

namespace tmt {

/* Information about a single glyph in the font atlas */
struct GlyphInfo {
    glm::vec4 uv_rect {};  // UV coordinates in atlas (min_u, min_v, max_u, max_v)
    glm::vec2 size {};     // Glyph size in pixels at base font size
    glm::vec2 bearing {};  // Offset from baseline to top-left of glyph
    float advance {};      // Horizontal advance to next character
    bool valid = false;    // Whether this glyph exists in the font
};

/* Font metrics for layout calculations */
struct FontMetrics {
    float ascender {};     // Distance from baseline to top of tallest glyph
    float descender {};    // Distance from baseline to bottom of lowest glyph (negative)
    float line_height {};  // Recommended line spacing
    float base_size {};    // Font size used to generate the atlas
};

/* Font Resource - Loads TTF/OTF fonts and generates SDF atlases */
class Font : public FileResource {
   public:
    Font(IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    /* Get glyph information for a character */
    const GlyphInfo& get_glyph(uint32_t codepoint) const;

    /* Get kerning adjustment between two characters */
    float get_kerning(uint32_t left, uint32_t right) const;

    /* Calculate the width of a string at a given font size */
    float measure_text(const std::string& text, float font_size) const;

    /* Get font metrics */
    const FontMetrics& get_metrics() const { return metrics; }

    /* Check if font is loaded and valid */
    bool is_valid() const { return valid; }

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".ttf", ".otf" };

    /* GPU resources */
    Texture atlas_texture {};
    Image atlas_image {};

    /* Atlas dimensions (read-only, set during generation) */
    int atlas_width = 0;
    int atlas_height = 0;

    /* ===== Atlas Generation Settings (editable in editor) ===== */

    /* Size of the atlas texture (width and height) */
    uint32_t atlas_size = 1024;

    /* Base font size for SDF generation (higher = more detail, but larger atlas) */
    float base_font_size = 64.0f;

    /* Padding around each glyph for SDF (higher = smoother edges at large sizes) */
    int sdf_padding = 8;

    /* SDF value at the glyph edge (0-255, typically 128) */
    float sdf_on_edge_value = 128.0f;

    /* Distance scale for SDF (higher = sharper falloff) */
    float sdf_pixel_dist_scale = 16.0f;

    /* First ASCII character to include */
    uint32_t first_char = 32;  // Space

    /* Last ASCII character to include */
    uint32_t last_char = 126;  // Tilde

   private:
    friend class FontManager;

    /* Load font from memory buffer (used for default font) */
    bool load_from_memory(const unsigned char* data, size_t size, const std::string& name);

    /* Generate SDF atlas from font data */
    bool generate_sdf_atlas(const unsigned char* font_data, size_t font_size);

    /* Generate MSDF atlas from font file */
    bool generate_msdf_atlas(const char* font_file);

    /* Font metrics */
    FontMetrics metrics {};

    /* Glyph data indexed by Unicode codepoint */
    std::unordered_map<uint32_t, GlyphInfo> glyphs {};

    /* Kerning pairs (key = (left << 16) | right) */
    std::unordered_map<uint32_t, float> kerning_pairs {};

    /* Fallback glyph for missing characters */
    GlyphInfo fallback_glyph {};

    /* Font validity flag */
    bool valid = false;

    /* Font name for debugging */
    std::string name = "Unknown Font";
};

/* Font manager for handling default font and font caching */
class FontManager {
   public:
    /* Initialize the font manager with default font */
    static void init();

    /* Shutdown and cleanup */
    static void shutdown();

    /* Get the default font */
    static std::shared_ptr<Font> get_default_font();

    /* Check if default font is available */
    static bool has_default_font();

   private:
    static std::shared_ptr<Font> default_font;
    static bool initialized;
};

}  // namespace tmt

TMT_OBJECT(tmt::Font, (atlas_size, base_font_size, sdf_padding, sdf_on_edge_value, sdf_pixel_dist_scale, first_char, last_char));