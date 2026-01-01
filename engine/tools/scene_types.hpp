#pragma once
#include <type_traits>
#include <typeindex>
#include <memory>
#include <string>

#include "engine/core/io.hpp"
#include "engine/core/scene.hpp"
#include "engine/tools/type_factory.hpp"

namespace tmt {

using SceneFactory = TypeFactory<std::unique_ptr<SceneBase>>;

struct SceneInfo {
    const std::string name;
    const IO::FileLocation file_location;

    SceneFactory factory;
};

using SceneIndex = std::type_index;

/* empty type_index */
static inline const SceneIndex NULL_SCENE = SceneIndex(typeid(void));

}  // namespace tmt