#include "scenes.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/events/scene.hpp"
#include "engine/core/io.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/resources/json.hpp"

#include "engine/tools/serializer/ecs.hpp"

namespace tmt {

void Scenes::update() {
    if (next_scene_type == NULL_SCENE) return;

    swap_scenes();
}

void Scenes::end() {
    if (active_scene_type == NULL_SCENE) return;

    OnPreUnloadScene::dispatch();
    if (active_scene) active_scene->on_end();
    active_scene_type = NULL_SCENE;
    engine.ecs.clear();
    OnSceneEnd::dispatch();
    active_scene.reset();
    next_scene_type = NULL_SCENE;
}

void Scenes::swap_scenes() {
    if (next_scene_type == NULL_SCENE) {
        Log::error(Log::Scope::ENGINE, "[Scenes] swap_scenes: No scene enqueued");
        return;
    }
    if (registered_scenes.contains(next_scene_type) == false) {
        Log::error(Log::Scope::ENGINE, "[Scenes] swap_scenes: Scene not registered");
        return;
    }

    /* Unload */
    OnPreUnloadScene::dispatch();
    if (active_scene) active_scene->on_end();
    active_scene_type = NULL_SCENE;
    engine.ecs.clear();
    OnSceneEnd::dispatch();

    /* Load */
    SceneInfo& info = registered_scenes.at(next_scene_type);
    active_scene = info.factory.create();
    active_scene_type = next_scene_type;
    next_scene_type = NULL_SCENE;

    PreLoadSceneEvent pre_load_event(active_scene_type);

    if (IO::file_exists(info.file_location)) {
        active_scene_json = engine.resources.load_resource<Json>(info.file_location);
        if (active_scene_json == nullptr) {
            Log::error(Log::Scope::ENGINE, "[Scenes] deserialize_scene: Failed to load active scene JSON");
        } else {
            pre_load_event.scene_json = active_scene_json->get_parsed_json();
        }
    }

    OnPreLoadScene::dispatch(pre_load_event);
    active_scene->on_pre_load();

    /* Deserialize */
    deserialize_scene(pre_load_event);

    OnPostLoadScene::dispatch();
    active_scene->on_post_load();

    /* Start */
    if (engine.game_controller.is_playing() && engine.game_controller.should_game_end() == false) {
        active_scene->on_start();
        OnSceneStart::dispatch();
    }
}

void Scenes::serialize_active_scene() {
    if (active_scene_type == NULL_SCENE) {
        Log::error(Log::Scope::ENGINE, "[Scenes] serialize_active_scene: No active scene");
        return;
    }

    const SceneInfo& info = registered_scenes.at(active_scene_type);

    json serialized = Serializer::serialize(engine.ecs);
    IO::write_text_file(info.file_location, serialized.dump(4), true);

    if (active_scene_json == nullptr) {
        active_scene_json = engine.resources.load_resource<Json>(info.file_location);
        if (active_scene_json == nullptr) {
            Log::error(Log::Scope::ENGINE, "[Scenes] serialize_active_scene: Failed to load active scene JSON after serialization");
        }
    } else {
        active_scene_json->unload();
        active_scene_json->load();
    }

    OnSceneSerialized::dispatch();
}

void Scenes::deserialize_scene(PreLoadSceneEvent& event) {
    if (event.scene_json.empty()) {
        Log::warn(Log::Scope::ENGINE, "[Scenes] deserialize_scene: Empty json, skipping deserialization");
        return;
    }
    if (event.scene_json.is_discarded()) {
        Log::error(Log::Scope::ENGINE, "[Scenes] deserialize_scene: Discarded json, cannot deserialize");
        return;
    }

    Serializer::deserialize(event.scene_json, tmt::engine.ecs);
}

void Scenes::enqueue_scene(const SceneIndex& type_id) {
    if (registered_scenes.contains(type_id) == false) {
        Log::error(Log::Scope::ENGINE, "[Scenes] Enqueue scene: Scene not registered");
        return;
    }
    next_scene_type = type_id;
}

void Scenes::load_scene(const SceneIndex& type_id) {
    enqueue_scene(type_id);
    swap_scenes();
}

const SceneInfo& Scenes::get_active_scene_info() const {
    return get_scene_info(active_scene_type);
}

const SceneInfo& Scenes::get_scene_info(const SceneIndex& type_index) const {
    if (registered_scenes.contains(type_index) == false) {
        Log::error(Log::Scope::ENGINE, "[Scenes] get_scene_info: Scene not registered");
        throw std::runtime_error("Scene not registered");
    }
    return registered_scenes.at(type_index);
}

}  // namespace tmt
