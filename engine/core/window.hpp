#pragma once

#include <string_view>

#include "core/application.hpp"

struct SDL_Window;

namespace tmt {

class Window {
   public:
    SDL_Window* window = nullptr;

    int width = 1280;
    int height = 720;
    std::string_view title {};
    bool is_running = true;

    Window() = default;
    ~Window();

    void init(const ApplicationSpecs& specs);
    void update();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

}  // namespace tmt