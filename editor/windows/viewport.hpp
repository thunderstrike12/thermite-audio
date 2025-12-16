#pragma once
#include "editor/core/window.hpp"

namespace tmt {
class Viewport : public IWindow {
   public:
    Viewport() = default;
    ~Viewport() = default;

    // Inherited via IWindow
    void on_editor_start() override;
    void register_input_actions();
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    void before_begin() override;
    void display() override;
    void end_display() override;

    constexpr std::string get_title() const override { return ICON_MS_VISIBILITY " Viewport"; };
    constexpr bool is_closable() const override { return false; };

   private:
    void gizmo_manip();
    void setup_gizmo_style();
    void update_debug_camera(const tmt::FrameData& frame_data);
    void toolbar(const glm::vec2& image_pos);

    uint8_t gizmo_space = 1;
    uint8_t gizmo_multiselect_mode = 0;
    uint8_t gizmo_operation = 0;

    static constexpr uint8_t GIZMO_OP_COUNT = 3;

    uint32_t gizmo_operations[GIZMO_OP_COUNT] = {7, 120, 896};
    const char* gizmo_op_icons[GIZMO_OP_COUNT] = {ICON_MS_DRAG_PAN, ICON_MS_ROTATE_RIGHT, ICON_MS_ZOOM_OUT_MAP};

    float width = -1;
    float height = -1;

    bool is_hovered = false;

    float camera_speed = 4.0f;

    struct Config {
        constexpr static float SPEED_CHANGE_FACTOR = 0.5f;
        constexpr static float MIN_BASE_SPEED = 1.0f;
        constexpr static float MAX_BASE_SPEED = 100.0f;
        constexpr static float MOUSE_SENSITIVITY = 0.1f;

        constexpr static const char* SPRINT = "Camera Sprint";
        constexpr static const char* FORWARD = "Camera Move Forward";
        constexpr static const char* BACKWARD = "Camera Move Backward";
        constexpr static const char* RIGHT = "Camera Move Right";
        constexpr static const char* LEFT = "Camera Move Left";
        constexpr static const char* UP = "Camera Move Up";
        constexpr static const char* DOWN = "Camera Move Down";
    };
};
}  // namespace tmt