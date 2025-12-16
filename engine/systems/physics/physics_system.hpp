#pragma once
#include "core/system.hpp"
#include "core/components/transform.hpp"

#include "collision.hpp"
#include "constraint_solver.hpp"
#include "components/voxel_body.hpp"
#include "core/renderer/voxel_object.hpp"
#include "engine/shared/bvh2.hpp"
// #include "components/rigidbody.hpp"
// #include "components/voxel_collider.hpp"

namespace tmt {

class Physics : public ISystem {
   public:
    Physics() = default;

    // Inherited via ISystem
    std::string get_name() override { return "Physics System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

   private:
    // Simulation data
    float simulation_time_accum = 0.0f;
    int max_contacts = 256;
    bool simulating = true;
    ConstraintSolver solver = {};
    Bvh2<VoxelObject> bvh {};

    // Simulation functions
    void generate_voxel_constraints_range(const int index, const int size);
    bool sat_early_out(const VoxelBody& vb_a, const VoxelBody& vb_b);
    void get_overlap(const VoxelBody& vb_a, const VoxelBody& vb_b, glm::ivec3& min, glm::ivec3& max);
    float ray_aabb(const glm::vec3& min, const glm::vec3& max, const glm::vec3& ro, const glm::vec3& rd) const;
    bool is_separated(const glm::vec3& axis, const VoxelBody::Box& box_a, const VoxelBody::Box& box_b, float& overlap) const;
    void check_neighbors(const VoxelBody& vb_a, const VoxelBody& vb_b, const size_t x, const size_t y, const size_t z, Collision& coll) const;
    void apply_velocities() const;

   public:
    // Static utility functions
    static void initialize_voxel_body(VoxelBody& vb);
    static void add_force(VoxelBody& vb, const glm::vec3& force);
    static void add_torque(VoxelBody& vb, const glm::vec3& torque);
    static void add_force_at_position(VoxelBody& vb, const glm::vec3& force, const glm::vec3& position);
};

}  // namespace tmt
