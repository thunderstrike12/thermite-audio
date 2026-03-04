#pragma once
#include "editor/core/window.hpp"
#include "engine/events/input.hpp"
#include "engine/core/entity.hpp"

struct ImVec2;

namespace tmt {

class Viewport : public IWindow, public OnRetrieveMouseState {
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
    void snap_to_entity();
    void end_display() override;

    std::string get_title() const override;
    int get_window_flags() const override;
    constexpr bool is_closable() const override { return false; };
    constexpr bool default_open() const override { return true; }

    bool get_is_hovered() const { return is_hovered; }
    bool is_using_debug_camera() const { return using_debug_camera; }

   private:
    friend class ModelViewer;

    void update_debug_camera(const tmt::FrameData& frame_data);
    bool toolbar(const ImVec2& image_pos);
    void selection_logic(const ImVec2& imgui_mouse_pos, const ImVec2& image_pos, bool toolbar_buttons_hovered);

    float width = -1;
    float height = -1;

    bool is_hovered = false;
    glm::vec2 mouse_pos = { 0.0f, 0.0f };

    float camera_speed = 4.0f;
    bool using_debug_camera = false;

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

    void force_open_recurse_upwards(tmt::Entity entity);

    // Inherited via OnRetrieveMouseState
    void on_retrieve_mouse_state(MouseOverride& event) override;
};

}  // namespace tmt