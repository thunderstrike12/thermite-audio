#pragma once

#include <string_view>

#include "engine/core/application.hpp"

struct SDL_Window;

namespace tmt {

class Window {
   public:
    SDL_Window* window = nullptr;

    int width = 1920;
    int height = 1080;
    bool resized = false;

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void init(const ApplicationSpecs& specs);

    void fullscreen_window(bool value);
    void toggle_fullscreen();
    bool is_fullscreen() const { return fullscreen; }

    void* get_window_handle() const;

   private:
    bool fullscreen = false;
};

}  // namespace tmt
