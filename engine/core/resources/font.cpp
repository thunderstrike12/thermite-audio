#include "font.hpp"

#include <fstream>
#include <cmath>
#include <algorithm>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#pragma warning(disable : 4458)
#pragma warning(disable : 4508)
#pragma warning(disable : 4505)
#include <msdf-atlas-gen/msdf-atlas-gen.h>

#include <graphite/vram_bank.hh>

#include "engine.hpp"
#include "core/logger.hpp"
#include "core/renderer/renderer.hpp"

namespace tmt {

/* Static members */
std::shared_ptr<Font> FontManager::default_font = nullptr;
bool FontManager::initialized = false;

bool Font::load() {
    /* Read the font file */
    std::ifstream file(file_location.get_absolute_path(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Log::error(Log::Scope::ENGINE, "Failed to open font file: {}", file_location.get_absolute_path().string());
        return false;
    }

    const size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> font_data(file_size);
    if (!file.read(reinterpret_cast<char*>(font_data.data()), file_size)) {
        Log::error(Log::Scope::ENGINE, "Failed to read font file: {}", file_location.get_absolute_path().string());
        return false;
    }
    file.close();

    name = file_location.get_relative_path().stem().string();

    // return generate_sdf_atlas(font_data.data(), file_size);
    return generate_msdf_atlas(file_location.get_absolute_path().string().c_str());
}

bool Font::load_from_memory(const unsigned char* data, size_t size, const std::string& font_name) {
    name = font_name;
    return generate_sdf_atlas(data, size);
}

bool Font::generate_sdf_atlas(const unsigned char* font_data, size_t) {
    /* Initialize stb_truetype */
    stbtt_fontinfo font_info;
    if (!stbtt_InitFont(&font_info, font_data, stbtt_GetFontOffsetForIndex(font_data, 0))) {
        Log::error(Log::Scope::ENGINE, "Failed to initialize font: {}", name);
        return false;
    }

    /* Get font scale for base size */
    const float scale = stbtt_ScaleForPixelHeight(&font_info, base_font_size);

    /* Get font metrics */
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

    /* Include SDF padding in ascender/descender since glyph bearings include padding.
     * This ensures consistent positioning: when bearing.y == ascender, the glyph
     * top aligns exactly with the line top (cursor_y). */
    metrics.ascender = ascent * scale + sdf_padding;
    metrics.descender = descent * scale - sdf_padding;
    metrics.line_height = (ascent - descent + line_gap) * scale;
    metrics.base_size = base_font_size;

    /* Calculate atlas layout using simple row packing */
    const uint32_t num_chars = last_char - first_char + 1;
    const uint32_t glyph_padding = sdf_padding * 2;

    /* First pass: calculate glyph sizes */
    struct GlyphRect {
        uint32_t codepoint;
        int width, height;
        int x0, y0, x1, y1;
        int advance, lsb;
    };
    std::vector<GlyphRect> glyph_rects;
    glyph_rects.reserve(num_chars);

    for (uint32_t c = first_char; c <= last_char; ++c) {
        GlyphRect rect {};
        rect.codepoint = c;

        int glyph_index = stbtt_FindGlyphIndex(&font_info, c);
        if (glyph_index == 0 && c != ' ') {
            continue;  // Skip missing glyphs (except space)
        }

        stbtt_GetGlyphHMetrics(&font_info, glyph_index, &rect.advance, &rect.lsb);
        stbtt_GetGlyphBitmapBox(&font_info, glyph_index, scale, scale, &rect.x0, &rect.y0, &rect.x1, &rect.y1);

        rect.width = rect.x1 - rect.x0 + glyph_padding;
        rect.height = rect.y1 - rect.y0 + glyph_padding;

        glyph_rects.push_back(rect);
    }

    /* Sort glyphs by height (descending) for better packing */
    std::sort(glyph_rects.begin(), glyph_rects.end(), [](const GlyphRect& a, const GlyphRect& b) { return a.height > b.height; });

    /* Pack glyphs into atlas using simple row-based algorithm */
    atlas_width = atlas_size;
    atlas_height = atlas_size;

    std::vector<uint8_t> atlas_data(atlas_width * atlas_height, 0);

    int current_x = 0;
    int current_y = 0;
    int row_height = 0;

    for (auto& rect : glyph_rects) {
        if (rect.width == 0 || rect.height == 0) {
            /* Space or empty glyph */
            GlyphInfo& glyph = glyphs[rect.codepoint];
            glyph.uv_rect = glm::vec4(0);
            glyph.size = glm::vec2(0);
            glyph.bearing = glm::vec2(rect.lsb * scale, 0);
            glyph.advance = rect.advance * scale;
            glyph.valid = true;
            continue;
        }

        /* Check if glyph fits in current row */
        if (current_x + rect.width > atlas_width) {
            current_x = 0;
            current_y += row_height;
            row_height = 0;
        }

        /* Check if atlas is full */
        if (current_y + rect.height > atlas_height) {
            Log::warn(Log::Scope::ENGINE, "Font atlas full, some glyphs may be missing: {}", name);
            break;
        }

        /* Render glyph bitmap */
        int glyph_index = stbtt_FindGlyphIndex(&font_info, rect.codepoint);
        int glyph_w = rect.x1 - rect.x0;
        int glyph_h = rect.y1 - rect.y0;

        if (glyph_w > 0 && glyph_h > 0) {
            /* Render to temporary buffer */
            std::vector<uint8_t> glyph_bitmap(glyph_w * glyph_h);
            stbtt_MakeGlyphBitmap(&font_info, glyph_bitmap.data(), glyph_w, glyph_h, glyph_w, scale, scale, glyph_index);

            /* Generate SDF from bitmap */
            const int sdf_w = glyph_w + glyph_padding;
            const int sdf_h = glyph_h + glyph_padding;
            std::vector<uint8_t> sdf_data(sdf_w * sdf_h);

            /* Simple SDF generation using distance transform */
            for (int sy = 0; sy < sdf_h; ++sy) {
                for (int sx = 0; sx < sdf_w; ++sx) {
                    /* Map SDF pixel to glyph bitmap coordinate */
                    int gx = sx - sdf_padding;
                    int gy = sy - sdf_padding;

                    /* Check if inside glyph */
                    bool inside = false;
                    if (gx >= 0 && gx < glyph_w && gy >= 0 && gy < glyph_h) {
                        inside = glyph_bitmap[gy * glyph_w + gx] > 127;
                    }

                    /* Find minimum distance to edge */
                    float min_dist = sdf_padding + 1.0f;
                    for (int dy = -sdf_padding; dy <= sdf_padding; ++dy) {
                        for (int dx = -sdf_padding; dx <= sdf_padding; ++dx) {
                            int ngx = gx + dx;
                            int ngy = gy + dy;

                            bool neighbor_inside = false;
                            if (ngx >= 0 && ngx < glyph_w && ngy >= 0 && ngy < glyph_h) {
                                neighbor_inside = glyph_bitmap[ngy * glyph_w + ngx] > 127;
                            }

                            if (neighbor_inside != inside) {
                                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                                min_dist = std::min(min_dist, dist);
                            }
                        }
                    }

                    /* Convert distance to 0-255 range */
                    float normalized = (min_dist / sdf_pixel_dist_scale) * sdf_on_edge_value;
                    if (inside) {
                        normalized = sdf_on_edge_value + normalized;
                    } else {
                        normalized = sdf_on_edge_value - normalized;
                    }
                    sdf_data[sy * sdf_w + sx] = static_cast<uint8_t>(std::clamp(normalized, 0.0f, 255.0f));
                }
            }

            /* Copy SDF to atlas */
            for (int y = 0; y < sdf_h; ++y) {
                for (int x = 0; x < sdf_w; ++x) {
                    uint32_t atlas_idx = (current_y + y) * atlas_width + (current_x + x);
                    if (atlas_idx < atlas_data.size()) {
                        atlas_data[atlas_idx] = sdf_data[y * sdf_w + x];
                    }
                }
            }
        }

        /* Store glyph info */
        GlyphInfo& glyph = glyphs[rect.codepoint];
        /* UV coordinates - standard layout (V=0 at top, V=1 at bottom in Vulkan) */
        glyph.uv_rect = glm::vec4(
            static_cast<float>(current_x) / atlas_width, static_cast<float>(current_y) / atlas_height, static_cast<float>(current_x + rect.width) / atlas_width,
            static_cast<float>(current_y + rect.height) / atlas_height
        );
        glyph.size = glm::vec2(rect.width, rect.height);
        /* bearing.x: horizontal offset from cursor to glyph left edge (adjusted for SDF padding)
         * bearing.y: vertical offset from baseline to glyph top (including SDF padding)
         *            y0 is negative for glyphs above baseline, so -y0 gives positive distance
         *            We add sdf_padding because the padded glyph extends further above */
        glyph.bearing = glm::vec2(rect.lsb * scale - sdf_padding, -rect.y0 + sdf_padding);
        glyph.advance = rect.advance * scale;
        glyph.valid = true;

        /* Advance position */
        row_height = std::max(row_height, rect.height);
        current_x += rect.width;
    }

    /* Load kerning pairs */
    int kern_table_length = stbtt_GetKerningTableLength(&font_info);
    if (kern_table_length > 0) {
        std::vector<stbtt_kerningentry> kern_table(kern_table_length);
        stbtt_GetKerningTable(&font_info, kern_table.data(), kern_table_length);

        for (const auto& kern : kern_table) {
            if ((uint32_t)kern.glyph1 >= first_char && (uint32_t)kern.glyph1 <= last_char && (uint32_t)kern.glyph2 >= first_char && (uint32_t)kern.glyph2 <= last_char) {
                uint32_t key = (static_cast<uint32_t>(kern.glyph1) << 16) | static_cast<uint32_t>(kern.glyph2);
                kerning_pairs[key] = kern.advance * scale;
            }
        }
    }

    /* Create fallback glyph (use '?' or first available) */
    if (glyphs.count('?')) {
        fallback_glyph = glyphs['?'];
    } else if (!glyphs.empty()) {
        fallback_glyph = glyphs.begin()->second;
    }

    /* Upload atlas texture to GPU */
    auto& bank = engine.renderer.vram_bank();

    /* Convert single-channel SDF data to RGBA format */
    std::vector<uint8_t> rgba_atlas_data(atlas_width * atlas_height * 4);
    for (int y = 0; y < atlas_height; ++y) {
        for (int x = 0; x < atlas_width; ++x) {
            size_t src_idx = y * atlas_width + x;
            size_t dst_idx = (y * atlas_width + x) * 4;
            rgba_atlas_data[dst_idx + 0] = atlas_data[src_idx]; /* R = SDF value */
            rgba_atlas_data[dst_idx + 1] = atlas_data[src_idx]; /* G = SDF value (for debugging) */
            rgba_atlas_data[dst_idx + 2] = atlas_data[src_idx]; /* B = SDF value (for debugging) */
            rgba_atlas_data[dst_idx + 3] = 255;                 /* A = fully opaque */
        }
    }

    std::string texture_name = name + " Atlas Texture";
    atlas_texture =
        bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA8Unorm, { (uint32_t)atlas_width, (uint32_t)atlas_height, 0 })
            .expect("Failed to create font atlas texture.");

    std::string image_name = name + " Atlas Image";
    atlas_image = bank.create_image(image_name.c_str(), atlas_texture).expect("Failed to create font atlas image.");

    bank.upload_texture(atlas_image, rgba_atlas_data.data(), rgba_atlas_data.size()).expect("Failed to upload font atlas texture.");

    valid = true;
    Log::info(Log::Scope::ENGINE, "Loaded font: {} ({} glyphs)", name, glyphs.size());

    return true;
}

// source: https://github.com/Chlumsky/msdf-atlas-gen/tree/v1.4?tab=readme-ov-file#generating-whole-atlas-at-once
bool Font::generate_msdf_atlas(const char* font_file) {
    bool success = false;

    if (msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype()) {
        if (msdfgen::FontHandle* font = msdfgen::loadFont(ft, font_file)) {
            // Storage for glyph geometry and their coordinates in the atlas
            std::vector<msdf_atlas::GlyphGeometry> glyphs_geo;

            // FontGeometry is a helper class that loads a set of glyphs from a single font.
            // It can also be used to get additional font metrics, kerning information, etc.
            msdf_atlas::FontGeometry fontGeometry(&glyphs_geo);

            // Load a set of character glyphs:
            // The second argument can be ignored unless you mix different font sizes in one atlas.
            // In the last argument, you can specify a charset other than ASCII.
            // To load specific glyph indices, use loadGlyphs instead.
            fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

            // Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
            const double maxCornerAngle = 3.0;
            for (msdf_atlas::GlyphGeometry& glyph : glyphs_geo) glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

            // TightAtlasPacker class computes the layout of the atlas.
            msdf_atlas::TightAtlasPacker packer;

            // Set atlas parameters:
            // setDimensions or setDimensionsConstraint to find the best value
            packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            packer.setDimensions(512, 512);

            // setScale for a fixed size or setMinimumScale to use the largest that fits
            packer.setMinimumScale(24.0);

            // setPixelRange or setUnitRange
            packer.setPixelRange(2.0);
            packer.setMiterLimit(1.0);

            // Compute atlas layout - pack glyphs
            packer.pack(glyphs_geo.data(), (int)glyphs_geo.size());

            // Get final atlas dimensions
            packer.getDimensions(atlas_width, atlas_height);

            // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
            msdf_atlas::ImmediateAtlasGenerator<
                float,                                   // pixel type of buffer for individual glyphs depends on generator function
                3,                                       // number of atlas color channels
                msdf_atlas::msdfGenerator,               // function to generate bitmaps for individual glyphs
                msdf_atlas::BitmapAtlasStorage<byte, 3>  // class that stores the atlas bitmap
                // For example, a custom atlas storage class that stores it in VRAM can be used.
                >
                generator(atlas_width, atlas_height);

            // GeneratorAttributes can be modified to change the generator's default settings.
            msdf_atlas::GeneratorAttributes attributes;
            generator.setAttributes(attributes);
            generator.setThreadCount(4);

            // Generate atlas bitmap
            generator.generate(glyphs_geo.data(), (int)glyphs_geo.size());

            /* Store font metrics — msdf-atlas-gen returns values in em units (1.0 = 1 em).
               Multiply by base_font_size to convert to pixels at the atlas generation size,
               matching the pixel units used for glyph size, bearing, and advance. */
            auto msdf_metrics = fontGeometry.getMetrics();
            metrics.ascender = (float)msdf_metrics.ascenderY * base_font_size;
            metrics.descender = (float)msdf_metrics.descenderY * base_font_size;
            metrics.line_height = (float)msdf_metrics.lineHeight * base_font_size;
            metrics.base_size = base_font_size;

            /* Store glyph info */
            for (const auto& glyph_geo : glyphs_geo) {
                uint32_t codepoint = glyph_geo.getCodepoint();

                GlyphInfo glyph_info {};

                // Atlas UV rect (pixel bounds in atlas)
                double al, ab, ar, at;
                glyph_geo.getQuadAtlasBounds(al, ab, ar, at);

                glyph_info.uv_rect = glm::vec4(
                    (float)al / atlas_width,
                    (float)(atlas_height - at) / atlas_height,  // top in flipped space
                    (float)ar / atlas_width,
                    (float)(atlas_height - ab) / atlas_height   // bottom in flipped space
                );

                // Glyph size and bearing from plane bounds (in em units)
                double pl, pb, pr, pt;
                glyph_geo.getQuadPlaneBounds(pl, pb, pr, pt);

                // Scale from em units to pixel size
                float scale = (float)base_font_size;
                glyph_info.size = glm::vec2((pr - pl) * scale, (pt - pb) * scale);
                glyph_info.bearing = glm::vec2((float)pl * scale, (float)pt * scale);
                glyph_info.advance = (float)glyph_geo.getAdvance() * scale;
                glyph_info.valid = true;

                glyphs[codepoint] = glyph_info;
            }

            /* Store kerning pairs */
            for (const auto& glyph1 : glyphs_geo) {
                for (const auto& glyph2 : glyphs_geo) {
                    double advance;
                    if (fontGeometry.getAdvance(advance, glyph1.getCodepoint(), glyph2.getCodepoint())) {
                        double default_advance = glyph1.getAdvance();
                        double kern = advance - default_advance;
                        if (kern != 0.0) {
                            uint32_t key = (glyph1.getCodepoint() << 16) | glyph2.getCodepoint();
                            kerning_pairs[key] = (float)kern * base_font_size;
                        }
                    }
                }
            }

            /* Fallback glyph */
            if (glyphs.count('?')) {
                fallback_glyph = glyphs['?'];
            } else if (!glyphs.empty()) {
                fallback_glyph = glyphs.begin()->second;
            }

            // Get bitmap
            auto bitmap = (msdfgen::BitmapConstRef<byte, 3>)generator.atlasStorage();

            /* Convert RGB -> RGBA */
            /* Convert RGB -> RGBA (with vertical flip) */
            std::vector<byte> rgba_atlas_data(atlas_width * atlas_height * 4);
            for (int y = 0; y < atlas_height; ++y) {
                int flipped_y = atlas_height - 1 - y;
                for (int x = 0; x < atlas_width; ++x) {
                    int src = (flipped_y * atlas_width + x) * 3;
                    int dst = (y * atlas_width + x) * 4;
                    rgba_atlas_data[dst + 0] = bitmap.pixels[src + 0];
                    rgba_atlas_data[dst + 1] = bitmap.pixels[src + 1];
                    rgba_atlas_data[dst + 2] = bitmap.pixels[src + 2];
                    rgba_atlas_data[dst + 3] = 255;
                }
            }

            /* Upload atlas texture to GPU */
            auto& bank = engine.renderer.vram_bank();

            std::string texture_name = name + " Atlas Texture";
            atlas_texture =
                bank.create_texture(texture_name.c_str(), TextureUsage::Sampled | TextureUsage::TransferDst, TextureFormat::RGBA8Unorm, { (uint32_t)atlas_width, (uint32_t)atlas_height, 0 })
                    .expect("Failed to create font atlas texture.");

            std::string image_name = name + " Atlas Image";
            atlas_image = bank.create_image(image_name.c_str(), atlas_texture).expect("Failed to create font atlas image.");

            bank.upload_texture(atlas_image, rgba_atlas_data.data(), rgba_atlas_data.size()).expect("Failed to upload font atlas texture.");

            valid = true;
            Log::info(Log::Scope::ENGINE, "Loaded font: {} ({} glyphs)", name, glyphs.size());

            // Cleanup
            msdfgen::destroyFont(font);

            success = true;
        }
        msdfgen::deinitializeFreetype(ft);
    }
    return success;
}

void Font::unload() {
    if (!valid) return;

    auto& bank = engine.renderer.vram_bank();

    bank.destroy(atlas_texture);
    bank.destroy(atlas_image);

    glyphs.clear();
    kerning_pairs.clear();
    valid = false;
}

const GlyphInfo& Font::get_glyph(uint32_t codepoint) const {
    auto it = glyphs.find(codepoint);
    if (it != glyphs.end()) {
        return it->second;
    }
    return fallback_glyph;
}

float Font::get_kerning(uint32_t left, uint32_t right) const {
    uint32_t key = (left << 16) | right;
    auto it = kerning_pairs.find(key);
    if (it != kerning_pairs.end()) {
        return it->second;
    }
    return 0.0f;
}

float Font::measure_text(const std::string& text, float font_size) const {
    if (!valid || text.empty()) return 0.0f;

    const float scale_factor = font_size / metrics.base_size;
    float width = 0.0f;
    uint32_t prev_char = 0;

    for (size_t i = 0; i < text.size(); ++i) {
        uint32_t c = static_cast<uint32_t>(text[i]);

        /* Add kerning */
        if (prev_char != 0) {
            width += get_kerning(prev_char, c) * scale_factor;
        }

        /* Add glyph advance */
        const GlyphInfo& glyph = get_glyph(c);
        width += glyph.advance * scale_factor;

        prev_char = c;
    }

    return width;
}

/* FontManager implementation */
void FontManager::init() {
    if (initialized) return;

    /* Create default font from embedded data */
    default_font = std::make_shared<Font>(IO::FileLocation { IO::Location::ENGINE, "fonts/Rubik-Regular.ttf" });

    initialized = true;
}

void FontManager::shutdown() {
    if (!initialized) return;

    if (default_font) {
        default_font->unload();
        default_font = nullptr;
    }

    initialized = false;
}

std::shared_ptr<Font> FontManager::get_default_font() {
    if (!initialized) {
        init();
    }
    return default_font;
}

bool FontManager::has_default_font() {
    if (!initialized) {
        init();
    }
    return default_font != nullptr && default_font->is_valid();
}

}  // namespace tmt