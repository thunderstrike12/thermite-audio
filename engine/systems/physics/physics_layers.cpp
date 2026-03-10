#include "physics_layers.hpp"

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

/**
 * Constructor for PhysicsLayers
 *
 * Responsibilities:
 *   - Initialize all collisions to false
 *   - Sets layer count to 0
 *   - Enables raycast for all layers
 */
PhysicsLayers::PhysicsLayers() {
    // Initialize collision matrix to false
    for (uint32_t i = 0; i < MAX_LAYERS; ++i) {
        for (uint32_t j = 0; j < MAX_LAYERS; ++j) {
            collision_matrix[i][j] = false;
        }
        // raycast_enabled[i] = true;  // default: ray hits everything
    }
}

/**
 * Enable or disable collision between two layers
 *
 * Notes:
 *   - Symmetric: changing a -> b also changes b -> s
 *   - Ignores invalid indices
 */
void PhysicsLayers::enable_collision(uint32_t a, uint32_t b, bool enable) {
    if (a >= MAX_LAYERS || b >= MAX_LAYERS) return;

    collision_matrix[a][b] = enable;
    collision_matrix[b][a] = enable;  // enforce symmetry
}

/**
 * Check if two layers can collide
 *
 * It will always check smaller vs bigger, since I only fill in
 * the opper triangle if the collision matrix.
 *
 * Returns:
 *   - true if collision enabled, false otherwise
 */
bool PhysicsLayers::can_collide(uint32_t a, uint32_t b) const {
    if (a >= MAX_LAYERS || b >= MAX_LAYERS) return false;

    // ensure we always access upper triangle
    if (a > b) std::swap(a, b);

    return collision_matrix[a][b];
}

/**
 * Add a new physics layer
 *
 * Responsibilities:
 *   - Assign new layer name
 *   - Default collision with all existing layers
 * Safety:
 *   - Will not exceed MAX_LAYERS
 * Returns:
 *   - Index of the new layer on success
 *   - UINT32_MAX if max layers reached (layer not added)
 */
uint32_t PhysicsLayers::add_layer(const std::string& name) {
    // Safety check: prevent exceeding MAX_LAYERS
    if (layer_count >= MAX_LAYERS) {
        Log::warn("Cannot add new physics layer '{}': maximum of {} reached", name, MAX_LAYERS);
        return UINT32_MAX;  // sentinel value
    }

    uint32_t new_layer = layer_count;
    layer_names[new_layer] = name;

    // Default collisions with existing layers
    for (uint32_t i = 0; i <= new_layer; ++i) {
        collision_matrix[new_layer][i] = true;
        collision_matrix[i][new_layer] = true;
    }

    layer_count++;
    return new_layer;
}

/**
 * Remove a physics layer safely
 *
 * Responsibilities:
 *   - Clears all collisions involving this layer
 *   - Clears its name
 *   - Does NOT shift other indices to prevent invalid references
 */
void PhysicsLayers::remove_layer(uint32_t layer) {
    if (layer >= layer_count) return;

    // Disable all collisions involving this layer
    for (uint32_t i = 0; i < MAX_LAYERS; ++i) {
        collision_matrix[layer][i] = false;
        collision_matrix[i][layer] = false;
    }

    layer_names[layer].clear();
}

/**
 * Get layer name by index
 *
 * Throws out_of_range if invalid
 */
const std::string& PhysicsLayers::get_layer_name(uint32_t layer) const {
    if (layer >= MAX_LAYERS) throw std::out_of_range("Invalid physics layer");

    return layer_names[layer];
}

/**
 * Set layer name by index
 *
 * Ignores invalid indices
 */
void PhysicsLayers::set_layer_name(uint32_t layer, const std::string& name) {
    if (layer >= MAX_LAYERS) return;
    layer_names[layer] = name;
}

/**
 * Load physics layers from JSON file
 *
 * Responsibilities:
 *   - Load layer_count, names, and collision matrix
 *   - Safely handle missing or null values
 */
void PhysicsLayers::load() {
    const std::string file_data = IO::read_text_file({ IO::Location::PROJECT, "physics_layers.json" });
    if (file_data.empty()) return;

    tmt::json j;
    try {
        j = tmt::json::parse(file_data);
    } catch (...) {
        return;
    }

    layer_count = j["layer_count"].get<uint32_t>();

    // Clear previous data
    for (uint32_t i = 0; i < MAX_LAYERS; ++i) {
        layer_names[i].clear();
        for (uint32_t k = 0; k < MAX_LAYERS; ++k) collision_matrix[i][k] = false;
    }

    // Load names
    auto& names = j["names"];
    for (uint32_t i = 0; i < layer_count; ++i) {
        if (i < names.size() && !names[i].is_null())
            layer_names[i] = names[i].get<std::string>();
        else
            layer_names[i] = "";
    }

    // Load collisions
    auto& collisions = j["collisions"];
    for (uint32_t i = 0; i < layer_count; ++i) {
        for (uint32_t k = 0; k < layer_count; ++k) {
            if (i < collisions.size() && k < collisions[i].size() && !collisions[i][k].is_null())
                collision_matrix[i][k] = collisions[i][k].get<bool>();
            else
                collision_matrix[i][k] = false;
        }
    }
}

/**
 * Save physics layers to JSON file
 *
 * Responsibilities:
 *   - Write layer_count, names, and collision matrix (upper triangle)
 *   - Make sure there is persistent storage between sessions
 */
void PhysicsLayers::save() const {
    nlohmann::json j;

    j["layer_count"] = layer_count;

    // Layer names
    for (uint32_t i = 0; i < layer_count; ++i) {
        j["names"][i] = layer_names[i];
    }

    // Collision matrix (upper triangle)
    for (uint32_t i = 0; i < layer_count; ++i) {
        for (uint32_t k = i; k < layer_count; ++k) {
            j["collisions"][i][k] = collision_matrix[i][k];
        }
    }

    IO::write_text_file({ IO::Location::PROJECT, "physics_layers.json" }, j.dump(4));
    Log::info("Saved {} physics layers", layer_count);
}

}  // namespace tmt
