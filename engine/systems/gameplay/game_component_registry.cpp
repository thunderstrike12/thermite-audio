#include "game_component_registry.hpp"

#include <stdexcept>

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"

namespace tmt {

void GameComponentRegistry::is_registered_or_throw(const ComponentIndex& type_id) {
    const bool is_registered = engine.component_registry.get_registered_components().contains(type_id);
    if (!is_registered) {
        const auto name = type_id.name();
        Log::error(Log::Scope::ENGINE, "[ComponentCollection] Component \"{}\" not registered in GameComponentRegistry.", name);
        throw std::runtime_error("[ComponentCollection] Component not registered in GameComponentRegistry.");
    }
}

}  // namespace tmt