#include "voxel_object.hpp"
#include "glm/gtx/norm.hpp"

namespace tmt {

Hit VoxelObject::intersect(Ray ray, const float tmax) const {
    if (!volume->blas) return Hit();

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
    if (local_hit.pos.x >= 1e30f) return Hit();    /* miss */

    /* Calculate world-space hit point and distance */
    const glm::vec3 hit_uvw = (local_hit.pos - 1.0f) / width_ratio;
    const glm::vec3 hit_ndc = hit_uvw * 2.0f - 1.0f;
    const glm::vec3 hit_point = local_to_world * glm::vec4(hit_ndc * half_extent, 1.0f);
    const float hit_dist = distance(world_origin, hit_point);
    if (hit_dist > tmax) return Hit(); /* miss */

    /* Calculate world-space hit normal */
    glm::vec3 hit_normal = glm::vec3(local_to_world * glm::vec4(local_hit.normal, 0.0f));
    if (glm::length(hit_normal) > 1.0f) {
        const glm::vec3 entry_ndc = entry_uvw * 2.0f - 1.0f;
        const glm::vec3 entry_abs = glm::abs(entry_ndc);
        const float max_axis = glm::max(entry_abs.x, glm::max(entry_abs.y, entry_abs.z));
        hit_normal = glm::vec3(
            entry_abs.x == max_axis ? glm::sign(entry_ndc.x) : 0.0f, entry_abs.y == max_axis ? glm::sign(entry_ndc.y) : 0.0f, entry_abs.z == max_axis ? glm::sign(entry_ndc.z) : 0.0f
        );
    }

    /* Overwrite the current hit with the closer one */
    return Hit(hit_dist, {}, local_hit.coord, glm::normalize(hit_normal));
}

std::vector<std::pair<float, glm::uvec3>> VoxelObject::sphere_overlap(const glm::vec3& center, float radius) const {
    if (!volume->blas) return {};

    glm::vec3 local_sphere_center = glm::vec3(world_to_local * glm::vec4(center, 1.0f));

    // Sphere AABB in local space
    const glm::vec3 sphere_min = local_sphere_center - glm::vec3(radius);
    const glm::vec3 sphere_max = local_sphere_center + glm::vec3(radius);

    const glm::vec3 half_extent = glm::vec3(size) * UNITS_PER_VOXEL * 0.5f;

    auto local_to_voxel = [&](const glm::vec3& p) -> glm::ivec3 {
        glm::vec3 f = (p + half_extent) / UNITS_PER_VOXEL;
        return glm::ivec3(glm::floor(f));
    };

    glm::ivec3 min_coord = local_to_voxel(sphere_min);
    glm::ivec3 max_coord = local_to_voxel(sphere_max);

    min_coord = glm::clamp(min_coord, glm::ivec3(0), glm::ivec3(size) - 1);
    max_coord = glm::clamp(max_coord, glm::ivec3(0), glm::ivec3(size) - 1);

    std::vector<std::pair<float, glm::uvec3>> hits;

    for (int x = min_coord.x; x <= max_coord.x; ++x) {
        for (int y = min_coord.y; y <= max_coord.y; ++y) {
            for (int z = min_coord.z; z <= max_coord.z; ++z) {
                if (!volume->blas->get_voxel(x, y, z)) continue;

                // sphere point overlap because fast and easy
                const glm::vec3 voxel_center = -half_extent + (glm::vec3(x, y, z) + glm::vec3(0.5f)) * UNITS_PER_VOXEL;
                float sqr_dist = glm::length2(local_sphere_center - voxel_center);

                bool overlap = sqr_dist < (radius * radius);
                if (!overlap) continue;

                hits.emplace_back(std::make_pair(sqr_dist, glm::vec3 { x, y, z }));
            }
        }
    }

    return hits;
}

}  // namespace tmt
