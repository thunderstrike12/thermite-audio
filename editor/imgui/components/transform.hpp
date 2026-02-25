#pragma once
#include <ImReflect.hpp>

#include "engine/core/components/transform.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Transform& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::Transform>();
    auto& type_response = response.get<tmt::Transform>();

    {
        struct Position {};
        auto& position_settings = type_settings.get<Position>();
        auto& position_response = type_response.get<Position>();

        position_settings.push<glm::vec3>().as_drag().speed(0.1f);

        glm::vec3 position = value.get_local_position();
        ImReflect::Input("Position", position, position_settings, position_response);

        if (position_response.is_changed()) {
            value.set_local_position(position);
        }
    }
    {
        struct Rotation {};
        auto& rotation_settings = type_settings.get<Rotation>();
        auto& rotation_response = type_response.get<Rotation>();

        rotation_settings.push<glm::vec3>().as_drag().speed(1.0f);

        glm::vec3 euler = glm::degrees(value.get_local_eulers());
        ImReflect::Input("Rotation", euler, rotation_settings, rotation_response);

        if (rotation_response.is_changed()) {
            value.set_local_rotation(glm::radians(euler));
        }
    }
    {
        struct Scale {};
        auto& scale_settings = type_settings.get<Scale>();
        auto& scale_response = type_response.get<Scale>();

        scale_settings.push<glm::vec3>().as_drag().speed(0.1f);
        glm::vec3 scale = value.get_local_scale();

        ImReflect::Input("Scale", scale, scale_settings, scale_response);

        if (scale_response.is_changed()) {
            value.set_local_scale(scale);
        }
    }
    {
        const auto& parent = value.get_parent();
        ImReflect::Input("Parent", parent);
    }
    {
        //const auto& children = value.get_children();
        //ImReflect::Input("Children", children);
    }
}