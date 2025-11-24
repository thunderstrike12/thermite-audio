#pragma once

#include <string_view>

class SDL_Window;

namespace tmt {

class Window {
   public:
    SDL_Window* window = nullptr;

    int width = 1920;
    int height = 1080;
    std::string_view title {};
    bool is_running = true;

    Window() = default;
    ~Window();

    void init() const;
    void update();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

}  // namespace tmt