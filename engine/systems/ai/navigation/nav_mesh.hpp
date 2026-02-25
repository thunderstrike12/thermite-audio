#pragma once

#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

class Volume {
   public:
    glm::uvec3 size;
    std::vector<bool> occupied;
    uint32_t divisor;
    bool traversed = false;

    void traverse(tmt::ResourceRef<tmt::VoxelVolume> voxel_volume, int lod_level);
    bool voxel_empty(uint32_t x, uint32_t y, uint32_t z) { return !occupied[x + size.x * y + size.x * size.y * z]; };
    bool is_surface(uint32_t x, uint32_t y, uint32_t z);
};

enum class VoxelType : uint8_t { EMPTY, OUTSIDE, INSIDE };

struct NavVoxel {
    VoxelType type = VoxelType::EMPTY;
};

class Node {
   public:
    glm::vec3 local_pos;
    glm::vec3 world_pos;
    int parent = 0;

    uint32_t voxel_index = 0;

    float g = -1.0f;
    float h = 0.0f;
    int id = -1;

    std::vector<int> connecting_nodes;
};

class NavMesh {
   public:
    NavMesh() = default;
    ~NavMesh() {
        delete buffer_a;
        delete buffer_b;
    };

    std::vector<Node>* nodes;
    std::vector<int> path;

    tmt::ResourceRef<tmt::VoxelVolume> voxel_volume;
    Volume volume;
    int lod_level = 0;

    bool generating = false;

   private:
    int generating_lod = 0;
    int iteration_nmg = 0;
    bool entered_loop = false;

    void init() {
        delete buffer_a;
        delete buffer_b;
        buffer_a = new std::vector<Node>;
        buffer_b = new std::vector<Node>;
        nodes = buffer_a;
        generating_nodes = buffer_b;
    }
    std::vector<Node>* buffer_a = nullptr;
    std::vector<Node>* buffer_b = nullptr;

    std::vector<Node>* generating_nodes = nullptr;

   public:
    void generate_mesh_over_time();
    void generate_mesh(int iterations = 1e34);
    std::vector<int> find_path(const int starting_node_id, const int ending_node_id);
    int find_closest_node(const glm::vec3& position);
    std::optional<glm::vec3> follow_path(glm::vec3 start, glm::vec3 end);
    void inspect();
};

}  // namespace tmt

TMT_COMPONENT(tmt::NavMesh, "Navigation Mesh", (voxel_volume, lod_level));