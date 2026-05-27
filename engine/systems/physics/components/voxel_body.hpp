#pragma once
#include "glm/gtc/quaternion.hpp"
#include "engine/systems/physics/physics_voxel_data.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/shared/aabb.hpp"
#include "engine/core/resources/voxel_volume.hpp"

namespace tmt {

struct VoxelBody {
    struct Box {
        glm::vec3 vertex[8] {};
    };

    struct Edge {
        glm::vec3 start = glm::vec3(0);
        glm::vec3 end = glm::vec3(0);
    };

    struct Axes {
        glm::vec3 axis[3] {};
    };

    bool initialized = false;

    // RigidBody properties
    float accumulated_forces = 1000.0f;
    glm::vec3 stored_velocity = glm::vec3(0);
    glm::vec3 stored_torque = glm::vec3(0);

    float inv_mass = -1.0f;
    glm::mat3 inv_inertia = glm::mat3(0);

    glm::vec3 com_local_offset = glm::vec3(0);
    glm::vec3 center_of_mass = glm::vec3(0);

    glm::vec3 velocity = glm::vec3(0);
    float linear_drag = 0.2f;
    glm::vec3 angular_velocity = glm::vec3(0);
    float angular_drag = 0.4f;

    float gravity = 0.0f;
    float density = 1.0f;

    glm::vec3 position = glm::vec3(0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);

    // For interpolation
    glm::vec3 prev_position = glm::vec3(0);
    glm::quat prev_rotation = glm::quat(1, 0, 0, 0);

    enum Type {
        STATIC,
        DYNAMIC,
        WANTS_SLEEP,
        SLEEPING,
    };

    Type type = DYNAMIC;

    float get_inv_mass() const;
    glm::mat3 get_inv_world_inertia() const;

    float get_mass() const { return type == STATIC ? 0.0f : 1.0f / inv_mass; }

    // Collider properties
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;

    uint32_t layer = 0;  // default layer

    Aabb aabb() const;
    Box get_local_bounds() const;
    Box get_world_bounds() const;
    Axes get_axes() const;
    std::array<Edge, 12> get_world_edges() const;
};

}  // namespace tmt

TMT_COMPONENT(tmt::VoxelBody, "Voxel Body", (type, gravity, layer));
