#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "engine/core/resources/font.hpp"
#include "engine/core/components/text_renderer.hpp"
#include "engine/systems/ui/rich_text_parser.hpp"

namespace tmt {

/* A single glyph instance ready for rendering */
struct GlyphInstance {
    glm::vec2 position {};    // Position relative to text origin
    glm::vec2 size {};        // Glyph size in pixels
    glm::vec4 uv_rect {};     // UV coordinates in atlas
    glm::vec4 color {};       // RGBA color
    uint32_t atlas_index {};  // Bindless texture index
    float size_scale = 1.0f;  // Scale factor from rich text
    float y_offset = 0.0f;    // Y offset from line top to glyph top (for wrap repositioning)
};

/* A line of text with its metrics */
struct TextLine {
    size_t start_index {};     // Index of first glyph in this line
    size_t glyph_count {};     // Number of glyphs in this line
    float width {};            // Total width of this line
    float height {};           // Height of this line
    float baseline_offset {};  // Y offset from line top to baseline
};

/* Result of text layout calculation */
struct TextLayoutResult {
    std::vector<GlyphInstance> glyphs {};  // All glyph instances
    std::vector<TextLine> lines {};        // Line information
    glm::vec2 bounds {};                   // Total text bounds
    glm::vec2 offset {};                   // Offset to apply for alignment
    bool truncated = false;                // True if text was truncated
};

/*
 * Text Layout System
 *
 * Calculates glyph positions for text rendering with support for:
 *   - Word wrapping
 *   - Horizontal and vertical alignment
 *   - Rich text formatting (size, color, bold, italic)
 *   - Line spacing and letter spacing
 *   - Text overflow handling
 */
class TextLayout {
   public:
    /*
     * Calculate glyph positions for a TextRenderer component.
     *
     * @param text_renderer The text renderer component with text and settings
     * @param font The font to use for layout (or default font if null)
     * @param container_size The size of the container (for alignment/wrapping)
     * @return Layout result with glyph instances and bounds
     */
    static TextLayoutResult layout(const TextRenderer& text_renderer, const Font* font, const glm::vec2& container_size);

    /*
     * Calculate glyph positions for plain text (no rich text parsing).
     *
     * @param text The text to layout
     * @param font The font to use
     * @param font_size Base font size in pixels
     * @param color Text color
     * @param max_width Maximum width before wrapping (0 = no limit)
     * @param word_wrap Enable word wrapping
     * @param h_align Horizontal alignment
     * @param v_align Vertical alignment
     * @param letter_spacing Extra spacing between characters
     * @param line_spacing Line height multiplier
     * @return Layout result with glyph instances and bounds
     */
    static TextLayoutResult layout_plain(
        const std::string& text, const Font& font, float font_size, const glm::vec4& color, float max_width = 0.0f, bool word_wrap = false, HorizontalAlign h_align = HorizontalAlign::LEFT,
        VerticalAlign v_align = VerticalAlign::TOP, float letter_spacing = 0.0f, float line_spacing = 1.0f
    );

    /*
     * Measure text bounds without generating glyph instances.
     * Faster than full layout when you only need dimensions.
     */
    static glm::vec2 measure(const std::string& text, const Font& font, float font_size, float max_width = 0.0f, bool word_wrap = false, float letter_spacing = 0.0f);

    /*
     * Calculate the optimal font size to fit text within the given bounds.
     * Uses binary search for efficiency.
     *
     * @param text_renderer The text renderer component (font_size is ignored)
     * @param font The font to use for measurement
     * @param container_size The target container size (width, height)
     * @return The calculated font size that makes text fit within bounds
     */
    static float calculate_auto_font_size(const TextRenderer& text_renderer, const Font* font, const glm::vec2& container_size);

   private:
    /* Internal layout state */
    struct LayoutState {
        const Font* font {};
        float base_font_size {};
        float letter_spacing {};
        float line_spacing {};
        float max_width {};
        bool word_wrap {};
        glm::vec4 base_color {};

        /* Current position */
        float cursor_x {};
        float cursor_y {};
        float line_width {};
        float line_height {};

        /* Current line tracking */
        size_t line_start_glyph {};
        size_t word_start_glyph {};
        float word_start_x {};

        /* SDF effect parameters */
        float outline_width {};
        glm::vec4 outline_color {};
        float softness {};
        float boldness {};
    };

    /* Process a single character and add glyph if needed */
    static void process_char(uint32_t codepoint, const TextSegment& segment, LayoutState& state, TextLayoutResult& result);

    /* Start a new line */
    static void new_line(LayoutState& state, TextLayoutResult& result);

    /* Break the current word (for word wrap) */
    static void break_word(LayoutState& state, TextLayoutResult& result);

    /* Apply horizontal alignment to a line */
    static void align_line(TextLine& line, const std::vector<GlyphInstance>& glyphs, HorizontalAlign h_align, float container_width);

    /* Apply vertical alignment to all lines */
    static void align_vertical(TextLayoutResult& result, VerticalAlign v_align, float container_height);

    /* Handle text overflow */
    static void handle_overflow(TextLayoutResult& result, const TextRenderer& text_renderer, float max_height);
};

}  // namespace tmt