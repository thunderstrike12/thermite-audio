#include "vengi_parser.hpp"
#include "engine/core/logger.hpp"
#include "zlib.h"

std::unique_ptr<vengi::Node> VengiParser::load(const tmt::IO::FileLocation& vengi_file) {
    using namespace tmt;
    std::vector<char> data = IO::read_file(vengi_file);

    std::unique_ptr<vengi::Node> ret_node = std::make_unique_for_overwrite<vengi::Node>();
    try {
        if (data.size() < 8) {
            throw std::runtime_error("File too small to be valid VENGI file");
            return ret_node;
        }

        // Check startfile "VENG" identifier
        uint32_t veng_mark = *reinterpret_cast<const uint32_t*>(&data[0]);
        if (veng_mark != 0x474E4556) {  // "VENG" in little-endian
            throw std::runtime_error("Invalid magic number. Expected 'VENG'");
            return ret_node;
        }

        std::vector<char> decompressed_output = zlib_decompress_vengi_file(data);

        // Parse decompressed data
        vengi::BinaryParser reader(decompressed_output);

        // Read version
        uint32_t version = reader.read_version();
        if (version > 6) {
            Log::warn(Log::Scope::ENGINE, "Newer file format detected, Vengi Parser might need refactor..");
        }

        // Parse root node
        if (!reader.parse_node(*ret_node)) {
            throw std::runtime_error("Node data parsing error!");
        }

        compute_ref_world_transforms(*ret_node, glm::mat4(1.f));

        return ret_node;
    } catch (const std::exception& e) {
        Log::error(Log::Scope::ENGINE, "Exception raised by loading .vengi file:\n{}", e.what());
        return ret_node;
    }
}

std::vector<char> VengiParser::zlib_decompress_vengi_file(std::vector<char>& compressed_data) {
    // Decompress zlib data (starts after magic number)
    std::vector<char> decompressed_output;
    z_stream stream = {};
    stream.next_in = reinterpret_cast<unsigned char*>(compressed_data.data() + 4);
    stream.avail_in = static_cast<uint32_t>(compressed_data.size() - 4);

    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("Zip parsing error!");
    }

    const size_t chunk_size = 32768;
    decompressed_output.reserve(chunk_size * 3);  // Estimate

    int ret;
    do {
        size_t old_size = decompressed_output.size();
        decompressed_output.resize(old_size + chunk_size);

        stream.next_out = reinterpret_cast<unsigned char*>(decompressed_output.data() + old_size);
        stream.avail_out = chunk_size;

        ret = inflate(&stream, Z_NO_FLUSH);

        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            throw std::runtime_error("Zip parsing error");
        }

        decompressed_output.resize(old_size + chunk_size - stream.avail_out);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);

    return decompressed_output;
}

void VengiParser::compute_ref_world_transforms(vengi::Node& node, const glm::mat4& parent_matrix) {
    if (node.animations.empty()) return;

    const glm::mat4& local = node.animations[0].keyframes[0].local_matrix;
    glm::mat4 world = parent_matrix * local;
    node.transform.set_world_matrix(world);

    for (auto& child : node.children) {
        compute_ref_world_transforms(*child, world);
    }
}

namespace vengi {
vengi::NodeType BinaryParser::parse_node_type(const std::string& type_str) {
    if (type_str == "Root") return NodeType::ROOT;
    if (type_str == "Model") return NodeType::MODEL;
    if (type_str == "ModelReference") return NodeType::MODELREFERENCE;
    if (type_str == "Group") return NodeType::GROUP;
    if (type_str == "Camera") return NodeType::CAMERA;
    if (type_str == "Point") return NodeType::POINT;
    return NodeType::NONE;
}

bool BinaryParser::parse_node(vengi::Node& node, vengi::Node* parent) {
    // Read NODE FourCC
    uint32_t four_cc = read_four_cc();
    if (four_cc != 0x45444F4E) {  // "NODE"
        throw std::runtime_error("Expected NODE block!");
    }

    // Read node data
    node.name = read_string();
    std::string type_str = read_string();
    node.type = parse_node_type(type_str);
    node.uuid[0] = read_uint64();
    node.uuid[1] = read_uint64();
    node.node_id = read_int32();
    node.reference_node_id = read_int32();
    node.visible = read_bool();
    node.locked = read_bool();
    node.color = RGBA::from_abgr(read_uint32());
    node.pivot = read_vec3f();

    // Parse chunks until ENDN
    while (!eof()) {
        four_cc = read_four_cc();

        switch (four_cc) {
            case 0x4E444E45:
                return true;
            case 0x504F5250:
                if (!parse_properties(node)) return false;
                break;
            case 0x41544144:
                if (!parse_voxel_data(node)) return false;
                break;
            case 0x434C4150:
                if (!parse_palette_colors(node)) return false;
                break;
            case 0x4E4C4150:
                if (!parse_palette_normals(node)) return false;
                break;
            case 0x494C4150:
                if (!parse_palette_identifier(node)) return false;
                break;
            case 0x4D494E41: {
                Animation anim;
                if (!parse_animation(anim)) return false;
                node.animations.push_back(anim);
                break;
            }
            case 0x45444F4E: {
                // Rewind to read NODE again
                seek(tell() - 4);
                auto child = std::make_unique<Node>();
                if (!parse_node(*child, &node)) return false;
                node.children.push_back(std::move(child));
                break;
            }
            default:
                // Unknown chunk. this shouldn't happen if format is correct
                throw std::runtime_error("Unknown chunk, panic.");
                break;
        }
    }

    return true;
}

bool BinaryParser::parse_properties(vengi::Node& node) {
    uint32_t count = read_uint32();
    for (uint32_t i = 0; i < count; i++) {
        std::string key = read_string();
        std::string value = read_string();
        node.properties[key] = value;
    }
    return true;
}

bool BinaryParser::parse_voxel_data(vengi::Node& node) {
    node.voxel_data = std::make_unique<VoxelData>();

    // IMPORTANT: VENGI stores region as (Z, Y, X) order, NOT (X, Y, Z)!
    // Read 6 int32s: lower z, lower y, lower x, upper z, upper y, upper x
    int32_t lower_z = read_int32();
    int32_t lower_y = read_int32();
    int32_t lower_x = read_int32();
    int32_t upper_z = read_int32();
    int32_t upper_y = read_int32();
    int32_t upper_x = read_int32();

    // Store in our X, Y, Z order
    node.voxel_data->region.lower = glm::ivec3(lower_x, lower_y, lower_z);
    node.voxel_data->region.upper = glm::ivec3(upper_x, upper_y, upper_z) + 1;

    int64_t volume = node.voxel_data->region.volume();
    node.voxel_data->voxels.reserve(volume);

    // Voxels are also stored in Z->Y->X order (not X->Y->Z)
    for (int32_t z = node.voxel_data->region.lower.z; z < node.voxel_data->region.upper.z; z++) {
        for (int32_t y = node.voxel_data->region.lower.y; y < node.voxel_data->region.upper.y; y++) {
            for (int32_t x = node.voxel_data->region.lower.x; x < node.voxel_data->region.upper.x; x++) {
                bool is_air = read_bool();
                if (is_air) {
                    node.voxel_data->voxels.emplace_back();
                } else {
                    uint8_t color_index = read_uint8();
                    uint8_t normal_index = read_uint8();
                    node.voxel_data->voxels.emplace_back(color_index, normal_index);
                }
            }
        }
    }

    return true;
}

bool BinaryParser::parse_palette_colors(vengi::Node& node) {
    if (!node.palette) {
        node.palette = std::make_unique<Palette>();
    }

    node.palette->name = read_string();
    uint32_t color_count = read_uint32();

    // Read colors
    std::vector<RGBA> colors;
    for (uint32_t i = 0; i < color_count; i++) {
        colors.push_back(RGBA::from_abgr(read_uint32()));
    }

    // Read emit colors (deprecated)
    std::vector<RGBA> emit_colors;
    for (uint32_t i = 0; i < color_count; i++) {
        emit_colors.push_back(RGBA::from_abgr(read_uint32()));
    }

    // Read indices
    std::vector<uint8_t> indices;
    for (uint32_t i = 0; i < color_count; i++) {
        indices.push_back(read_uint8());
    }

    // Read color names
    std::vector<std::string> names;
    for (uint32_t i = 0; i < color_count; i++) {
        names.push_back(read_string());
    }

    // Combine into palette colors
    for (uint32_t i = 0; i < color_count; i++) {
        PaletteColor pc;
        pc.color = colors[i];
        pc.emit_color = emit_colors[i];
        pc.index = indices[i];
        pc.name = names[i];
        node.palette->colors.push_back(pc);
    }

    // Read materials
    uint32_t material_count = read_uint32();
    for (uint32_t i = 0; i < material_count; i++) {
        Material mat;
        mat.type = read_uint32();
        uint8_t prop_count = read_uint8();

        for (uint8_t j = 0; j < prop_count; j++) {
            MaterialProperty prop;
            prop.name = read_string();
            prop.value = read_float();
            mat.properties.push_back(prop);
        }

        node.palette->materials.push_back(mat);
    }

    return true;
}

bool BinaryParser::parse_palette_normals(vengi::Node& node) {
    if (!node.palette) {
        node.palette = std::make_unique<Palette>();
    }

    uint32_t normal_count = read_uint32();
    for (uint32_t i = 0; i < normal_count; i++) {
        node.palette->normals.push_back(RGBA::from_abgr(read_uint32()));
    }

    return true;
}

bool BinaryParser::parse_palette_identifier(vengi::Node& node) {
    if (!node.palette) {
        node.palette = std::make_unique<vengi::Palette>();
    }

    node.palette->name = read_string();
    return true;
}

bool BinaryParser::parse_animation(vengi::Animation& anim) {
    anim.name = read_string();

    // Parse keyframes until ENDA
    while (!eof()) {
        uint32_t fourCC = read_four_cc();

        if (fourCC == 0x41444E45) {  // "ENDA"
            break;
        } else if (fourCC == 0x4659454B) {  // "KEYF"
            Keyframe keyframe;
            keyframe.frame_index = read_uint32();
            keyframe.long_rotation = read_bool();
            keyframe.interpolation_type = read_string();
            keyframe.local_matrix = read_matrix4x4();
            anim.keyframes.push_back(keyframe);
        } else {
            throw std::runtime_error("Unexpected chunk in animation!");
        }
    }

    return true;
}

}  // namespace vengi