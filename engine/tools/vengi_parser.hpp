#pragma once

#include "engine/core/io.hpp"
#include "glm/fwd.hpp"
#include "core/components/transform.hpp"

namespace vengi {

struct Region {
    glm::ivec3 lower;
    glm::ivec3 upper;

    Region() {}
    Region(const glm::ivec3& lower, const glm::ivec3& upper) : lower(lower), upper(upper) {}

    int32_t width() const { return upper.x - lower.x; }
    int32_t height() const { return upper.y - lower.y; }
    int32_t depth() const { return upper.z - lower.z; }
    int64_t volume() const { return static_cast<int64_t>(width() * height() * depth()); }
};

struct RGBA {
    uint8_t r, g, b, a;

    RGBA() : r(0), g(0), b(0), a(255) {}
    RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

    // Convert from ABGR uint32
    static RGBA from_abgr(uint32_t abgr) {
        return RGBA(
            (abgr >> 0) & 0xFF,   // R
            (abgr >> 8) & 0xFF,   // G
            (abgr >> 16) & 0xFF,  // B
            (abgr >> 24) & 0xFF   // A
        );
    }

    uint32_t to_abgr() const { return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r; }
};

struct PaletteColor {
    RGBA color;
    RGBA emit_color;
    uint8_t index;
    std::string name;
};

struct MaterialProperty {
    std::string name;
    float value;
};

struct Material {
    uint32_t type;
    std::vector<MaterialProperty> properties;
};

struct Palette {
    std::string name;
    std::vector<PaletteColor> colors;
    std::vector<RGBA> normals;
    std::vector<Material> materials;

    // Helper to get color by index
    RGBA get_color(uint8_t index) const {
        for (const auto& pc : colors) {
            if (pc.index == index) return pc.color;
        }
        return RGBA();  // Default black if not found
    }

    // Helper to get normal by index
    RGBA get_normal(uint8_t index) const {
        if (index < normals.size()) {
            return normals[index];
        }
        return RGBA(128, 128, 255, 255);  // Default upward normal
    }
};
struct Keyframe {
    uint32_t frame_index;
    bool long_rotation;
    std::string interpolation_type;
    glm::mat4 local_matrix;
};

struct Animation {
    std::string name;
    std::vector<Keyframe> keyframes;
};

enum class NodeType { ROOT, MODEL, MODELREFERENCE, GROUP, CAMERA, POINT, NONE };

struct VoxelInformation {
    bool is_air;
    uint8_t color_index;   // Palette index for color
    uint8_t normal_index;  // Palette index for normal

    VoxelInformation() : is_air(true), color_index(0), normal_index(0) {}
    VoxelInformation(uint8_t color, uint8_t normal) : is_air(false), color_index(color), normal_index(normal) {}
};

struct VoxelData {
    Region region;
    std::vector<VoxelInformation> voxels;

    // Get voxel at specific position (returns nullptr if out of bounds)
    const VoxelInformation* get(int32_t x, int32_t y, int32_t z) const {
        if (x < region.lower.x || x > region.upper.x || y < region.lower.y || y > region.upper.y || z < region.lower.z || z > region.upper.z) {
            return nullptr;
        }

        int32_t idx = (x - region.lower.x) + (y - region.lower.y) * region.width() + (z - region.lower.z) * region.height() * region.width();

        if (idx >= 0 && idx < (int32_t)voxels.size()) {
            return &voxels[idx];
        }
        return nullptr;
    }
};

struct Node {
    std::string name;
    NodeType type;
    uint64_t uuid[2];  // 128-bit UUID
    int32_t node_id;
    int32_t reference_node_id;
    bool visible;
    bool locked;
    RGBA color;
    glm::vec3 pivot;
    glm::mat4 transform;  // local transform

    std::unordered_map<std::string, std::string> properties;
    std::unique_ptr<Palette> palette;
    std::unique_ptr<VoxelData> voxel_data;
    std::vector<Animation> animations;
    std::vector<std::unique_ptr<Node>> children;

    Node() : type(NodeType::NONE), node_id(-1), reference_node_id(-1), visible(true), locked(false), pivot(0.f) { uuid[0] = uuid[1] = 0; }

    // Helper to find child nodes by name
    Node* find_child(const std::string& child_name) {
        for (auto& child : children) {
            if (child->name == child_name) return child.get();
        }
        return nullptr;
    }

    // Helper to count all model nodes recursively
    size_t count_model_nodes() const {
        size_t count = (type == NodeType::MODEL) ? 1 : 0;
        for (const auto& child : children) {
            count += child->count_model_nodes();
        }
        return count;
    }
};

class BinaryParser {
   public:
    BinaryParser(const std::vector<char>& data) : data(data), pos(0) {}

    bool parse_node(vengi::Node& node);

    int32_t read_version() { return read_uint32(); }

   private:
    // reader related
    const std::vector<char>& data;
    size_t pos;

    size_t tell() const { return pos; }
    void seek(size_t npos) { pos = npos; }
    size_t size() const { return data.size(); }
    bool eof() const { return pos >= data.size(); }

    bool can_read(size_t bytes) const { return pos + bytes <= data.size(); }

    uint8_t read_uint8() {
        if (!can_read(1)) throw std::runtime_error("Unexpected end of data");
        return data[pos++];
    }

    int32_t read_int32() {
        if (!can_read(4)) throw std::runtime_error("Unexpected end of data");
        int32_t value = *reinterpret_cast<const int32_t*>(&data[pos]);
        pos += 4;
        return value;
    }

    uint32_t read_uint32() {
        if (!can_read(4)) throw std::runtime_error("Unexpected end of data");
        uint32_t value = *reinterpret_cast<const uint32_t*>(&data[pos]);
        pos += 4;
        return value;
    }

    uint64_t read_uint64() {
        if (!can_read(8)) throw std::runtime_error("Unexpected end of data");
        uint64_t value = *reinterpret_cast<const uint64_t*>(&data[pos]);
        pos += 8;
        return value;
    }

    float read_float() {
        if (!can_read(4)) throw std::runtime_error("Unexpected end of data");
        float value = *reinterpret_cast<const float*>(&data[pos]);
        pos += 4;
        return value;
    }

    bool read_bool() { return read_uint8() != 0; }

    std::string read_string() {
        uint16_t length = read_uint16();
        if (!can_read(length)) throw std::runtime_error("Unexpected end of data");
        std::string result(reinterpret_cast<const char*>(&data[pos]), length);
        pos += length;
        return result;
    }

    uint16_t read_uint16() {
        if (!can_read(2)) throw std::runtime_error("Unexpected end of data");
        uint16_t value = *reinterpret_cast<const uint16_t*>(&data[pos]);
        pos += 2;
        return value;
    }

    uint32_t read_four_cc() { return read_uint32(); }

    glm::vec3 read_vec3f() { return glm::vec3 { read_float(), read_float(), read_float() }; }

    glm::mat4 read_matrix4x4() {
        glm::mat4 mat;
        for (int c = 0; c < 4; c++) {
            for (int r = 0; r < 4; r++) {
                mat[c][r] = read_float();
            }
        }
        return mat;
    }

    // parsing related
    vengi::NodeType parse_node_type(const std::string& type_str);
    bool parse_palette_identifier(vengi::Node& node);
    bool parse_palette_normals(vengi::Node& node);
    bool parse_palette_colors(vengi::Node& node);
    bool parse_voxel_data(vengi::Node& node);
    bool parse_properties(vengi::Node& node);
    bool parse_animation(vengi::Animation& anim);
};

}  // namespace vengi

class VengiParser {
   public:
    static std::unique_ptr<vengi::Node> load(const tmt::IO::FileLocation& vengi_file);

   private:
    static std::vector<char> zlib_decompress_vengi_file(std::vector<char>& compressed_data);
    static void compute_parent_transform_offsets(vengi::Node& node, const glm::vec3& parent_offset);
};