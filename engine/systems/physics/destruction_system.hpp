#pragma once
#include "engine/core/system.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/physics/components/destructable.hpp"

namespace tmt {

class Stencil;
class VoxelVolume;

class Destruction : public ISystem {
   public:
    Destruction() = default;

    struct FloodStackEntry {
        uint32_t node_index;
        uint64_t edge_mask;  // 0 = high-level path, non-zero = low-level path
    };

    // Inherited via ISystem
    std::string get_name() override { return "Destruction System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

    void destroy_voxels(Entity entity, const Stencil* stencil, glm::ivec3 offset = glm::ivec3(0));
    void destroy_voxel(Entity entity, glm::uvec3 pos);

    static uint32_t pos_to_node_id(uint32_t x, uint32_t y, uint32_t z) { return (x + (y << 10) + (z << 20)); };

    static void generate_connection_graph(Destructible& graph, Svt64* tree);

   private:
    // Returns true if we can skip separation
    bool seperation_early_out(VoxelVolume* volume, const glm::uvec3& pos, const std::vector<glm::uvec3>& neighbors);

    void fill_edge_indices(std::vector<glm::uvec3>& edge_indices, Entity entity, const Stencil* stencil, glm::ivec3 offset);
    // std::vector<Entity> find_seperations(Entity entity, const std::vector<glm::uvec3>& edge_indices, const Stencil* stencil, glm::ivec3 offset);
    std::vector<Entity> find_seperations(Entity entity, const std::vector<glm::uvec3>& edge_indices, Destructible& graph);
    void regenerate_connection_graph(Destructible& graph, Svt64* tree, const Stencil* stencil, glm::ivec3 offset);
    void update_connection_graph_at(Destructible& graph, Svt64* tree, const glm::uvec3& pos);
    // FloodStackEntry separation_flood(Destructable& graph, DestructionNode& current_node, uint8_t id, Svt64* tree);
};

}  // namespace tmt