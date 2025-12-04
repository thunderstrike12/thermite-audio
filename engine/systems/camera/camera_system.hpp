#pragma once

#include <string>

#include "core/system.hpp"

namespace tmt {

constexpr const char* SPRINT = "Camera Sprint";
constexpr const char* FORWARD = "Camera Move Forward";
constexpr const char* BACKWARD = "Camera Move Backward";
constexpr const char* RIGHT = "Camera Move Right";
constexpr const char* LEFT = "Camera Move Left";
constexpr const char* UP = "Camera Move Up";
constexpr const char* DOWN = "Camera Move Down";

class CameraSystem : public ISystem {
    constexpr virtual std::string get_name() { return "Camera System"; }

    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_end() override;

    float base_speed = 4.0f;
    float sprint_mult = 3.0f;
    float cam_sensitivity = 0.1f;
};
}  // namespace tmt