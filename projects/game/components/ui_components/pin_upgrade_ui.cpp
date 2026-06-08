#include "pin_upgrade_ui.hpp"

#include "text_component.hpp"

#include "engine/core/components/image_renderer.hpp"

namespace game {

static tmt::Entity copy_entity(tmt::Entity peepee) {
    auto json = tmt::Serializer::serialize(peepee, tmt::engine.ecs);
    std::set<tmt::Entity> result;
    tmt::Serializer::deserialize(json, result, tmt::engine.ecs);

    auto parent_result = tmt::EntityHelper::upper_parents(result);

    auto it = parent_result.begin();
    if (it != parent_result.end()) {
        auto firstElement = *it;  // Access the first element

        return firstElement;
    }
    return entt::null;
}

void UpgradePinUiComponent::start() {
    // Check if the template was set up correctly.
    if (!tmt::engine.ecs.valid(pin_cost_template)) {
        tmt::Log::error("[Upgrade pins] Pin Cost Template Entity has not been set or is invalid.");
        return;
    }
    tmt::TextRenderer* template_text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(pin_cost_template);
    if (!template_text_component) {
        tmt::Log::error("[Upgrade pins] Pin Cost Template Entity does not contain a text component (it should).");
        return;
    }
    auto* template_entity_transform = tmt::engine.ecs.try_get_component<tmt::Transform>(pin_cost_template);
    auto& template_children = template_entity_transform->get_children();
    tmt::Entity template_icon_entity = entt::null;
    auto it = template_children.begin();
    if (it != template_children.end()) {
        auto firstElement = *it;  // Access the first element
        template_icon_entity = firstElement;
    }
    if (template_icon_entity == entt::null) {
        tmt::Log::error("[Upgrade pins] Pin Cost Template Entity has no icon child.");
        return;
    }

    tmt::ImageRenderer* template_image_renderer_component = tmt::engine.ecs.try_get_component<tmt::ImageRenderer>(template_icon_entity);
    if (!template_image_renderer_component) {
        tmt::Log::error("[Upgrade pins] Pin Cost Template Entity does not contain a image renderer component (it should).");
        return;
    }
    tmt::engine.ecs.disable(pin_cost_template);

    // Check the text entities
    if (!tmt::engine.ecs.valid(pin_text_entity_1)) {
        tmt::Log::error("[Upgrade pins] Pin text entity 1 wasnt set up or is invalid.");
        return;
    }
    tmt::engine.ecs.disable(pin_text_entity_1);
    tmt::Transform* pin_transform_entity_1 = tmt::engine.ecs.try_get_component<tmt::Transform>(pin_text_entity_1);

    if (!tmt::engine.ecs.valid(pin_text_entity_2)) {
        tmt::Log::error("[Upgrade pins] Pin text entity 2 wasnt set up or is invalid.");
        return;
    }
    tmt::engine.ecs.disable(pin_text_entity_2);
    tmt::Transform* pin_transform_entity_2 = tmt::engine.ecs.try_get_component<tmt::Transform>(pin_text_entity_2);

    if (!tmt::engine.ecs.valid(pin_text_entity_3)) {
        tmt::Log::error("[Upgrade pins] Pin text entity 3 wasnt set up or is invalid.");
        return;
    }
    tmt::engine.ecs.disable(pin_text_entity_3);
    tmt::Transform* pin_transform_entity_3 = tmt::engine.ecs.try_get_component<tmt::Transform>(pin_text_entity_3);

    // Load in pin data
    auto& saved_upgrade_pins = tmt::engine.player_data.get<SavedUpgradePins>(PINNED_UPGRADES, SavedUpgradePins());

    // Collect the entity handles in order
    tmt::Entity pin_text_entities[] = { pin_text_entity_1, pin_text_entity_2, pin_text_entity_3 };
    int slot = 0;

    // Generate pin texts according to data and enable them
    for (auto& entry : saved_upgrade_pins.pinned_upgrades) {
        if (entry.active) {
            auto* text_renderer_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(pin_text_entities[slot]);
            if (!text_renderer_component) {
                tmt::Log::error("[Upgrade pins] Pin text entity {} has no text component.", slot);
                return;
            }

            text_renderer_component->text = entry.shown_name;
            tmt::engine.ecs.enable(pin_text_entities[slot]);
            int cost_entity_count = 0;
            float pos_diff = .0f;
            if (slot == 1) {
                pos_diff = pin_transform_entity_2->get_world_position().y - pin_transform_entity_1->get_world_position().y;
            }
            if (slot == 2) {
                pos_diff = pin_transform_entity_3->get_world_position().y - pin_transform_entity_1->get_world_position().y;
            }
            auto pin_cost_template_vertical_size = tmt::engine.ecs.try_get_component<tmt::UIComponent>(pin_cost_template)->size.y;

            // cost entities
            spawn_cost_entity(pin_text_entities[slot], cost_entity_count, entry.dollar_cost, icon_textures.dollar_icon_texture, pin_cost_template_vertical_size, pos_diff);
            spawn_cost_entity(pin_text_entities[slot], cost_entity_count, entry.scrap_cost, icon_textures.scrap_icon_texture, pin_cost_template_vertical_size, pos_diff);
            spawn_cost_entity(pin_text_entities[slot], cost_entity_count, entry.copper_cost, icon_textures.copper_icon_texture, pin_cost_template_vertical_size, pos_diff);
            spawn_cost_entity(pin_text_entities[slot], cost_entity_count, entry.thermite_cost, icon_textures.thermite_icon_texture, pin_cost_template_vertical_size, pos_diff);
            spawn_cost_entity(pin_text_entities[slot], cost_entity_count, entry.titanium_cost, icon_textures.titanium_icon_texture, pin_cost_template_vertical_size, pos_diff);
            slot++;
        }
    }
}

void UpgradePinUiComponent::update(const tmt::FrameData& time) {}

void UpgradePinUiComponent::end() {}

void UpgradePinUiComponent::spawn_cost_entity(
    tmt::Entity parent, int& cost_count, int cost, const tmt::ResourceRef<tmt::Texture2D>& icon_texture_ref, float vertical_padding, float y_pos_diff
) const {
    if (cost > 0) {
        auto generated_entity = copy_entity(pin_cost_template);
        auto* cost_text_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(generated_entity);
        auto* transform_component = tmt::engine.ecs.try_get_component<tmt::Transform>(generated_entity);
        transform_component->set_parent(parent);
        glm::vec3 world_position = transform_component->get_world_position();

        auto& generated_entity_children = transform_component->get_children();
        tmt::Entity generated_entity_icon_entity = entt::null;
        auto it = generated_entity_children.begin();
        if (it != generated_entity_children.end()) {
            auto firstElement = *it;  // Access the first element
            generated_entity_icon_entity = firstElement;
        }
        tmt::ImageRenderer* image_component = tmt::engine.ecs.try_get_component<tmt::ImageRenderer>(generated_entity_icon_entity);

        image_component->texture = icon_texture_ref;
        cost_text_component->text = std::to_string(cost);
        transform_component->set_world_position(glm::vec3(world_position.x, world_position.y + (static_cast<float>(cost_count) * vertical_padding) + y_pos_diff, world_position.z));
        tmt::engine.ecs.enable(generated_entity);

        cost_count++;
    }
}

}  // namespace game