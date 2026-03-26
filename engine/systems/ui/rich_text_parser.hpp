#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>

#include <glm/glm.hpp>

namespace tmt {

/* Supported rich text tag types */
enum class RichTextTagType : uint8_t {
    BOLD,           // <b> or </b>
    ITALIC,         // <i> or </i>
    UNDERLINE,      // <u> or </u>
    STRIKETHROUGH,  // <s> or </s>
    COLOR,          // <color=#RRGGBB> or <color=red> or </color>
    SIZE,           // <size=N> or </size>
    FONT,           // <font=name> or </font>
    LINE_BREAK,     // <br> or <br/>
    SPRITE,         // <sprite=N> for inline images
    LINK,           // <link=url> or </link>
};

/* A parsed rich text tag */
struct RichTextTag {
    RichTextTagType type {};
    bool is_closing = false;   // true if this is a closing tag (</...>)
    std::string value {};      // tag parameter value (e.g., "#FF0000" for color)
    size_t text_position = 0;  // position in the stripped text where this tag applies
};

/* Text segment with uniform styling */
struct TextSegment {
    std::string_view text {};  // view into the original text
    size_t start_index = 0;    // start index in original text
    size_t length = 0;         // length of this segment

    /* Styling */
    glm::vec4 color = glm::vec4(1.0f);  // RGBA color
    float size_scale = 1.0f;            // size multiplier relative to base size
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;

    /* For links */
    std::string link_url {};
};

/* Result of parsing rich text */
struct RichTextParseResult {
    std::string stripped_text {};          // text with all tags removed
    std::vector<TextSegment> segments {};  // styled text segments
    std::vector<RichTextTag> tags {};      // all parsed tags (for debugging)
    bool has_errors = false;               // true if parsing encountered errors
    std::string error_message {};          // description of first error
};

/*
 * Rich Text Parser
 *
 * Parses text containing HTML-like formatting tags and produces
 * a list of styled text segments.
 *
 * Supported tags:
 *   <b>, </b>               - Bold text
 *   <i>, </i>               - Italic text
 *   <u>, </u>               - Underlined text
 *   <s>, </s>               - Strikethrough text
 *   <color=#RRGGBB>, </color> - Colored text (hex color)
 *   <color=name>, </color>  - Named colors (red, green, blue, etc.)
 *   <size=N>, </size>       - Font size as percentage (e.g., 150 = 150%)
 *   <size=+N>, <size=-N>    - Relative size adjustment
 *   <br>, <br/>             - Line break
 *   <sprite=N>              - Inline sprite (index into sprite sheet)
 *
 * Example:
 *   "Hello <b>bold</b> and <color=#FF0000>red</color> world!"
 */
class RichTextParser {
   public:
    /* Parse rich text and return styled segments */
    static RichTextParseResult parse(const std::string& text, const glm::vec4& base_color = glm::vec4(1.0f));

    /* Strip all tags from text, returning plain text */
    static std::string strip_tags(const std::string& text);

    /* Check if text contains any rich text tags */
    static bool has_tags(const std::string& text);

    /* Named color lookup */
    static std::optional<glm::vec4> get_named_color(const std::string& name);

    /* Parse hex color string (#RRGGBB or #RRGGBBAA) */
    static std::optional<glm::vec4> parse_hex_color(const std::string& hex);

   private:
    /* Internal parsing state */
    struct ParseState {
        glm::vec4 color = glm::vec4(1.0f);
        float size_scale = 1.0f;
        bool bold = false;
        bool italic = false;
        bool underline = false;
        bool strikethrough = false;
        std::string link_url {};
    };

    /* Parse a single tag at the given position */
    static std::optional<RichTextTag> parse_tag(const std::string& text, size_t& pos);

    /* Apply a tag to the parse state (returns false if closing tag doesn't match) */
    static bool apply_tag(const RichTextTag& tag, ParseState& state, std::vector<ParseState>& state_stack);
};

}  // namespace tmt