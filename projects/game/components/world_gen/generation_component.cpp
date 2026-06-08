#include "generation_component.hpp"
#include "engine/tools/random.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "engine/engine.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"
#include "engine/systems/physics/physics_system.hpp"
#include "engine/core/components/light.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"

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

        cell_template.field.use_noise = level_configuration.use_noise;
        cell_template.field.noise_treshold = level_configuration.threshold;
        cell_template.field.noise_scale = level_configuration.noise_scale;

        if (cell_template.random_seed)
            Random::set_seed(Random::irand());
        else
            Random::set_seed(cell_template.seed);

        auto local_points = tmt::get_poisson_points(cell_template.field);

        if (level_configuration.use_noise) {
            std::erase_if(local_points, [this, &coord](const auto& point) {
                const auto& p = (coord_to_world(coord, level_configuration) + point.pos) * level_configuration.noise_scale;
                float noise = Random::noise3D(p.x, p.y, p.z);

                return noise < level_configuration.threshold;
            });
        }

        glm::vec3 cell_pos = coord_to_world(coord, level_configuration);

        // spawning
        for (auto& point : local_points) {
            auto& layer_entry = cell_template.field.layer_entries[point.entry_idx];
            const auto& spawnables = layer_entry.spawnables;
            // float size_factor = cell_template.field.layer_entries[point.entry_idx].radius_factor;
            int obj_idx = static_cast<int>(Random::rand_range(0.f, static_cast<float>(spawnables.size()) - 0.0001f));

            const auto& spawn_obj = spawnables[obj_idx];
            if (spawn_obj) {
                glm::vec3 min = cell_pos - glm::vec3(level_configuration.cell_size * 0.5f, cell_template.height * 0.5f, level_configuration.cell_size * 0.5f) +
                                glm::vec3(level_configuration.cell_margin, 0.f, level_configuration.cell_margin);

                float pitch = Random::rand_range(0.f, 360.f);
                float yaw = Random::rand_range(0.f, 360.f);
                float roll = Random::rand_range(0.f, 360.f);

                for (auto& [entity, clearance_volume] : tmt::engine.ecs.view<ClearanceVolume>()) {
                    if (clearance_volume.contains(min + point.pos)) {
                        continue;
                    }
                }

                auto instantiated = tmt::PrefabHelper::instantiate_prefab(spawn_obj.file_location, entity);
                if (instantiated == entt::null) continue;

                auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(instantiated);

                transform.set_local_position(min + point.pos);
                transform.set_local_rotation(glm::vec3(pitch, yaw, roll));

                auto sub_location_path = tmt::IO::get_sub_location_path(cell_template.light_prefab.file_location.sub_location);
                auto absolute_path = cell_template.light_prefab.file_location.get_relative_path();

                bool has_lantern_prefab = sub_location_path != absolute_path;  // i dont like this, but idk how else

                if (Random::rand_range(0.f, 1.f) < layer_entry.light_spawn_chance) lighting_pass_data.push_back(std::make_tuple(cell.template_index, point, instantiated, has_lantern_prefab));
            }
        }
    }
}

void GenerationComponent::end() {}

glm::vec3 GenerationComponent::coord_to_world(glm::ivec2 coord, const tmt::LevelConfiguration& level_configuration) {
    return level_configuration.global_field_offset +
           glm::vec3(coord.x * level_configuration.cell_size + level_configuration.cell_size * 0.5f, 0.f, coord.y * level_configuration.cell_size + level_configuration.cell_size * 0.5f);
}

void GenerationComponent::update(const tmt::FrameData& time) {
    auto& phys_sys = tmt::engine.ecs.systems.get<tmt::Physics>();

    if (lighting_pass_data.size() > 0) {
        auto& [cell_template_idx, point, spawned_entity, has_lantern_prefab] = lighting_pass_data[iteration];

        bool remove_entry = false;

        auto& cell_template = level_configuration.scene_pickable_cell_templates[cell_template_idx];
        auto& layer_entry = cell_template.field.layer_entries[point.entry_idx];

        if (layer_entry.can_spawn_lights && main_sun_entity != entt::null && layer_entry.radius_factor < cell_template.light_spawn_radius_factor_threshold) {
            auto& sun_transform = tmt::engine.ecs.get_component<tmt::Transform>(main_sun_entity);
            auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(spawned_entity);

            glm::quat rot = glm::angleAxis(glm::radians(Random::rand_range(0.f, 360.f)), sun_transform.get_forward());
            glm::vec3 slanted_vec = glm::normalize(glm::mix(sun_transform.get_forward(), glm::normalize(rot * sun_transform.get_up()), 0.2f));

            glm::vec3 pos_check_start = transform.get_world_position() + slanted_vec * cell_template.field.spacing_radius * layer_entry.radius_factor;

            auto ray = tmt::Ray(pos_check_start, glm::normalize(transform.get_world_position() - pos_check_start));
            uint32_t layer_mask = 0xFFFFFFFF & ~(1 << 2) & ~(1 << 1);  // ignore enemies and player
            tmt::Hit hit = phys_sys.raycast(ray, layer_mask);

            if (!hit.miss()) {
                remove_entry = true;
                if (hit.distance > 2.f && (hit.distance * hit.distance) < glm::length2(pos_check_start - transform.get_world_position())) {
                    tmt::Entity asteroid_light_entity;

                    glm::vec3 color = { 1.f, 1.f, 1.f };
                    if (!cell_template.possible_light_colors.empty()) {
                        int color_idx = static_cast<int>(Random::rand_range(0.f, static_cast<float>(cell_template.possible_light_colors.size()) - 0.01f));
                        auto& rgba = cell_template.possible_light_colors[color_idx];
                        color = rgba.get();
                    }

                    if (cell_template.light_prefab) {
                        asteroid_light_entity = tmt::PrefabHelper::instantiate_prefab(cell_template.light_prefab);

                        tmt::Entity point_light_entity = tmt::engine.ecs.create_entity("Point Light Entity");
                        auto& point_light_transform = tmt::engine.ecs.get_component<tmt::Transform>(point_light_entity);

                        point_light_transform.set_parent(asteroid_light_entity);
                        point_light_transform.set_local_position({ 0.f, 0.f, 0.f });

                        auto& light = tmt::engine.ecs.add_component<tmt::Light>(point_light_entity);
                        auto& sphere_light = std::get<tmt::SphereLight>(light.light);

                        light.color = color;
                        sphere_light.attenuation_radius = 5.f;
                        sphere_light.luminous_flux = 400.f;

                    } else
                        asteroid_light_entity = tmt::engine.ecs.create_entity("AsteroidLight");

                    auto& light = tmt::engine.ecs.add_component<tmt::Light>(asteroid_light_entity);

                    auto& light_transform = tmt::engine.ecs.get_component<tmt::Transform>(asteroid_light_entity);

                    float surface_dist = cell_template.field.spacing_radius * layer_entry.radius_factor - hit.distance;
                    float light_spacing = surface_dist + 12.f;

                    light_transform.set_world_position(transform.get_world_position() + slanted_vec * light_spacing);
                    light_transform.set_parent(entity);

                    light.light = tmt::SpotLight {};
                    light.type = tmt::LightType::SPOT_LIGHT;
                    auto& spot_light = std::get<tmt::SpotLight>(light.light);
                    spot_light.attenuation_distance = light_spacing + 10.f;
                    spot_light.luminous_intensity = cell_template.spot_light_power;
                    spot_light.beam_angle = glm::radians(cell_template.spot_light_angle);

                    light_transform.set_world_rotation(glm::quatLookAt(slanted_vec, glm::vec3(0.f, 1.f, 0.f)));

                    light.color = color;
                }
            }
        }

        if (remove_entry) {
            lighting_pass_data.erase(lighting_pass_data.begin() + iteration);
        } else if (iteration + 1 < lighting_pass_data.size()) {
            ++iteration;
        } else
            iteration = 0;
    }
}

void ClearanceVolume::start() {}

void ClearanceVolume::update(const tmt::FrameData& time) {}

void ClearanceVolume::end() {}

void ClearanceVolume::draw_debug_lines() const {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);

    glm::vec3 pos = transform.get_world_position();

    tmt::engine.polyline.use_color({ 1.f, 0.f, 0.f, 1.f });
    tmt::engine.polyline.draw_aabb(pos - size * 0.5f, pos + size * 0.5f);
}

bool ClearanceVolume::contains(const glm::vec3& pos) {
    auto& transform = tmt::engine.ecs.get_component<tmt::Transform>(entity);
    glm::vec3 mid_pos = transform.get_world_position();

    glm::vec3 min = mid_pos - size * 0.5f;
    glm::vec3 max = mid_pos + size * 0.5f;

    if (pos.x > min.x && pos.x < max.x && pos.y > min.y && pos.y < max.y && pos.z > min.z && pos.z < max.z)
        return true;
    else
        return false;
}

}  // namespace game
