#include "wallet_ui_link_component.hpp"

namespace game {

void WalletUiLink::start() {
    if (wallet_entity_check()) {
        text_renderer_component = tmt::engine.ecs.try_get_component<tmt::TextRenderer>(entity);
        wallet_component = tmt::engine.ecs.try_get_component<Wallet>(entity_with_wallet);
    }

    update_value();
}

void WalletUiLink::update(const tmt::FrameData& time) {
    update_value();
}

void WalletUiLink::end() {}

bool WalletUiLink::wallet_entity_check() const {
    if (entity_with_wallet == entt::null) {
        tmt::Log::error("No wallet entity set for wallet ui link component on entity: {}", entity);
        return false;
    }

    if (!tmt::engine.ecs.valid(entity_with_wallet)) {
        tmt::Log::error("Wallet entity is set to invalid entity, in wallet ui link component on entity: {}", entity);
        return false;
    }
    return true;
}

bool WalletUiLink::component_check() const {
    if (!wallet_component) {
        tmt::Log::error("No wallet found on wallet entity: {}, for wallet ui link component on entity: {}", entity_with_wallet, entity);
        return false;
    }

    if (!text_renderer_component) {
        tmt::Log::error("No text renderer found for wallet ui link component on entity: {}", entity);
        return false;
    }
    return true;
}

void WalletUiLink::change_text() const {
    text_renderer_component->text = std::to_string(current_value);
}

void WalletUiLink::update_value() {
    if (component_check()) {
        if (resource_to_display == DisplayTextType::DOLLARS) {
            current_value = static_cast<int>(wallet_component->currencies.dollars);
        } else if (resource_to_display == DisplayTextType::ALL_RESOURCES) {
            // reset value first for accumulation
            current_value = 0;
            // accumulate resource counts
            for (auto resource_pair : wallet_component->currencies.resource_counts) {
                current_value += static_cast<int>(resource_pair.second);
            }
        } else {
            // Cast enum to OreResources and use directly
            auto ore_type = static_cast<OreProperties::OreResources>(resource_to_display);
            current_value = static_cast<int>(wallet_component->currencies.resource_counts.at(ore_type));
        }
    }

    if (current_value != previous_value) {
        change_text();
    }
}

}  // namespace game
