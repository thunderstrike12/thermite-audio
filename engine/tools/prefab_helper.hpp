#pragma once
#include "engine/core/io.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/resources/json.hpp"

namespace tmt {

class PrefabHelper {
   public:
    static void create_prefab(const IO::FileLocation& location, const Entity& root);
    static Entity instantiate_prefab(const IO::FileLocation& location, const Entity& parent = entt::null);
    static Entity instantiate_prefab(const ResourceRef<Json>& prefab_json, const Entity& parent = entt::null);

    class Config {
       public:
        constexpr static const char* PREFAB_EXTENSION = ".prefab";
    };

    private:
    static Entity instantiate_prefab(const tmt::json& parsed_json, const IO::FileLocation& location, const Entity& parent);
};

}  // namespace tmt