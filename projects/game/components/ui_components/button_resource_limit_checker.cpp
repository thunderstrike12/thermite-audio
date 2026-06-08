#include "button_resource_limit_checker.hpp"

#include "engine/core/components/button.hpp"
#include "engine/systems/ui/ui.hpp"
#include "projects/game/components/gameplay_functionality_components/player.hpp"

namespace game {

void ResourceLimitChecker::start() {
    if (wallet_entity == entt::null) {
        tmt::Log::warn("Wallet entity not set for resource limit checker on entity: {} Will attempt to default to player.", entity);
        wallet_entity = Player::get().entity;
        if (!tmt::engine.ecs.valid(wallet_entity)) {
            tmt::Log::error("No player found in scene!");
            return;
        }
    }

    auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(wallet_entity);
    if (!wallet_component) {
        tmt::Log::error("Wallet entity set for resource checker has no wallet component: {}", wallet_entity);
        return;
    }

    auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity);
    auto* interactable_component = tmt::engine.ecs.try_get_component<tmt::UIInteractable>(entity);
    if (!button_component) {
        tmt::Log::error("Entity {} has no Button component but is trying to use resource checker.", entity);
        return;
    }
    if (!interactable_component) {
        tmt::Log::error("Entity {} has no UIInteractable component but is trying to use resource checker.", entity);
        return;
    }

    if (!resource_check()) {
        interactable_component->disabled = true;
        tmt::Log::warn("Button on entity: {} was disabled due to insufficient resources!", entity);
    }
    button_component->on_click.add(this, &ResourceLimitChecker::button_click);
}

void ResourceLimitChecker::update(const tmt::FrameData& time) {
    interval_counter += time.delta_time;
    if (interval_counter > check_interval) {
        auto* interactable_component = tmt::engine.ecs.try_get_component<tmt::UIInteractable>(entity);
        interval_counter = 0.0f;
        if (!interactable_component) {
            tmt::Log::error("Entity {} has no Button or UIInteractable component but is trying to use resource checker.", entity);
            return;
        }
        if (interactable_component->disabled) {
            if (resource_check()) {
                interactable_component->disabled = false;
            }
        }
    }

    if (is_flashing) {
        if (flashing_counter < flashing_time) {
            // count up flashing counter
            flashing_counter += time.delta_time;
            // count up flash counter for individual flash time
            flash_counter += time.delta_time;
            if (flash_counter >= flash_time) {
                flash_counter = 0.0f;
                flash_ui();
            }
        } else {
            // reset flashing counter
            flashing_counter = 0.0f;
            // set flash counter to flash time to instantly initiate flashing next time
            flash_counter = flash_time;
            stop_flash_ui();
        }
    }
}

void ResourceLimitChecker::end() {
    if (auto* button_component = tmt::engine.ecs.try_get_component<tmt::Button>(entity)) button_component->on_click.clear();
}

bool ResourceLimitChecker::resource_check() {
    auto* wallet_component = tmt::engine.ecs.try_get_component<Wallet>(wallet_entity);
    if (!wallet_component) return false;
    int count = 0;
    auto resources = wallet_component->currencies.resource_counts;
    for (auto& resource_pair : resources) {
        count += static_cast<int>(resource_pair.second);
    }
    if (count > wallet_component->total_resource_limit) return false;
    return true;
}

void ResourceLimitChecker::button_click(tmt::Button::Context context) {
    auto* button_component = tmt::engine.ecs.try_get_component<tmt::UIInteractable>(entity);
    if (context.disabled) {
        if (resource_check()) {
            button_component->disabled = false;
        } else {
            tmt::Log::info("Button stays disabled, resource check failed, sell some ores.");

            auto* ui = tmt::engine.ecs.systems.try_get<tmt::UI>();
            if (!ui->insufficient_funds_instance.is_valid()) ui->insufficient_funds_instance = ui->menu_sounds.sounds.insufficient_funds.play();

            is_flashing = true;

            if (pop_up_entity != entt::null) {
                tmt::engine.ecs.enable(pop_up_entity);
                Tweening::tween<float>()  //
                    .duration(pop_up_time)
                    .on_complete([this]() {
                        //
                        tmt::engine.ecs.disable(pop_up_entity);
                    });
            }
        }
    }
}

void ResourceLimitChecker::flash_ui() {
    if (flashing_ui_entity == entt::null) {
        tmt::Log::warn("No flashing entity set for resource limit component on entity: {}", entity);
        return;
    }
    if (!tmt::engine.ecs.valid(flashing_ui_entity)) {
        tmt::Log::warn("Flashing entity set for resource limit component on entity: {}, is invalid", entity);
        return;
    }

    if (tmt::engine.ecs.is_enabled(flashing_ui_entity)) {
        tmt::engine.ecs.disable(flashing_ui_entity);
    } else {
        tmt::engine.ecs.enable(flashing_ui_entity);
    }
}

void ResourceLimitChecker::stop_flash_ui() {
    if (flashing_ui_entity == entt::null) {
        tmt::Log::warn("No flashing entity set for resource limit component on entity: {}", entity);
        return;
    }
    if (!tmt::engine.ecs.valid(flashing_ui_entity)) {
        tmt::Log::warn("Flashing entity set for resource limit component on entity: {}, is invalid", entity);
        return;
    }
    tmt::engine.ecs.enable(flashing_ui_entity);
    is_flashing = false;
}

}  // namespace game
