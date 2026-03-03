#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>

namespace tmt {

/**
 * Constant representing the maximum number of physics layers supported.
 */
constexpr uint32_t MAX_LAYERS = 32;

/**
 * Class PhysicsLayers
 *
 * Manages physics collision layers.
 *
 * Responsibilities:
 *   - Store layer names
 *   - Manage a collision matrix indicating which layers can collide
 *   - Add, remove, or rename layers safely
 *   - Load and save layer configuration from/to disk
 *
 * Notes:
 *   - Removing a layer does not shift indices of other layers to prevent dangling references in voxel bodies or other components.
 *   - Default layers (0 = "Default" for example) should always exist; removing them clears the name but preserves the index.
 */
class PhysicsLayers {
   public:
    // Constructor initializes an empty collision matrix with all false
    PhysicsLayers();

    // Enable or disable collisions between two layers
    void enable_collision(uint32_t a, uint32_t b, bool enable);

    // Returns true if two layers can collide
    bool can_collide(uint32_t a, uint32_t b) const;

    // Add a new layer with a name, returns its index
    uint32_t add_layer(const std::string& name);

    /**
     * Remove a layer by index.
     *
     * Notes:
     *   - The index is not shifted, so other objects referencing indices remain valid.
     *   - The layer's name is cleared and collisions are disabled.
     */
    void remove_layer(uint32_t layer);

    // Get the name of a layer by index
    const std::string& get_layer_name(uint32_t layer) const;

    // Rename a layer by index
    void set_layer_name(uint32_t layer, const std::string& name);

    // Returns the current number of layers (including removed ones with empty names)
    uint32_t get_layer_count() const { return layer_count; }

    // Load layers and collision matrix from JSON file
    void load();

    // Save layers and collision matrix to JSON file
    void save() const;

   private:
    // Symmetric collision matrix: collision_matrix[a][b] = collision_matrix[b][a]
    bool collision_matrix[MAX_LAYERS][MAX_LAYERS];

    // Array of layer names, empty string indicates unused/removed layer
    std::string layer_names[MAX_LAYERS];

    // Number of layers currently defined (max = MAX_LAYERS)
    uint32_t layer_count = 0;
};

}  // namespace tmt
