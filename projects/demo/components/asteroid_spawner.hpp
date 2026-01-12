#pragma once
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources/voxel_volume.hpp"
#include <glm/glm.hpp>

class AsteroidSpawner : public tmt::GameComponent<AsteroidSpawner> {
   public:
    using GameComponent::GameComponent;
    static std::string_view get_name() { return "AsteroidSpawner"; }

    void start() override;
    void update(const tmt::FrameData& time) override;
    void end() override;

    void spawn_asteroids();

    void on_inspect(ImResponse& response) override;

    bool randomize_seed = true;
    uint64_t seed = 42;

    std::vector<tmt::ResourceRef<tmt::VoxelVolume>> asteroid_bodies;

    float min_radius = 50.0f;   // Inner circle radius (empty center)
    float max_radius = 200.0f;  // Outer circle radius
    int asteroid_count = 100;   // Total number of asteroids to spawn

    float min_height_offset = -10.0f;  // Minimum Y offset from ring plane
    float max_height_offset = 10.0f;   // Maximum Y offset from ring plane

    float min_scale_factor = 0.5f;  // Minimum scale multiplier
    float max_scale_factor = 2.0f;  // Maximum scale multiplier

    bool enable_random_rotation = true;  // Toggle random rotation

    bool use_3d_distribution = false;  // If true, distributes in sphere annulus instead of ring
    float radial_bias = 0.0f;          // -1.0 (inner bias) to 1.0 (outer bias), 0.0 = uniform

   private:
    std::vector<tmt::Entity> spawned_asteroids;

    tmt::Entity create_asteroid(const glm::vec3& position, const glm::vec3& rotation, float scale);

    float random_float(float min, float max);
    glm::vec3 random_rotation();

    glm::vec3 calculate_position(int index, int total);
};

TMT_OBJECT(
    AsteroidSpawner, (randomize_seed, seed, min_radius, max_radius, asteroid_count, min_height_offset, max_height_offset, min_scale_factor, max_scale_factor, enable_random_rotation,
                      use_3d_distribution, radial_bias, asteroid_bodies)
);