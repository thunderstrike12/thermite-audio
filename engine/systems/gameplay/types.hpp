#pragma once
#include <memory>
#include <string>
#include <typeindex>

#include "engine/tools/type_factory.hpp"

namespace tmt {

/* Forward declate */
class IGameComponent;

using ComponentIndex = std::type_index;

static inline const ComponentIndex NULL_COMPONENT = std::type_index(typeid(void));

using ComponentFactory = TypeFactory<std::unique_ptr<IGameComponent>>;

struct ComponentInfo {
    const std::string name;
    const ComponentIndex type_id = NULL_COMPONENT;

    ComponentFactory factory;
};

}  // namespace tmt