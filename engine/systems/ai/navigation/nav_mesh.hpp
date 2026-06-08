#pragma once

#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

struct GenerateClosestNavMeshToThis {};

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

enum class NavMeshGenerationState {
    UNINITIALISED,
    INITIALISING_VOLUME,
    GENERATING_MESH,
    FINISHED_LOWER_LOD,
    GENERATING_NORMALS,
    AVERAGING_NORMALS1,
    AVERAGING_NORMALS2,
    AVERAGING_NORMALS3,
    FINISHED_AVERAGING_NORMALS,
    FINISHED
};

class NavNode {
   public:
    glm::vec3 local_pos;
    glm::vec3 world_pos;
    glm::vec3 normal;
    int parent = 0;

    int iteration = 0;

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

    NavMeshGenerationState generation_state = NavMeshGenerationState::UNINITIALISED;
    std::vector<NavNode>* nodes_mesh;
    std::vector<NavNode>* generating_nodes = nullptr;
    std::vector<int> path;

    // tmt::ResourceRef<tmt::VoxelScene> voxel_volume;
    // tmt::ResourceRef<tmt::VoxelVolume> voxel_volume;
    std::vector<Entity> nav_mesh_entities;
    int selected_entity_index = 0;
    /*static constexpr */int lod_level = 2;
    glm::vec3 inflation = glm::vec3(1.0f);

    bool draw_nodes = false;
    bool draw_gen_nodes = false;
    bool draw_path = false;

   private:
    int last_starting_node = -1;
    int last_ending_node = -1;

    std::unordered_map<uint32_t, int>* node_map;
    int generating_entity_index = 0;
    Volume volume;

    int generating_lod = 0;
    int generation_iteration = 0;

    void init() {
        delete node_map;
        node_map = new std::unordered_map<uint32_t, int>;
        delete buffer_a;
        delete buffer_b;
        buffer_a = new std::vector<NavNode>;
        buffer_b = new std::vector<NavNode>;
        nodes_mesh = buffer_a;
        generating_nodes = buffer_b;
    }
    std::vector<NavNode>* buffer_a = nullptr;
    std::vector<NavNode>* buffer_b = nullptr;

    int voxels_since_generation = -1;
    int voxels_to_lose = 100;

   public:
    glm::mat4 world_matrix = glm::mat4(1.f);
    bool nav_nodes_valid() {
        if (!nodes_mesh) return false;
        if (nodes_mesh->empty()) return false;
        return true;
    }
    void compute_normals(int iterations = std::numeric_limits<int>::max());
    void average_neighbor_normals(int iterations = std::numeric_limits<int>::max());
    void generate_mesh_over_time();
    void generate_mesh(int iterations = std::numeric_limits<int>::max());
    std::vector<int> find_path(const int starting_node_id, const int ending_node_id);
    int find_closest_node(const glm::vec3& position);
    int find_closest_node_cached(const glm::vec3& position, int hint);
    std::optional<glm::vec3> follow_path(glm::vec3 start, glm::vec3 end);
    void check_if_should_regenerate();
    void inspect();
};

}  // namespace tmt

TMT_COMPONENT_NAME(tmt::GenerateClosestNavMeshToThis, "Generate Closest Nav Mesh To This");
TMT_COMPONENT_SERIALIZE_EMPTY(tmt::GenerateClosestNavMeshToThis);
TMT_COMPONENT_INSPECT_EMPTY(tmt::GenerateClosestNavMeshToThis);
TMT_COMPONENT(tmt::NavMesh, "Navigation Mesh", (draw_nodes, draw_gen_nodes, draw_path));