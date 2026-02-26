#pragma once
#include <glm/glm.hpp>
#include <named_type.hpp>

namespace tmt {

using RGB = fluent::NamedType<glm::vec3, struct ColorRGBTag, fluent::ImplicitlyConvertibleTo<glm::vec4>::templ>;
using RGBA = fluent::NamedType<glm::vec4, struct ColorRGBATag, fluent::ImplicitlyConvertibleTo<glm::vec4>::templ>;

}  // namespace tmt