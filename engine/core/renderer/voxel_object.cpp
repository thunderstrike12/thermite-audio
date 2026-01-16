#include "voxel_object.hpp"

namespace tmt {

Hit VoxelObject::intersect(Ray ray, const float tmax) const {
    /* Transform the ray into the local space of the object */
    const glm::vec3 world_origin = ray.origin;
    ray.origin = glm::vec3(world_to_local * glm::vec4(ray.origin, 1.0f));
    ray.dir = glm::vec3(world_to_local * glm::vec4(ray.dir, 0.0f));
    ray.rcp_dir = 1.0f / ray.dir;

    /* Intersect the object AABB in local-space */
    const glm::vec3 half_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;
    const glm::vec3 t_to_min = (-half_extent - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_to_max = (half_extent - ray.origin) * ray.rcp_dir;
    const glm::vec3 t_min = glm::min(t_to_min, t_to_max);
    const glm::vec3 t_max = glm::max(t_to_min, t_to_max);
    const float t_near = glm::max(glm::max(glm::max(t_min.x, t_min.y), t_min.z), 0.0f);
    const float t_far = glm::min(glm::min(glm::min(t_max.x, t_max.y), t_max.z), tmax);
    if (t_near > t_far) return Hit(); /* miss */

    /* Calculate the ratio between the object size and its tree size */
    const glm::vec3 width_ratio = glm::vec3(size) * rcp_tree_width;

    /* Calculate the entry point for the tree traversal */
    const glm::vec3 entry_point = ray.origin + ray.dir * t_near;
    const glm::vec3 entry_uvw = (entry_point / half_extent) * 0.5f + 0.5f;
    ray.origin = (entry_uvw * width_ratio) + 1.0f; /* [1.0, 2.0) */

    const Svt64Hit local_hit = volume->blas->trace(ray);
    if (local_hit.pos.x >= 1e30f) return Hit(); /* miss */

    /* Calculate world-space hit point and distance */
    const glm::vec3 hit_uvw = (local_hit.pos - 1.0f) / width_ratio;
    const glm::vec3 hit_ndc = hit_uvw * 2.0f - 1.0f;
    const glm::vec3 hit_point = local_to_world * glm::vec4(hit_ndc * half_extent, 1.0f);
    const float hit_dist = distance(world_origin, hit_point);
    if (hit_dist > tmax) return Hit(); /* miss */

    /* Overwrite the current hit with the closer one */
    return Hit(hit_dist, {}, local_hit.coord);
}

}  // namespace tmt
