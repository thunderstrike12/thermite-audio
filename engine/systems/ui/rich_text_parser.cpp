#include "rich_text_parser.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace tmt {

/* Named colors lookup table */
static const std::unordered_map<std::string, glm::vec4> NAMED_COLORS = {
    { "white", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },        { "black", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) },       { "red", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
    { "green", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },        { "blue", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) },        { "yellow", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) },
    { "cyan", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) },         { "magenta", glm::vec4(1.0f, 0.0f, 1.0f, 1.0f) },     { "orange", glm::vec4(1.0f, 0.65f, 0.0f, 1.0f) },
    { "purple", glm::vec4(0.5f, 0.0f, 0.5f, 1.0f) },       { "pink", glm::vec4(1.0f, 0.75f, 0.8f, 1.0f) },       { "brown", glm::vec4(0.65f, 0.16f, 0.16f, 1.0f) },
    { "gray", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) },         { "grey", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) },        { "lightgray", glm::vec4(0.75f, 0.75f, 0.75f, 1.0f) },
    { "lightgrey", glm::vec4(0.75f, 0.75f, 0.75f, 1.0f) }, { "darkgray", glm::vec4(0.25f, 0.25f, 0.25f, 1.0f) }, { "darkgrey", glm::vec4(0.25f, 0.25f, 0.25f, 1.0f) },
    { "transparent", glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) },
};

/* Helper to convert string to lowercase */
static std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/* Helper to trim whitespace */
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::optional<glm::vec4> RichTextParser::get_named_color(const std::string& name) {
    std::string lower_name = to_lower(trim(name));
    auto it = NAMED_COLORS.find(lower_name);
    if (it != NAMED_COLORS.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<glm::vec4> RichTextParser::parse_hex_color(const std::string& hex) {
    std::string color = hex;

    /* Remove # prefix if present */
    if (!color.empty() && color[0] == '#') {
        color = color.substr(1);
    }

    /* Validate hex string */
    if (color.length() != 6 && color.length() != 8) {
        return std::nullopt;
    }

    for (char c : color) {
        if (!std::isxdigit(c)) {
            return std::nullopt;
        }
    }

    /* Parse color components */
    unsigned int r, g, b, a = 255;
    std::stringstream ss;

    ss << std::hex << color.substr(0, 2);
    ss >> r;
    ss.clear();

    ss << std::hex << color.substr(2, 2);
    ss >> g;
    ss.clear();

    ss << std::hex << color.substr(4, 2);
    ss >> b;

    if (color.length() == 8) {
        ss.clear();
        ss << std::hex << color.substr(6, 2);
        ss >> a;
    }

    return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

bool RichTextParser::has_tags(const std::string& text) {
    return text.find('<') != std::string::npos;
}

std::string RichTextParser::strip_tags(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '<') {
            /* Find closing bracket */
            size_t end = text.find('>', i);
            if (end != std::string::npos) {
                /* Check if this looks like a tag */
                std::string tag_content = text.substr(i + 1, end - i - 1);
                bool is_tag = false;

                /* Basic validation - tag should start with letter or / */
                if (!tag_content.empty()) {
                    char first = tag_content[0];
                    if (first == '/' || std::isalpha(first)) {
                        is_tag = true;
                    }
                }

                if (is_tag) {
                    /* Handle <br> as newline */
                    std::string lower_tag = to_lower(tag_content);
                    if (lower_tag == "br" || lower_tag == "br/") {
                        result += '\n';
                    }
                    i = end + 1;
                    continue;
                }
            }
        }
        result += text[i];
        ++i;
    }

    return result;
}

std::optional<RichTextTag> RichTextParser::parse_tag(const std::string& text, size_t& pos) {
    if (pos >= text.size() || text[pos] != '<') {
        return std::nullopt;
    }

    /* Find closing bracket */
    size_t end = text.find('>', pos);
    if (end == std::string::npos) {
        return std::nullopt;
    }

    std::string tag_content = text.substr(pos + 1, end - pos - 1);
    if (tag_content.empty()) {
        return std::nullopt;
    }

    RichTextTag tag;
    tag.text_position = pos;

    /* Check for closing tag */
    size_t name_start = 0;
    if (tag_content[0] == '/') {
        tag.is_closing = true;
        name_start = 1;
    }

    /* Extract tag name and value */
    size_t equals_pos = tag_content.find('=', name_start);
    std::string tag_name;

    if (equals_pos != std::string::npos) {
        tag_name = to_lower(trim(tag_content.substr(name_start, equals_pos - name_start)));
        tag.value = trim(tag_content.substr(equals_pos + 1));

        /* Remove quotes from value if present */
        if (tag.value.size() >= 2) {
            if ((tag.value.front() == '"' && tag.value.back() == '"') || (tag.value.front() == '\'' && tag.value.back() == '\'')) {
                tag.value = tag.value.substr(1, tag.value.size() - 2);
            }
        }
    } else {
        tag_name = to_lower(trim(tag_content.substr(name_start)));

        /* Handle self-closing br */
        if (tag_name.size() >= 2 && tag_name.back() == '/') {
            tag_name = tag_name.substr(0, tag_name.size() - 1);
        }
    }

    /* Map tag name to type */
    if (tag_name == "b" || tag_name == "bold") {
        tag.type = RichTextTagType::BOLD;
    } else if (tag_name == "i" || tag_name == "italic") {
        tag.type = RichTextTagType::ITALIC;
    } else if (tag_name == "u" || tag_name == "underline") {
        tag.type = RichTextTagType::UNDERLINE;
    } else if (tag_name == "s" || tag_name == "strike" || tag_name == "strikethrough") {
        tag.type = RichTextTagType::STRIKETHROUGH;
    } else if (tag_name == "color" || tag_name == "c") {
        tag.type = RichTextTagType::COLOR;
    } else if (tag_name == "size") {
        tag.type = RichTextTagType::SIZE;
    } else if (tag_name == "font") {
        tag.type = RichTextTagType::FONT;
    } else if (tag_name == "br") {
        tag.type = RichTextTagType::LINE_BREAK;
        tag.is_closing = false;  // br is never a closing tag
    } else if (tag_name == "sprite" || tag_name == "img") {
        tag.type = RichTextTagType::SPRITE;
    } else if (tag_name == "link" || tag_name == "a") {
        tag.type = RichTextTagType::LINK;
    } else {
        /* Unknown tag - skip it */
        pos = end + 1;
        return std::nullopt;
    }

    pos = end + 1;
    return tag;
}

bool RichTextParser::apply_tag(const RichTextTag& tag, ParseState& state, std::vector<ParseState>& state_stack) {
    if (tag.is_closing) {
        /* Restore previous state from stack if available */
        if (!state_stack.empty()) {
            state = state_stack.back();
            state_stack.pop_back();
        }
        return true;
    }

    /* Save current state before applying tag */
    state_stack.push_back(state);

    switch (tag.type) {
        case RichTextTagType::BOLD:
            state.bold = true;
            break;

        case RichTextTagType::ITALIC:
            state.italic = true;
            break;

        case RichTextTagType::UNDERLINE:
            state.underline = true;
            break;

        case RichTextTagType::STRIKETHROUGH:
            state.strikethrough = true;
            break;

        case RichTextTagType::COLOR: {
            /* Try hex color first, then named color */
            auto hex_color = parse_hex_color(tag.value);
            if (hex_color) {
                state.color = *hex_color;
            } else {
                auto named_color = get_named_color(tag.value);
                if (named_color) {
                    state.color = *named_color;
                }
            }
            break;
        }

        case RichTextTagType::SIZE: {
            /* Parse size value */
            if (!tag.value.empty()) {
                try {
                    float value = std::stof(tag.value);

                    /* Handle relative sizes (+N or -N) */
                    if (tag.value[0] == '+' || tag.value[0] == '-') {
                        state.size_scale += value / 100.0f;
                    } else {
                        /* Absolute size as percentage */
                        state.size_scale = value / 100.0f;
                    }

                    /* Clamp to reasonable range */
                    state.size_scale = std::clamp(state.size_scale, 0.1f, 10.0f);
                } catch (...) {
                    /* Invalid number, ignore */
                }
            }
            break;
        }

        case RichTextTagType::LINK:
            state.link_url = tag.value;
            break;

        case RichTextTagType::LINE_BREAK:
        case RichTextTagType::SPRITE:
        case RichTextTagType::FONT:
            /* These don't modify state */
            break;
    }

    return true;
}

RichTextParseResult RichTextParser::parse(const std::string& text, const glm::vec4& base_color) {
    RichTextParseResult result;
    result.stripped_text.reserve(text.size());

    ParseState current_state;
    current_state.color = base_color;

    std::vector<ParseState> state_stack;

    /* Track current segment */
    size_t segment_start = 0;
    ParseState segment_state = current_state;

    size_t pos = 0;
    while (pos < text.size()) {
        if (text[pos] == '<') {
            /* Try to parse a tag */
            size_t tag_start = pos;
            auto tag = parse_tag(text, pos);

            if (tag) {
                /* End current segment if we have text */
                if (result.stripped_text.size() > segment_start) {
                    TextSegment segment;
                    segment.start_index = segment_start;
                    segment.length = result.stripped_text.size() - segment_start;
                    segment.text = std::string_view(result.stripped_text).substr(segment_start, segment.length);
                    segment.color = segment_state.color;
                    segment.size_scale = segment_state.size_scale;
                    segment.bold = segment_state.bold;
                    segment.italic = segment_state.italic;
                    segment.underline = segment_state.underline;
                    segment.strikethrough = segment_state.strikethrough;
                    segment.link_url = segment_state.link_url;
                    result.segments.push_back(std::move(segment));
                }

                /* Handle line break specially */
                if (tag->type == RichTextTagType::LINE_BREAK) {
                    result.stripped_text += '\n';
                }

                /* Apply tag to state */
                tag->text_position = result.stripped_text.size();
                apply_tag(*tag, current_state, state_stack);
                result.tags.push_back(*tag);

                /* Start new segment */
                segment_start = result.stripped_text.size();
                segment_state = current_state;
            } else {
                /* Not a valid tag, treat as text */
                result.stripped_text += text[tag_start];
                pos = tag_start + 1;
            }
        } else {
            result.stripped_text += text[pos];
            ++pos;
        }
    }

    /* Add final segment */
    if (result.stripped_text.size() > segment_start) {
        TextSegment segment;
        segment.start_index = segment_start;
        segment.length = result.stripped_text.size() - segment_start;
        segment.text = std::string_view(result.stripped_text).substr(segment_start, segment.length);
        segment.color = segment_state.color;
        segment.size_scale = segment_state.size_scale;
        segment.bold = segment_state.bold;
        segment.italic = segment_state.italic;
        segment.underline = segment_state.underline;
        segment.strikethrough = segment_state.strikethrough;
        segment.link_url = segment_state.link_url;
        result.segments.push_back(std::move(segment));
    }

    /* If no segments (empty text or only tags), add empty segment */
    if (result.segments.empty()) {
        TextSegment segment;
        segment.start_index = 0;
        segment.length = 0;
        segment.text = std::string_view();
        segment.color = base_color;
        segment.size_scale = 1.0f;
        result.segments.push_back(segment);
    }

    return result;
}

}  // namespace tmt