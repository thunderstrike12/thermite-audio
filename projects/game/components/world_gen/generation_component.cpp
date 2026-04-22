#include "generation_component.hpp"
#include "engine/tools/random.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"

namespace game {

void GenerationComponent::clear_children() {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    for (auto child : transform.get_all_children()) {
        tmt::engine.ecs.destroy_entity(child);
    }
}

void GenerationComponent::start() {
    auto& grouper_trans = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    if (grouper_trans.has_children()) {
        for (auto ent : grouper_trans.get_children()) {
            tmt::engine.ecs.destroy_entity(ent);
        }
    }

    const std::string file_str = tmt::IO::read_text_file({ tmt::IO::Location::PROJECT, "level/config.json" });
    if (file_str.empty()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "No level configuration present, please generate one with the Level Editor");
        return;
    }

    tmt::json json_obj;
    try {
        json_obj = tmt::json::parse(file_str);
    } catch (const std::exception& e) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to parse level config.json: {}", e.what());
        return;
    }

    tmt::LevelConfiguration level_configuration;
    tmt::Serializer::deserialize(json_obj, level_configuration);

    auto& pickable_cell_templates = level_configuration.scene_pickable_cell_templates;
    auto& cells = level_configuration.scene_cells;

    for (auto& cell_template : pickable_cell_templates) {
        cell_template.field.size = { level_configuration.cell_size, cell_template.height, level_configuration.cell_size };
    }

    for (auto& [coord, cell] : cells) {
        auto& cell_template = pickable_cell_templates[cell.template_index];

        cell_template.field.size = { level_configuration.cell_size - level_configuration.cell_margin * 2.f, cell_template.height,
                                     level_configuration.cell_size - level_configuration.cell_margin * 2.f };

        if (cell_template.random_seed)
            Random::set_seed(Random::irand());
        else
            Random::set_seed(cell_template.seed);

        auto local_points = tmt::get_poisson_points(cell_template.field);

        glm::vec3 cell_pos = coord_to_world(coord, level_configuration);

        // spawning
        for (auto& point : local_points) {
            auto spawnables = cell_template.field.layer_entries[point.entry_idx].spawnables;
            // float size_factor = cell_template.field.layer_entries[point.entry_idx].radius_factor;
            int obj_idx = static_cast<int>(Random::rand_range(0.f, static_cast<float>(spawnables.size()) - 0.0001f));

            const auto& spawn_obj = spawnables[obj_idx];

            float pitch = Random::rand_range(0.f, 360.f);
            float yaw = Random::rand_range(0.f, 360.f);
            float roll = Random::rand_range(0.f, 360.f);

            auto instantiated = tmt::PrefabHelper::instantiate_prefab(spawn_obj->file_location, entity);

            auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);

            glm::vec3 min = cell_pos - glm::vec3(level_configuration.cell_size * 0.5f, cell_template.height * 0.5f, level_configuration.cell_size * 0.5f) +
                            glm::vec3(level_configuration.cell_margin, 0.f, level_configuration.cell_margin);

            transform.set_local_position(min + point.pos);
            transform.set_local_rotation(glm::vec3(pitch, yaw, roll));
        }
    }
}

void GenerationComponent::end() {}

glm::vec3 GenerationComponent::coord_to_world(glm::ivec2 coord, const tmt::LevelConfiguration& level_configuration) {
    return level_configuration.global_field_offset +
           glm::vec3(coord.x * level_configuration.cell_size + level_configuration.cell_size * 0.5f, 0.f, coord.y * level_configuration.cell_size + level_configuration.cell_size * 0.5f);
}

void GenerationComponent::update(const tmt::FrameData& time) {}

}  // namespace game
