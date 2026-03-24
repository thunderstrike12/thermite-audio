#pragma once
#include "engine/core/system.hpp"
#include "engine/core/components/transform.hpp"

#include "collision.hpp"
#include "constraint_solver.hpp"
#include "physics_layers.hpp"
#include "components/voxel_body.hpp"
#include "engine/core/components/voxel_renderer.hpp"
#include "engine/core/renderer/voxel_object.hpp"
#include "engine/shared/bvh2.hpp"

namespace tmt {

using PhysicsGroup = decltype(std::declval<Ecs>().group<VoxelBody>(entt::get<Transform, VoxelRenderer>));

class Physics : public ISystem {
   public:
    Physics() = default;

    // Inherited via ISystem
    std::string get_name() override { return "Physics System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

    const Bvh2<VoxelObject>& get_bvh() { return bvh; }
    const std::vector<Entity>& get_entities() { return entities; }

   private:
    // Simulation data
    float simulation_time_accum = 0.0f;
    int max_contacts = 256;
    bool simulating = true;
    ConstraintSolver solver = {};
    Bvh2<VoxelObject> bvh {};
    std::vector<Entity> entities {};

    // Layers
    PhysicsLayers physics_layers;

    // Simulation functions
    bool sat_early_out(const VoxelBody& vb_a, const VoxelBody& vb_b);
    float ray_aabb(const glm::vec3& min, const glm::vec3& max, const glm::vec3& ro, const glm::vec3& rd) const;
    bool is_separated(const glm::vec3& axis, const VoxelBody::Box& box_a, const VoxelBody::Box& box_b, float& overlap) const;
    void apply_velocities() const;
    void generate_constraints();
    void generate_constraint(int index, const PhysicsGroup& group);
    void compare_trees(
        tmt::Svt64* tree_a, const glm::vec3& center_a, const glm::quat& rotation_a, float half_extent_a, tmt::Svt64* tree_b, const glm::vec3& center_b, const glm::quat& rotation_b,
        float half_extent_b, Collision& coll
    );

   public:
    // Static utility functions
    static void initialize_voxel_body(VoxelBody& vb, VoxelVolume& volume);
    static void add_force(VoxelBody& vb, const glm::vec3& force);
    static void add_torque(VoxelBody& vb, const glm::vec3& torque);
    static void add_force_at_position(VoxelBody& vb, const glm::vec3& force, const glm::vec3& position);
    static void set_position(VoxelBody& vb, const glm::vec3& position);
    static void set_rotation(VoxelBody& vb, const glm::quat& rotation);

    PhysicsLayers& layers() { return physics_layers; }
    // Raycast against layers in the layer_mask
    Hit raycast(const Ray& ray, uint32_t layer_mask) const;
    // Overlap against layers in layer_mask
    std::vector<uint32_t> overlap(const Aabb& aabb, uint32_t layer_mask) const;
    std::vector<std::pair<Entity, std::vector<std::pair<float, glm::uvec3>>>> overlap_sphere(const glm::vec3& center, float radius, uint32_t layer_mask);

    static void recalculate_physics_data(VoxelBody& vb, VoxelVolume& volume);
    static void recalculate_surface_normals(VoxelRenderer& renderer);
    static void recalculate_surface_normals(VoxelVolume& volume);
};

}  // namespace tmt
