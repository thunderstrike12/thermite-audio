#include "rig_controller.hpp"

#include "engine/tools/serializer/containers.hpp"
#include "engine/tools/serializer.hpp"

JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::RigController& controller) {
    JsonReflect::json j = JsonReflect::Detail::to_json_visitable(controller);

#ifdef THERMITE_EDITOR
    j["editor_context"] = tmt::Serializer::serialize(controller.editor_data);
#endif  // THERMITE_EDITOR

    return j;
}

void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::RigController& controller) {
    JsonReflect::Detail::from_json_visitable(j, controller);

#ifdef THERMITE_EDITOR
    if (j.contains("editor_context")) tmt::Serializer::deserialize(j["editor_context"], controller.editor_data);
#endif  // KUDZU_INSPECTOR
}
