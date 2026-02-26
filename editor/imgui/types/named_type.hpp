#pragma once
#include <named_type.hpp>
#include "ImReflect.hpp"

template <typename T, typename Parameter, template <typename> class... Skills>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, fluent::NamedType<T, Parameter, Skills...>& type, ImSettings& settings, ImResponse& response) {
    ImReflect::Input(label, type.get(), settings, response);
}