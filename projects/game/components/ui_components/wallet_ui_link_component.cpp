#include "wallet_ui_link_component.hpp"

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
    text_renderer_component->text = std::to_string(current_value);
}

void WalletUiLink::update_value() {
    int value_accumulator = 0;

    for (tmt::Entity entity_with_wallet : entities_with_wallet) {
        auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(entity_with_wallet);
        // wallet guard
        if (wallet_component) {
            if (display_type == DisplayType::CURRENT) {
                if (resource_to_display == DisplayTextType::DOLLARS) {
                    value_accumulator += static_cast<int>(wallet_component->currencies.dollars);
                } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
                    // accumulate resource counts
                    for (auto resource_pair : wallet_component->currencies.resource_counts) {
                        value_accumulator += static_cast<int>(resource_pair.second);
                    }
                } else {
                    // Cast enum to OreResources and use directly
                    auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
                    value_accumulator += static_cast<int>(wallet_component->currencies.resource_counts.at(ore_type));
                }
            } else if (display_type == DisplayType::LIMIT) {
                if (resource_to_display == DisplayTextType::DOLLARS) {
                    value_accumulator += static_cast<int>(wallet_component->limits.dollars);
                } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
                    // accumulate resource counts
                    value_accumulator += wallet_component->total_resource_limit;
                } else {
                    // Cast enum to OreResources and use directly
                    auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
                    if (resource_to_display == DisplayTextType::DOLLARS) {
                        value_accumulator += static_cast<int>(wallet_component->currencies.dollars);
                    } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
                        // accumulate resource counts
                        for (auto resource_pair : wallet_component->currencies.resource_counts) {
                            value_accumulator += static_cast<int>(resource_pair.second);
                        }
                    }
                }
            }
        }
    }

    current_value = value_accumulator;

    change_text();
}

}  // namespace game