#include "spawner.hpp"

#include "engine/tools/prefab_helper.hpp"
namespace game {

std::string_view Spawner::get_name() {
    return "Spawner";
}
void Spawner::start() {
    if (use_attached_entity_as_spawn_parent) {
        spawn_parent = entity;
    }
}
void Spawner::update(const tmt::FrameData& time) {}
void Spawner::end() {}
tmt::Entity Spawner::spawn() const {
    return tmt::PrefabHelper::instantiate_prefab(prefab_location, spawn_parent);
}
tmt::Entity Spawner::spawn(const tmt::ResourceRef<tmt::Json>& location, const tmt::Entity& parent) {
    return tmt::PrefabHelper::instantiate_prefab(location, parent);
}

}  // namespace game
