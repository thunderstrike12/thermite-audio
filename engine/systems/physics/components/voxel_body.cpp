#include "voxel_body.hpp"
#include <glm/gtx/quaternion.hpp>
#include <array>

float tmt::VoxelBody::get_inv_mass() const {
    return type == STATIC ? 0 : inv_mass;
}

glm::mat3 tmt::VoxelBody::get_inv_world_inertia() const {
    if (type == STATIC) return glm::mat3(0);

    const glm::mat3 r = glm::toMat3(rotation);
    return r * inv_inertia * glm::transpose(r);
}

tmt::Aabb tmt::VoxelBody::aabb() const {
    Axes axes = get_axes();
    glm::vec3 world_half_extends = glm::vec3(0);
    for (int i = 0; i < 3; ++i) {
        world_half_extends.x += std::abs(axes.axis[i].x) * (width * 0.5f);
        world_half_extends.y += std::abs(axes.axis[i].y) * (height * 0.5f);
        world_half_extends.z += std::abs(axes.axis[i].z) * (depth * 0.5f);
    }

    // Return AABB min and max
    return { position - world_half_extends, position + world_half_extends };
}

tmt::VoxelBody::Box tmt::VoxelBody::get_local_bounds() const {
    // Define local corners of the box
    const float half_width = width / 2.0f;
    const float half_height = height / 2.0f;
    const float half_depth = depth / 2.0f;
    tmt::VoxelBody::Box box = {
        glm::vec3(-half_width, -half_height, -half_depth),  // 0
        glm::vec3(half_width, -half_height, -half_depth),   // 1
        glm::vec3(half_width, half_height, -half_depth),    // 2
        glm::vec3(-half_width, half_height, -half_depth),   // 3
        glm::vec3(-half_width, -half_height, half_depth),   // 4
        glm::vec3(half_width, -half_height, half_depth),    // 5
        glm::vec3(half_width, half_height, half_depth),     // 6
        glm::vec3(-half_width, half_height, half_depth),    // 7
    };
    return box;
}

tmt::VoxelBody::Box tmt::VoxelBody::get_world_bounds() const {
    tmt::VoxelBody::Box box = get_local_bounds();

    // Transform vertices to world space
    for (size_t i = 0; i < 8; i++) box.vertex[i] = (rotation * box.vertex[i]) + position;

    return box;
}

tmt::VoxelBody::Axes tmt::VoxelBody::get_axes() const {
    Axes axes = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
    };

    for (size_t i = 0; i < 3; i++) axes.axis[i] = glm::normalize(rotation * axes.axis[i]);

    return axes;
}

std::array<tmt::VoxelBody::Edge, 12> tmt::VoxelBody::get_world_edges() const {
    auto box = get_world_bounds();
    return std::array<tmt::VoxelBody::Edge, 12> { {
        // X axis edges
        { box.vertex[0], box.vertex[1] },
        { box.vertex[2], box.vertex[3] },
        { box.vertex[4], box.vertex[5] },
        { box.vertex[6], box.vertex[7] },

        // Y axis edges
        { box.vertex[0], box.vertex[3] },
        { box.vertex[1], box.vertex[2] },
        { box.vertex[4], box.vertex[7] },
        { box.vertex[5], box.vertex[6] },

        // Z axis edges
        { box.vertex[0], box.vertex[4] },
        { box.vertex[1], box.vertex[5] },
        { box.vertex[2], box.vertex[6] },
        { box.vertex[3], box.vertex[7] },
    } };
}
