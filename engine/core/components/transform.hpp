#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/core/ecs.hpp"
#include "engine/tools/fmt_helpers.hpp"

namespace tmt {

struct Transform {
    /* Local */
    void set_local_position(const glm::vec3& pos);
    void set_local_rotation(const glm::quat& rot);               /* quat */
    void set_local_rotation(const glm::vec3& euler);             /* euler */
    void set_local_rotation(const glm::vec3& axis, float angle); /* axis-angle */
    void set_local_scale(const glm::vec3& scale);

    const glm::vec3& get_local_position() const;
    const glm::quat& get_local_rotation() const;
    const glm::vec3& get_local_scale() const;

    /* World */
    void set_world_position(const glm::vec3& pos);
    void set_world_rotation(const glm::quat& rot);               /* quat */
    void set_world_rotation(const glm::vec3& euler);             /* euler */
    void set_world_rotation(const glm::vec3& axis, float angle); /* axis-angle */
    void set_world_scale(const glm::vec3& scale);

    glm::vec3 get_world_position() const;
    glm::quat get_world_rotation() const;
    glm::vec3 get_world_scale() const;

    /* Helpers */
    void translate(const glm::vec3& delta);

    void rotate_local(const glm::quat& delta);             /* quat */
    void rotate_local(const glm::vec3& euler_delta);       /* euler */
    void rotate_local(const glm::vec3& axis, float angle); /* axis-angle */

    void rotate_world(const glm::quat& delta);             /* quat */
    void rotate_world(const glm::vec3& euler_delta);       /* euler */
    void rotate_world(const glm::vec3& axis, float angle); /* axis-angle */

    void scale(const glm::vec3& factor);

    glm::vec3 get_forward() const;
    glm::vec3 get_up() const;
    glm::vec3 get_right() const;

    void look_at(const glm::vec3& target, const glm::vec3& up);

    /* Matrix */
    void set_world_matrix(const glm::mat4& matrix);

    const glm::mat4& get_world_matrix() const;

    /* Hierarchy */
    void set_parent(Entity new_parent);
    void clear_parent();

    bool has_parent() const;
    Entity get_parent() const;

    void add_child(const Entity child);
    void remove_child(const Entity child);

    const std::set<Entity>& get_children() const;

   private:
    glm::vec3 local_position {0.0f, 0.0f, 0.0f};
    glm::quat local_rotation {1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 local_scale {1.0f, 1.0f, 1.0f};

    mutable glm::mat4 world_matrix {glm::identity<glm::mat4>()};

    mutable bool is_dirty = true;

    Entity parent {entt::null};
    std::set<Entity> children;

    void mark_dirty();

    /* Recalculate world matrix if dirty */
    void calculate_world_matrix() const;

    Entity get_self() const;
};

}  // namespace tmt

FMT_LOGGING(tmt::Transform, "Position: {}, Rotation: {}, Scale: {}", obj.get_world_position(), obj.get_world_rotation(), obj.get_world_scale());