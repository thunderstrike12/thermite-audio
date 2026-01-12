#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/resources/voxel_volume.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

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
    ~NavMesh() = default;

    std::vector<Node> nodes;
    std::vector<int> path;

    tmt::ResourceRef<tmt::VoxelVolume> voxel_volume;
    int lod_level = 0;

    void generate_mesh();
    std::vector<int> find_path(const int starting_node_id, const int ending_node_id);
    int find_closest_node(const glm::vec3& position);
    std::optional<glm::vec3> follow_path(glm::vec3 start, glm::vec3 end);
    void inspect();
};
}  // namespace tmt

TMT_COMPONENT(tmt::NavMesh, "Navigation Mesh", (voxel_volume, lod_level));