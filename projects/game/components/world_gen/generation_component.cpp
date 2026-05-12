#include "generation_component.hpp"
#include "engine/tools/random.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/light.hpp"

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

    tmt::Serializer::deserialize(json_obj, level_configuration);

    auto& pickable_cell_templates = level_configuration.scene_pickable_cell_templates;
    auto& cells = level_configuration.scene_cells;

    for (auto& cell_template : pickable_cell_templates) {
        cell_template.field.size = { level_configuration.cell_size, cell_template.height, level_configuration.cell_size };
    }

    main_sun_entity = entt::null;
    for (const auto& [entity, transform, light] : tmt::engine.ecs.view<tmt::Transform, tmt::Light>().each()) {
        if (light.type == tmt::LightType::SUN_LIGHT) {
            if (main_sun_entity != entt::null) {
                tmt::Log::warn("[GenerationComponent]: multiple sun lights found, setting light placement influence sun to last one..");
            }
            main_sun_entity = entity;
        }
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
            auto& layer_entry = cell_template.field.layer_entries[point.entry_idx];
            const auto& spawnables = layer_entry.spawnables;
            // float size_factor = cell_template.field.layer_entries[point.entry_idx].radius_factor;
            int obj_idx = static_cast<int>(Random::rand_range(0.f, static_cast<float>(spawnables.size()) - 0.0001f));

            const auto& spawn_obj = spawnables[obj_idx];

            float pitch = Random::rand_range(0.f, 360.f);
            float yaw = Random::rand_range(0.f, 360.f);
            float roll = Random::rand_range(0.f, 360.f);

            auto instantiated = tmt::PrefabHelper::instantiate_prefab(spawn_obj->file_location, entity);
            lighting_pass_data.push_back(std::make_tuple(cell.template_index, point, instantiated));

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

void GenerationComponent::update(const tmt::FrameData& time) {
    if (lighting_pass_data.size() > 0) {
        for (size_t i = 0; i < lighting_pass_data.size();) {
            auto& [cell_template_idx, point, spawned_entity] = lighting_pass_data[i];

            bool remove_entry = false;

            auto& cell_template = level_configuration.scene_pickable_cell_templates[cell_template_idx];
            auto& layer_entry = cell_template.field.layer_entries[point.entry_idx];

            if (layer_entry.can_spawn_lights && Random::rand_range(0.f, 1.f) < layer_entry.light_spawn_chance && main_sun_entity != entt::null) {
                auto& sun_transform = tmt::engine.ecs.get_component<tmt::Transform>(main_sun_entity);
                auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(spawned_entity);

                glm::vec3 pos_check_start = transform.get_world_position() + sun_transform.get_forward() * cell_template.field.spacing_radius * layer_entry.radius_factor;

                auto ray = tmt::Ray(pos_check_start, -sun_transform.get_forward());
                tmt::Hit hit = tmt::engine.renderer.trace_ray(ray);

                if (!hit.miss()) {
                    remove_entry = true;

                    tmt::Entity asteroid_light_entity = tmt::engine.ecs.create_entity("AsteroidLight");
                    auto& light = tmt::engine.ecs.add_component<tmt::Light>(asteroid_light_entity);

                    auto& light_transform = tmt::engine.ecs.get_component<tmt::Transform>(asteroid_light_entity);

                    float surface_dist = cell_template.field.spacing_radius * layer_entry.radius_factor - hit.distance;
                    float light_spacing = surface_dist + 12.f;

                    light_transform.set_world_position(transform.get_world_position() + sun_transform.get_forward() * light_spacing);
                    light_transform.set_parent(entity);

                    light.light = tmt::SpotLight {};
                    light.type = tmt::LightType::SPOT_LIGHT;
                    auto& spot_light = std::get<tmt::SpotLight>(light.light);
                    spot_light.attenuation_distance = light_spacing + 10.f;
                    spot_light.luminous_intensity = 450.f;
                    spot_light.beam_angle = glm::radians(60.f);

                    light_transform.set_world_rotation(glm::quatLookAt(sun_transform.get_forward(), glm::vec3(0.f, 1.f, 0.f)));

                    light.color = { 1.f, 1.f, 1.f };

                    if (!cell_template.possible_light_colors.empty()) {
                        int color_idx = static_cast<int>(Random::rand_range(0.f, static_cast<float>(cell_template.possible_light_colors.size()) - 0.01f));
                        auto& rgba = cell_template.possible_light_colors[color_idx];
                        light.color = rgba.get();
                    }
                }
            }

            if (remove_entry) {
                lighting_pass_data.erase(lighting_pass_data.begin() + i);
            } else {
                ++i;
            }
        }
    }
}

}  // namespace game
