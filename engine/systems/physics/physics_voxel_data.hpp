#pragma once
#include "glm/vec3.hpp"
#include <vector>

namespace tmt {

class VoxelInstance;

static const glm::ivec3 NORMAL_LUT[27] {
    {0, 0, 0},
    // Cardinal directions 1-7
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {-1, 0, 0},
    {0, -1, 0},
    {0, 0, -1},
    // Edge directions 7-19
    {1, 1, 0},
    {1, 0, 1},
    {0, 1, 1},
    {-1, 1, 0},
    {-1, 0, 1},
    {0, -1, 1},
    {1, -1, 0},
    {1, 0, -1},
    {0, 1, -1},
    {-1, -1, 0},
    {-1, 0, -1},
    {0, -1, -1},
    // Corner directions 19-27
    {1, 1, 1},
    {1, 1, -1},
    {1, -1, 1},
    {1, -1, -1},
    {-1, 1, 1},
    {-1, 1, -1},
    {-1, -1, 1},
    {-1, -1, -1}
};

enum PhysicsVoxelType : uint8_t { EMPTY, CORNER, EDGE, FACE, INSIDE };

struct PhysicsVoxel {
    PhysicsVoxelType type : 3;
    uint8_t normal_index : 5;

    const glm::ivec3 get_normal() const { return NORMAL_LUT[normal_index]; }
};

struct PhysicsVoxelData {
    PhysicsVoxelData() = default;

    inline PhysicsVoxel get_voxel(size_t x, size_t y, size_t z) const { return data[x + size.x * y + size.x * size.y * z]; }
    inline bool is_in_range(size_t x, size_t y, size_t z) const { return x < size.x && y < size.y && z < size.z; }

    const std::vector<PhysicsVoxel>& get_data() const { return data; }
    const glm::uvec3& get_size() const { return size; }
    //std::vector<PhysicsVoxel>& set_data() { return data; }
    //glm::uvec3& set_size() { return size; }

    void upload_voxel_texture_if_dirty() {}

   private:
    std::vector<PhysicsVoxel> data = {};
    glm::uvec3 size = {};
};

}  // namespace tmt