#pragma once
#include <random>
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
class Random {
   public:
    static void set_seed(uint32_t seed) { rng().seed(seed); }

    static int rand_range(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng());
    }

    static int irand() { return rng()(); }

    static float rand_range(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng());
    }

    static glm::vec3 rand_unit_vec() {
        std::uniform_real_distribution<float> dist(0.f, 1.f);

        float z = dist(rng()) * 2.f - 1.f;
        float a = dist(rng()) * 2.f * glm::pi<float>();

        float r = std::sqrt(1.f - z * z);

        return glm::vec3(r * std::cos(a), r * std::sin(a), z);
    }

   private:
    static std::mt19937& rng() {
        static thread_local std::mt19937 engine { std::random_device {}() };
        return engine;
    }
};