#pragma once

#include <cstdint> /* uint32_t, int16_t, etc */
#include <cmath>   /* fminf, fmaxf, powf, etc */
#include <utility> /* std::swap */

#include <glm/glm.hpp> /* vector math */

/* Large 32-bit floating point constant. */
constexpr float BIG_F32 = 1e30f;
