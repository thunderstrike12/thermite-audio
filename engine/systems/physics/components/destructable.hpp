#pragma once
#include "engine/shared/svt64.hpp"

namespace tmt {

// 1024 ^3, max number of leaf nodes in a svt64 with the largest supported size
constexpr uint32_t MAX_VOXEL_NODES = 1 << 30;

struct DestructionNode;

struct AxisConnections {
    std::vector<uint32_t> node_indices;

    void add_connection(uint32_t node_index) { node_indices.push_back(node_index); };
};

struct DestructionNode {
    uint32_t id = 0;

    // Level is the inverse depth of the tree, where 0 is the voxel level
    uint8_t level = 0;

    // Index of the corresponding node in the svt64
    uint32_t tree_node = 0;

    glm::uvec3 global_position = glm::uvec3(0);

    bool cleared = false;
    uint64_t flood_id = 0;
    std::vector<std::pair<uint8_t, uint64_t>> flood_masks;

    // One vector per axis (non continuous memory)
    std::array<AxisConnections, 6> connections;
};

struct Destructible {
    bool initialized = false;
    uint32_t node_count = 0;

    // svt64 tree node index to destruction node index
    std::unordered_map<uint32_t, uint32_t> nodes_map;
    std::vector<DestructionNode> nodes;

    DestructionNode* get_node(uint32_t x, uint32_t y, uint32_t z) {
        uint32_t key = x + (y << 10) + (z << 20);
        auto it = nodes_map.find(key);
        if (it != nodes_map.end()) {
            return &nodes[it->second];
        }
        return nullptr;
    }

    const DestructionNode* get_node(uint32_t x, uint32_t y, uint32_t z) const {
        uint32_t key = x + (y << 10) + (z << 20);
        auto it = nodes_map.find(key);
        if (it != nodes_map.end()) {
            return &nodes[it->second];
        }
        return nullptr;
    }

    void clear() {
        node_count = 0;
        nodes_map.clear();
        nodes.clear();
    }
};

}  // namespace tmt