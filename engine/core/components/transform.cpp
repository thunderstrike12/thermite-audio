#include "transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

void Transform::set_local_position(const glm::vec3& pos) {
    local_position = pos;
    mark_dirty();
}

/* Quat */
void Transform::set_local_rotation(const glm::quat& rot) {
    local_rotation = rot;
    local_eulers = glm::eulerAngles(rot);
    mark_dirty();
}

/* Euler */
void Transform::set_local_rotation(const glm::vec3& rot) {
    local_rotation = glm::quat(rot);
    local_eulers = rot;
    mark_dirty();
}

/* Axis-Angle */
void Transform::set_local_rotation(const glm::vec3& axis, float angle) {
    local_rotation = glm::angleAxis(angle, glm::normalize(axis));
    local_eulers = glm::eulerAngles(local_rotation);
    mark_dirty();
}

void Transform::set_local_scale(const glm::vec3& scale) {
    local_scale = scale;
    mark_dirty();
}

const glm::vec3& Transform::get_local_position() const {
    return local_position;
}

const glm::quat& Transform::get_local_rotation() const {
    return local_rotation;
}

const glm::vec3& Transform::get_local_scale() const {
    return local_scale;
}

const glm::vec3& Transform::get_local_eulers() const {
    return local_eulers;
}

void Transform::set_world_position(const glm::vec3& pos) {
    if (has_parent()) {
        auto& parent_transform = engine.ecs.get_component<Transform>(parent);

        const glm::mat4 parent_world_inverse = glm::inverse(parent_transform.get_world_matrix());
        const glm::vec4 local_pos = parent_world_inverse * glm::vec4(pos, 1.0f);

        local_position = glm::vec3(local_pos);
    } else {
        local_position = pos;
    }
    mark_dirty();
}

/* Quat */
void Transform::set_world_rotation(const glm::quat& rot) {
    if (has_parent()) {
        auto& parent_transform = engine.ecs.get_component<Transform>(parent);
        const glm::quat parent_world_rot = parent_transform.get_world_rotation();
        local_rotation = glm::inverse(parent_world_rot) * rot;
    } else {
        local_rotation = rot;
    }
    local_eulers = glm::eulerAngles(local_rotation);
    mark_dirty();
}

/* Euler */
void Transform::set_world_rotation(const glm::vec3& rot) {
    set_world_rotation(glm::quat(rot));
}

/* Axis-Angle */
void Transform::set_world_rotation(const glm::vec3& axis, float angle) {
    set_world_rotation(glm::angleAxis(angle, glm::normalize(axis)));
}

void Transform::set_world_scale(const glm::vec3& scale) {
    if (has_parent()) {
        auto& parent_transform = engine.ecs.get_component<Transform>(parent);
        const glm::vec3 parent_world_scale = parent_transform.get_world_scale();
        // local_scale = scale / parent_world_scale; // division by zero possible

        local_scale = glm::vec3(
            parent_world_scale.x != 0.0f ? scale.x / parent_world_scale.x : scale.x, parent_world_scale.y != 0.0f ? scale.y / parent_world_scale.y : scale.y,
            parent_world_scale.z != 0.0f ? scale.z / parent_world_scale.z : scale.z
        );

    } else {
        local_scale = scale;
    }
    mark_dirty();
}

glm::vec3 Transform::get_world_position() const {
    if (has_parent() == false) {
        return local_position;
    }
    return glm::vec3(get_world_matrix()[3]);
}

glm::quat Transform::get_world_rotation() const {
    if (has_parent() == false) {
        return local_rotation;
    }
    const glm::mat4& wm = get_world_matrix();

    // Extract scale from matrix columns
    glm::vec3 scale(glm::length(glm::vec3(wm[0])), glm::length(glm::vec3(wm[1])), glm::length(glm::vec3(wm[2])));

    // Build pure rotation matrix by removing scale
    glm::mat4 rotation_matrix = wm;
    if (scale.x != 0.0f) rotation_matrix[0] /= scale.x;
    if (scale.y != 0.0f) rotation_matrix[1] /= scale.y;
    if (scale.z != 0.0f) rotation_matrix[2] /= scale.z;

    // Now extract quaternion from pure rotation matrix
    return glm::normalize(glm::quat_cast(rotation_matrix));
}

glm::vec3 Transform::get_world_scale() const {
    if (has_parent() == false) {
        return local_scale;
    }
    const glm::mat4& wm = get_world_matrix();
    return glm::vec3(glm::length(glm::vec3(wm[0])), glm::length(glm::vec3(wm[1])), glm::length(glm::vec3(wm[2])));
}

void Transform::translate(const glm::vec3& delta) {
    set_local_position(local_position + delta);
}

/* Quat */
void Transform::rotate_local(const glm::quat& delta) {
    set_local_rotation(local_rotation * delta);
}

/* Euler */
void Transform::rotate_local(const glm::vec3& euler_delta) {
    set_local_rotation(local_rotation * glm::quat(euler_delta));
}

/* Axis-Angle */
void Transform::rotate_local(const glm::vec3& axis, float angle) {
    set_local_rotation(local_rotation * glm::angleAxis(angle, glm::normalize(axis)));
}

void Transform::rotate_world(const glm::quat& delta) {
    set_local_rotation(delta * local_rotation);
}

void Transform::rotate_world(const glm::vec3& euler_delta) {
    set_local_rotation(glm::quat(euler_delta) * local_rotation);
}

void Transform::rotate_world(const glm::vec3& axis, float angle) {
    set_local_rotation(glm::angleAxis(angle, glm::normalize(axis)) * local_rotation);
}

void Transform::scale(const glm::vec3& factor) {
    set_local_scale(local_scale * factor);
}

glm::vec3 Transform::get_forward() const {
    return glm::normalize(get_world_rotation() * glm::vec3(0, 0, 1));
}

glm::vec3 Transform::get_up() const {
    return glm::normalize(get_world_rotation() * glm::vec3(0, 1, 0));
}

glm::vec3 Transform::get_right() const {
    return glm::normalize(get_world_rotation() * glm::vec3(1, 0, 0));
}

void Transform::look_at(const glm::vec3& target, const glm::vec3& up) {
    glm::vec3 world_pos = get_world_position();
    glm::mat4 lookat_matrix = glm::lookAt(world_pos, target, up);
    glm::quat rotation = glm::quat_cast(glm::inverse(lookat_matrix));
    set_world_rotation(rotation);
}

void Transform::set_local_matrix(const glm::mat4& matrix) {
    /* unused */
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(matrix, local_scale, local_rotation, local_position, skew, perspective);
    local_eulers = glm::eulerAngles(local_rotation);
    mark_dirty();
}

void Transform::set_world_matrix(const glm::mat4& matrix) {
    if (has_parent()) {
        auto& parent_transform = engine.ecs.get_component<Transform>(parent);
        const glm::mat4 parent_world_inverse = glm::inverse(parent_transform.get_world_matrix());
        const glm::mat4 local_matrix = parent_world_inverse * matrix;

        /* unused */
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(local_matrix, local_scale, local_rotation, local_position, skew, perspective);
    } else {
        /* unused */
        glm::vec3 skew;
        glm::vec4 perspective;

        glm::decompose(matrix, local_scale, local_rotation, local_position, skew, perspective);
    }
    local_eulers = glm::eulerAngles(local_rotation);

    mark_dirty();
}

const glm::mat4& Transform::get_world_matrix() const {
    calculate_world_matrix();
    return world_matrix;
}

void Transform::set_parent(Entity new_parent) {
    Entity self = get_self();
    if (new_parent == self) {
        throw std::runtime_error("Transform::set_parent: Cannot set self as parent");
    }

    // Cycle detection
    int iterations = 0;
    constexpr static int MAX_ITERATIONS = 1000;
    Entity current = new_parent;
    while (current != entt::null) {
        if (current == self) {
            throw std::runtime_error("Circular parent hierarchy detected");
        }

        if (iterations >= MAX_ITERATIONS) {
            throw std::runtime_error("Transform::set_parent: Max hierarchy depth exceeded during cycle detection");
        }

        auto& transform = engine.ecs.get_component<Transform>(current);
        current = transform.parent;
        iterations++;
    }

    // If unparenting or changing parent, preserve world transform
    glm::vec3 world_pos;
    glm::quat world_rot;
    glm::vec3 world_scale;
    const bool unparenting = (new_parent == entt::null && has_parent());
    const bool changing_parent = (new_parent != entt::null && parent != entt::null && new_parent != parent);

    if (unparenting || changing_parent) {
        world_pos = get_world_position();
        world_rot = get_world_rotation();
        world_scale = get_world_scale();
        world_scale = glm::round(world_scale * 100000.0f) / 100000.0f;  // Avoid floating point precision issues
    }

    if (has_parent()) {
        auto& old_parent_transform = engine.ecs.get_component<Transform>(parent);
        old_parent_transform.children.erase(get_self());
    }

    parent = new_parent;

    if (has_parent()) {
        auto& new_parent_transform = engine.ecs.get_component<Transform>(parent);
        new_parent_transform.children.insert(get_self());

        const bool parent_is_disabled = engine.ecs.is_disabled(new_parent);
        if (parent_is_disabled) {
            engine.ecs.disable(self, false);
        } else {
            if (!engine.ecs.has_component<DisableFlag>(self)) {
                engine.ecs.enable(self, false);
            }
        }
    }

    if (unparenting || changing_parent) {
        set_world_position(world_pos);
        set_world_rotation(world_rot);
        set_world_scale(world_scale);
    }

    mark_dirty();
}

void Transform::clear_parent() {
    set_parent(entt::null);
}

bool Transform::has_parent() const {
    return parent != entt::null;
}

Entity Transform::get_parent() const {
    return parent;
}

void Transform::add_child(Entity child) {
    if (child == get_self()) {
        throw std::runtime_error("Transform::add_child: Cannot add self as child");
    }

    children.insert(child);

    auto& child_transform = engine.ecs.get_component<Transform>(child);
    child_transform.set_parent(get_self());
}

void Transform::remove_child(Entity child) {
    if (children.erase(child) > 0) {
        auto& child_transform = engine.ecs.get_component<Transform>(child);
        child_transform.parent = entt::null;  // Clear the parent reference
        child_transform.mark_dirty();
    }
}

std::set<Entity> Transform::get_all_parents() const {
    std::set<Entity> result;
    Entity current_parent = parent;
    while (current_parent != entt::null) {
        result.insert(current_parent);
        const auto& transform = engine.ecs.get_component<Transform>(current_parent);
        current_parent = transform.parent;
    }
    return result;
}

bool Transform::validate_scale() const {
    Entity current_parent = parent;
    while (current_parent != entt::null) {
        const auto& transform = engine.ecs.get_component<Transform>(current_parent);
        current_parent = transform.parent;

        if (glm::any(glm::epsilonEqual(transform.get_local_scale(), glm::vec3(0.0f), 0.001f))) {
            return false;
        }
    }
    return true;
}

bool Transform::has_children() const {
    return !children.empty();
}

const std::set<Entity>& Transform::get_children() const {
    return children;
}

std::set<Entity> Transform::get_all_children() const {
    std::set<Entity> result = get_children();
    for (const auto child : children) {
        const auto& transform = tmt::engine.ecs.get_component<Transform>(child);
        result.merge(transform.get_all_children());
    }
    return result;
}

void Transform::mark_dirty() {
    if (is_dirty) {
        return;
    }

    is_dirty = true;
    for (const Entity& child : children) {
        auto& child_transform = engine.ecs.get_component<Transform>(child);
        child_transform.mark_dirty();
    }
}

void Transform::calculate_world_matrix() const {
    if (is_dirty) {
        const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), local_position);
        const glm::mat4 rotation = glm::toMat4(local_rotation);
        const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), local_scale);

        world_matrix = translation * rotation * scale;

        if (parent != entt::null) {
            auto& parent_transform = engine.ecs.get_component<Transform>(parent);
            world_matrix = parent_transform.get_world_matrix() * world_matrix;
        }
        is_dirty = false;
    }
}

Entity Transform::get_self() const {
    return engine.ecs.get_entity(*this);
}

}  // namespace tmt