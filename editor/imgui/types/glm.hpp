#pragma once
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <ImReflect.hpp>

template <typename T>
concept GlmVector =
    requires(T v) {
        typename T::value_type;                                // Has scalar type
        { v.x };                                               // Has .x member
        { T::length() } -> std::same_as<glm::length_t>;        // Has length() returning glm::length_t
    } && !std::is_same_v<T, glm::qua<typename T::value_type>>  // Not a quaternion
    && (T::length() >= 2 && T::length() <= 4);                 // Valid vector dimensions

template <typename T>
struct glm_traits {
    using scalar_type = typename T::value_type;
    static constexpr int DIMENSION = T::length();
};

template <typename T>
constexpr ImGuiDataType get_imgui_data_type() {
    if constexpr (std::is_same_v<T, float>)
        return ImGuiDataType_Float;
    else if constexpr (std::is_same_v<T, double>)
        return ImGuiDataType_Double;
    else if constexpr (std::is_same_v<T, int>)
        return ImGuiDataType_S32;
    else if constexpr (std::is_same_v<T, unsigned>)
        return ImGuiDataType_U32;
    else
        static_assert(sizeof(T) == 0, "Unsupported scalar type");
}

template <GlmVector T>
struct ImReflect::type_settings<T> : ImRequired<T>,
                                     ImReflect::Detail::drag_speed<T>,
                                     ImReflect::Detail::input_widget<T>,
                                     ImReflect::Detail::drag_widget<T>,
                                     ImReflect::Detail::input_flags<T>,
                                     ImReflect::Detail::slider_flags<T> {
    type_settings() : ImReflect::Detail::input_type(ImReflect::Detail::input_type_widget::Drag) {}
};

template <GlmVector T>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, T& value, ImSettings& settings, ImResponse& response) {
    using scalar = typename glm_traits<T>::scalar_type;
    constexpr int DIM = glm_traits<T>::DIMENSION;

    auto& type_settings = settings.get<T>();
    auto& type_response = response.get<T>();

    const auto& speed = type_settings.get_speed();

    auto scope_id = ImReflect::Detail::scope_id(label);

    constexpr auto DATA_TYPE = get_imgui_data_type<scalar>();

    bool changed = false;
    if (type_settings.is_drag()) {
        changed = ImGui::DragScalarN(label, DATA_TYPE, &value.x, DIM, speed, nullptr, nullptr, nullptr, type_settings.get_slider_flags());
    } else if (type_settings.is_input()) {
        const auto& step = type_settings.get_step();
        const auto& step_fast = type_settings.get_step_fast();
        changed = ImGui::InputScalarN(label, DATA_TYPE, &value.x, DIM, &step, &step_fast, nullptr, type_settings.get_input_flags());
    }

    if (changed) {
        type_response.changed();
    }

    ImReflect::Detail::check_input_states(type_response);
}

template <GlmVector T>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, const T& value, ImSettings& settings, ImResponse& response) {
    auto copy = value;
    ImGui::BeginDisabled(true);
    tag_invoke(ImReflect::ImInput_t {}, label, copy, settings, response);
    ImGui::EndDisabled();
}

inline void tag_invoke(ImReflect::ImInput_t, const char* label, glm::quat& value, ImSettings&, ImResponse& response) {
    auto& type_response = response.get<glm::quat>();

    auto scope_id = ImReflect::Detail::scope_id(label);

    constexpr auto DATA_TYPE = get_imgui_data_type<float>();

    const bool changed = ImGui::DragScalarN(label, DATA_TYPE, &value.x, 4);

    if (changed) {
        type_response.changed();
    }

    ImReflect::Detail::check_input_states(type_response);
}
