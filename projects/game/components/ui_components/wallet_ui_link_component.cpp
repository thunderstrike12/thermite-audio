#include "wallet_ui_link_component.hpp"

#include "projects/game/components/development_tools/save_data.hpp"
#include "projects/game/components/managers/menu_controller.hpp"

namespace game {

void WalletUiLink::start() {
    if (wallet_entity_check()) {
        update_value();
    }
}

void WalletUiLink::update(const tmt::FrameData& time) {
    update_value();
}

void WalletUiLink::end() {}

bool WalletUiLink::wallet_entity_check() const {
    for (tmt::Entity entity_with_wallet : entities_with_wallet) {
        if (entity_with_wallet == entt::null) {
            tmt::Log::error("No wallet entity set for wallet ui link component on entity: {}", entity);
            return false;
        }

        if (!tmt::engine.ecs.valid(entity_with_wallet)) {
            tmt::Log::error("Wallet entity is set to invalid entity, in wallet ui link component on entity: {}", entity);
            return false;
        }
    }
    return true;
}

void WalletUiLink::change_text() const {
    auto* text_renderer_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);
    text_renderer_component->text = std::to_string(target_value);
}

void WalletUiLink::update_value() {
    for (tmt::Entity entity_with_wallet : entities_with_wallet) {
        auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(entity_with_wallet);
        // wallet guard
        if (wallet_component) {
            auto view_elements = tmt::engine.ecs.view<MenuController>();
            switch (display_type) {
                case DisplayType::CURRENT:
                    if (resource_to_display == DisplayTextType::DOLLARS) {
                        target_value += static_cast<int>(wallet_component->currencies.dollars);
                    } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
                        // accumulate resource counts
                        for (auto resource_pair : wallet_component->currencies.resource_counts) {
                            target_value += static_cast<int>(resource_pair.second);
                        }
                    } else {
                        // Cast enum to OreResources and use directly
                        auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
                        float current_count = wallet_component->currencies.resource_counts.at(ore_type);
                        if (use_ramp_up_curve) {
                            if (current_count != ramp_target_value) {
                                if (is_ramping && ramp_duration > 0.0f) {
                                    const float t = glm::clamp(ramp_elapsed / ramp_duration, 0.0f, 1.0f);
                                    const float eased = ramp_up_curve.eval(t);
                                    ramp_start_value = ramp_start_value + (ramp_target_value - ramp_start_value) * eased;
                                }

                                ramp_target_value = current_count;
                                ramp_elapsed = 0.0f;
                                is_ramping = true;
                            }

                            float displayed;
                            if (is_ramping) {
                                ramp_elapsed += tmt::engine.frame_data().delta_time;
                                const float t = glm::clamp(ramp_elapsed / ramp_duration, 0.0f, 1.0f);
                                const float eased = ramp_up_curve.eval(t);
                                displayed = ramp_start_value + (ramp_target_value - ramp_start_value) * eased;

                                if (t >= 1.0f) {
                                    ramp_start_value = ramp_target_value;
                                    is_ramping = false;
                                }
                            } else {
                                displayed = ramp_target_value;
                            }

                            target_value = static_cast<int>(displayed);
                        } else {
                            target_value = static_cast<int>(current_count);
                        }
                    }

                    break;
                case DisplayType::LIMIT:
                    if (resource_to_display == DisplayTextType::DOLLARS) {
                        target_value = static_cast<int>(wallet_component->limits.dollars);
                    } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
                        // reset value first for accumulation
                        target_value = 0;
                        // accumulate resource counts
                        target_value = wallet_component->total_resource_limit;
                    } else {
                        // Cast enum to OreResources and use directly
                        auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
                        target_value = static_cast<int>(wallet_component->limits.resource_counts.at(ore_type));
                        auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(entity_with_wallet);
                        if (wallet_component) {
                            if (resource_to_display == DisplayTextType::DOLLARS) {
                                target_value = static_cast<int>(wallet_component->currencies.dollars);
                            } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
                                // reset value first for accumulation
                                target_value = 0;
                                // accumulate resource counts
                                for (auto resource_pair : wallet_component->currencies.resource_counts) {
                                    target_value += static_cast<int>(resource_pair.second);
                                }
                            }
                        }
                    }
                    break;
                case DisplayType::PENALTY:

                    if (view_elements.empty() == false) {
                        const auto multiplier { std::get<0>(view_elements.front().components).penalty_percentage };
                        auto original_currencies { wallet_component->currencies };
                        apply_multiplier(original_currencies, multiplier);
                        auto difference { wallet_component->currencies - original_currencies };

                        const auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
                        target_value = static_cast<int>(difference.resource_counts.at(ore_type));
                    }

                    break;
                case DisplayType::TOTAL:
                    if (view_elements.empty() == false) {
                        // If the player returned do not apply penalty
                        const auto multiplier { Player::get().player_ended_run ? 1.0f : std::get<0>(view_elements.front().components).penalty_percentage };
                        auto original_currencies { wallet_component->currencies };
                        apply_multiplier(original_currencies, multiplier);
                        auto& resources { tmt::engine.player_data.get<Currencies>(PERSISTENT_RESOURCES) };

                        auto total { resources + original_currencies };

                        const auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
                        target_value = static_cast<int>(total.resource_counts.at(ore_type));
                    }
                    break;
            }
        }

        current_value = target_value;

        change_text();
    }
}

}  // namespace game