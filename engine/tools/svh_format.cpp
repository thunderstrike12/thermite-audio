#include "svh_format.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/resources/voxel_scene.hpp"
#include "engine/tools/uuid.hpp"
#include "engine/shared/colorspace.hpp"

namespace tmt {

namespace {

constexpr uint32_t MAGIC_NUMBER { 0xFE2032A1 };
constexpr uint32_t CURRENT_MAJOR_VERSION { 1 };
constexpr uint32_t CURRENT_MINOR_VERSION { 1 };

// Helper structs with correct size to easily parse the binary data.
struct FileHeader {
    uint32_t magic_number;
    uint32_t major_version;
    uint32_t minor_version;
};
static_assert(sizeof(FileHeader) == 12);

struct HierarchyHeader {
    uint32_t node_count;
    uint32_t char_count;
};
static_assert(sizeof(HierarchyHeader) == 8);

struct HierarchyNode {
    UUID uuid;  // *Technically* not trivially copyable, but has the same layout and will be copied right after.
    uint32_t parent_index;
    uint32_t svt64_offset;
    uint32_t svt64_size;
    uint32_t name_offset;
    uint32_t name_length;
    glm::uvec3 size;
    glm::mat4 transform;
};
static_assert(sizeof(HierarchyNode) == 112);

struct SVT64Header {
    uint32_t tree_node_count;
    uint32_t voxel_count;
    uint32_t depth;
};
static_assert(sizeof(SVT64Header) == 12);

// Helper functions for reading data.
template <typename Type>
const Type& read_data(const char*& data_pointer) {
    const Type* value = reinterpret_cast<const Type*>(data_pointer);
    data_pointer += sizeof(Type);

    return *value;
}

template <typename Type>
std::span<const Type> read_data(const char*& data_pointer, const uint32_t size) {
    const Type* data_start = reinterpret_cast<const Type*>(data_pointer);
    data_pointer += size * sizeof(Type);

    return { data_start, data_start + size };
}

template <typename Type>
void read_data_to_buffer(const char*& data_pointer, Type* read_out_buffer, const uint32_t read_count) {
    const size_t total_data_size = read_count * sizeof(Type);
    std::memcpy(read_out_buffer, data_pointer, total_data_size);
    data_pointer += total_data_size;
}

template <typename Type>
void write_data(std::vector<char>& data, Type& value) {
    const size_t write_index = data.size();
    data.resize(write_index + sizeof(Type));
    std::memcpy(data.data() + write_index, &value, sizeof(Type));
}

// Helper functions for writing data.
template <typename Type>
void write_data(std::vector<char>& data, std::vector<Type>& value) {
    const size_t write_index = data.size();
    const size_t total_data_size = value.size() * sizeof(Type);

    data.resize(write_index + total_data_size);
    std::memcpy(data.data() + write_index, value.data(), total_data_size);
}

template <typename Type>
void write_data(std::vector<char>& data, const Type* write_data, const uint32_t write_data_count) {
    const size_t write_index = data.size();

    const size_t write_data_size = write_data_count * sizeof(Type);
    data.resize(write_index + write_data_size);

    std::memcpy(data.data() + write_index, write_data, write_data_size);
}

std::unique_ptr<Svt64> decode_svt64_tree(const std::span<const char>& svt64_data, const FileHeader& file_header) {
    // If the node doesn't have model/svt64 data then we skip parsing/allocating the svt64 tree.
    if (svt64_data.empty()) return {};

    const char* data_pointer = svt64_data.data();
    const auto& header = read_data<SVT64Header>(data_pointer);

    auto tree = std::make_unique<Svt64>();
    tree->node_count = header.tree_node_count;
    tree->voxel_count = header.voxel_count;
    tree->voxels_capacity = header.voxel_count + SVT64_BUFFER_MEMORY;
    tree->nodes_capacity = header.tree_node_count + SVT64_BUFFER_MEMORY;
    tree->depth = header.depth;

    // Handle material palette conversion for versions below 1.1.
    if (file_header.major_version == 1 && file_header.minor_version < 1) {
        const std::span<const glm::vec4> albedo_palette = read_data<glm::vec4>(data_pointer, 256llu);
        constexpr size_t MIN_MATERIAL_COUNT = glm::min(256llu, MaterialPalette::ENTRY_COUNT);

        for (size_t i = 0; i < MIN_MATERIAL_COUNT; i++) {
            tree->palette.entries[i] = Material { .albedo = cs::r709_to_acescg(cs::linearize(albedo_palette[i])) };
        }
    } else {
        tree->palette = read_data<MaterialPalette>(data_pointer);
    }

    tree->nodes = new Svt64Node[tree->node_count + SVT64_BUFFER_MEMORY];
    read_data_to_buffer(data_pointer, tree->nodes, tree->node_count);

    tree->materials = new MaterialIndex[tree->voxel_count + SVT64_BUFFER_MEMORY];
    read_data_to_buffer(data_pointer, tree->materials, tree->voxel_count);

    tree->physics_data = new PhysicsVoxel[tree->voxel_count + SVT64_BUFFER_MEMORY];
    read_data_to_buffer(data_pointer, tree->physics_data, tree->voxel_count);

    return tree;
}

struct VoxelSceneDecodeData {
    std::span<const HierarchyNode> hierarchy_nodes;
    std::string_view string_data;
    std::span<const char> svt64_data;
};

void recurse_decode_scene_node(VoxelSceneNode& scene_node, VoxelSceneDecodeData& decode_data, size_t& node_index, const FileHeader& file_header) {
    const HierarchyNode& node_data = decode_data.hierarchy_nodes[node_index];

    const auto& node_svt64_data = decode_data.svt64_data.subspan(node_data.svt64_offset, node_data.svt64_size);
    scene_node.uuid = node_data.uuid;
    scene_node.tree = decode_svt64_tree(node_svt64_data, file_header);
    scene_node.size = node_data.size;
    scene_node.name = decode_data.string_data.substr(node_data.name_offset, node_data.name_length);
    scene_node.transform = node_data.transform;

    const size_t current_index = node_index;
    while ((node_index + 1) < decode_data.hierarchy_nodes.size() && decode_data.hierarchy_nodes[node_index + 1].parent_index == current_index) {
        recurse_decode_scene_node(scene_node.children.emplace_back(), decode_data, ++node_index, file_header);
    }
}

uint32_t encode_svt64_tree(const std::unique_ptr<Svt64>& tree, std::vector<char>& svt64_data) {
    // If the node doesn't have a model/svt64 tree, then we skip trying to parse it.
    if (!tree) return 0;

    const size_t start_size = svt64_data.size();

    const SVT64Header header {
        .tree_node_count = tree->node_count,
        .voxel_count = tree->voxel_count,
        .depth = tree->depth,
    };
    write_data(svt64_data, header);

    write_data(svt64_data, tree->palette);
    write_data(svt64_data, tree->nodes, tree->node_count);
    write_data(svt64_data, tree->materials, tree->voxel_count);
    write_data(svt64_data, tree->physics_data, tree->voxel_count);

    return static_cast<uint32_t>(svt64_data.size() - start_size);
}

struct VoxelSceneEncodeData {
    std::vector<HierarchyNode> hierarchy_nodes;
    std::string string_data;
    std::vector<char> svt64_data;
};

void recurse_encode_voxel_node(const VoxelSceneNode& node, const uint32_t parent_index, VoxelSceneEncodeData& encode_data) {
    const uint32_t current_index = static_cast<int32_t>(encode_data.hierarchy_nodes.size());

    const uint32_t svt64_offset = static_cast<uint32_t>(encode_data.svt64_data.size());
    const uint32_t svt64_size = encode_svt64_tree(node.tree, encode_data.svt64_data);

    const uint32_t string_offset = static_cast<uint32_t>(encode_data.string_data.size());
    encode_data.string_data += node.name;

    const HierarchyNode hierarchy_node { .uuid = node.uuid,
                                         .parent_index = parent_index,
                                         .svt64_offset = svt64_offset,
                                         .svt64_size = svt64_size,
                                         .name_offset = string_offset,
                                         .name_length = static_cast<uint32_t>(node.name.size()),
                                         .size = node.size,
                                         .transform = node.transform };
    encode_data.hierarchy_nodes.push_back(hierarchy_node);

    for (const VoxelSceneNode& child : node.children) {
        recurse_encode_voxel_node(child, current_index, encode_data);
    }
}

}  // namespace

std::vector<VoxelSceneNode> decode_svh(const std::vector<char>& data) {
    const auto* data_pointer = data.data();

    const auto& file_header = read_data<FileHeader>(data_pointer);
    if (file_header.magic_number != MAGIC_NUMBER) return {};  // Make sure the magic number matches.
    assert(
        (file_header.major_version < CURRENT_MAJOR_VERSION || (file_header.major_version == CURRENT_MAJOR_VERSION && file_header.minor_version <= CURRENT_MINOR_VERSION)) &&
        "File format version not supported!"
    );
    const auto& hierarchy_header = read_data<HierarchyHeader>(data_pointer);

    VoxelSceneDecodeData decode_data;
    decode_data.hierarchy_nodes = read_data<HierarchyNode>(data_pointer, hierarchy_header.node_count);
    decode_data.string_data = std::string_view { data_pointer, hierarchy_header.char_count };
    data_pointer += hierarchy_header.char_count;
    decode_data.svt64_data = std::span<const char> { data_pointer, data.data() + data.size() };  // The rest of the file is svt64_data.

    size_t node_index = 0;
    std::vector<VoxelSceneNode> root_nodes;

    while (node_index < decode_data.hierarchy_nodes.size()) {
        VoxelSceneNode& root_node = root_nodes.emplace_back();

        recurse_decode_scene_node(root_node, decode_data, node_index, file_header);
        ++node_index;
    }

    return root_nodes;
}

std::vector<char> encode_svh(const std::span<VoxelSceneNode>& root_nodes) {
    std::vector<char> data {};

    constexpr FileHeader file_header {
        .magic_number = MAGIC_NUMBER,
        .major_version = CURRENT_MAJOR_VERSION,
        .minor_version = CURRENT_MINOR_VERSION,
    };
    write_data(data, file_header);

    VoxelSceneEncodeData encode_data;
    for (const VoxelSceneNode& root_node : root_nodes) {
        recurse_encode_voxel_node(root_node, UINT_MAX, encode_data);
    }

    HierarchyHeader hierarchy_header {
        .node_count = static_cast<uint32_t>(encode_data.hierarchy_nodes.size()),
        .char_count = static_cast<uint32_t>(encode_data.string_data.size()),
    };
    write_data(data, hierarchy_header);

    write_data(data, encode_data.hierarchy_nodes);

    const auto* string_data = reinterpret_cast<const uint8_t*>(encode_data.string_data.data());
    data.insert(data.end(), string_data, string_data + encode_data.string_data.size());

    // Insert the svt64 tree data at the end.
    data.insert(data.end(), encode_data.svt64_data.begin(), encode_data.svt64_data.end());

    return data;
}

}  // namespace tmt