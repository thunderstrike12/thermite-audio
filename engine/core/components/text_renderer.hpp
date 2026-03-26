#pragma once

#include <string>

#include "engine/core/resources/font.hpp"
#include "engine/tools/types/color.hpp"

namespace tmt {

/* Horizontal text alignment */
enum class HorizontalAlign : uint8_t {
    LEFT,   /* Align to left edge */
    CENTER, /* Center horizontally */
    RIGHT   /* Align to right edge */
};

/* Vertical text alignment */
enum class VerticalAlign : uint8_t {
    TOP,    /* Align to top edge */
    MIDDLE, /* Center vertically */
    BOTTOM  /* Align to bottom edge */
};

/* Text overflow behavior */
enum class TextOverflow : uint8_t {
    VISIBLE,  /* Text extends beyond bounds */
    CLIP,     /* Clip text at bounds */
    ELLIPSIS, /* Add "..." when text exceeds bounds */
    WRAP      /* Wrap to next line (same as word_wrap=true) */
};

/* Font style for rich text */
enum class FontStyle : uint8_t { NORMAL = 0, BOLD = 1 << 0, ITALIC = 1 << 1, BOLD_ITALIC = BOLD | ITALIC };

struct TextRenderer {
    /* Text content (supports rich text tags) */
    std::string text {};

    /* Font resource, if null, uses default font */
    ResourceRef<Font> font {};

    /* Base font size in pixels */
    float font_size = 16.0f;

    /* Text color (Rec.709) */
    RGBA color = RGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    /* Layout properties */
    float line_spacing = 1.0f;       // Multiplier for line height
    float letter_spacing = 0.0f;     // Extra spacing between characters
    float paragraph_spacing = 0.0f;  // Extra spacing after line breaks

    /* Alignment */
    HorizontalAlign horizontal_align = HorizontalAlign::LEFT;
    VerticalAlign vertical_align = VerticalAlign::TOP;

    /* Word wrapping and overflow */
    bool word_wrap = false;   // Enable word wrapping
    float max_width = 0.0f;   // Max width before wrapping (0 = unlimited)
    float max_height = 0.0f;  // Max height for text area (0 = unlimited)
    TextOverflow overflow = TextOverflow::VISIBLE;

    /* When enabled, max_width and max_height are automatically set from the
     * attached UIComponent's size every frame. This allows the text bounds
     * to follow the UI element's size. */
    bool use_ui_component_size = true;

    /* When enabled, the font size is automatically calculated to make the text
     * fit exactly within max_width and max_height. Requires both max_width and
     * max_height to be set (or use_ui_component_size to be enabled).
     * This will override the font_size value during layout. */
    bool auto_font_size = false;

    /* Rich text parsing */
    bool rich_text_enabled = true;

    /* Constructors */
    TextRenderer() = default;
    TextRenderer(std::string text) : text(std::move(text)) {}
    TextRenderer(std::string text, ResourceRef<Font> font) : text(std::move(text)), font(std::move(font)) {}
    ~TextRenderer() = default;
};

}  // namespace tmt

TMT_COMPONENT(
    tmt::TextRenderer, "TextRenderer",
    (text, font, font_size, color, line_spacing, letter_spacing, paragraph_spacing, horizontal_align, vertical_align, word_wrap, max_width, max_height, overflow, use_ui_component_size,
     auto_font_size, rich_text_enabled)
);
