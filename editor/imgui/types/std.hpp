#pragma once
#include <ImReflect.hpp>
#include <filesystem>

template <typename T>
concept ExactlyPath = std::same_as<std::remove_cvref_t<T>, std::filesystem::path>;

void tag_invoke(ImReflect::ImInput_t, const char* label, ExactlyPath auto& value, ImSettings& settings, ImResponse& response) {
    auto str = value.string();
    ImReflect::Input(label, str, settings, response);
    value = std::filesystem::path(str);
}

void tag_invoke(ImReflect::ImInput_t, const char* label, const ExactlyPath auto& value, ImSettings& settings, ImResponse& response) {
    const auto& str = value.string();
    ImReflect::Input(label, str, settings, response);
}

/*
void tag_invoke(ImReflect::ImInput_t, const char* label, std::vector<tmt::VoxelSceneNode>& value, ImSettings& settings, ImResponse& response) {
    constexpr bool is_const = false;
    constexpr bool allow_insert = false;
    constexpr bool allow_remove = true;
    constexpr bool all_reorder = true;
    constexpr bool allow_copy = true;

    ImReflect::Detail::container_input<ImReflect::std_vector, std::vector<tmt::VoxelSceneNode>, is_const, allow_insert, allow_remove, all_reorder, allow_copy>(
        label, value, settings, response
    );
}

void tag_invoke(ImReflect::ImInput_t, const char* label, const std::vector<tmt::VoxelSceneNode>& value, ImSettings& settings, ImResponse& response) {
    constexpr bool is_const = true;
    constexpr bool allow_insert = false;
    constexpr bool allow_remove = false;
    constexpr bool all_reorder = false;
    constexpr bool allow_copy = false;

    ImReflect::Detail::container_input<ImReflect::std_vector, std::vector<tmt::VoxelSceneNode>, is_const, allow_insert, allow_remove, all_reorder, allow_copy>(
        label, value, settings, response
    );
}
*/