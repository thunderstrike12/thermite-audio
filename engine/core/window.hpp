#pragma once

#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef min
#undef max

#include "engine/core/application.hpp"

struct SDL_Window;

namespace tmt {

class Window {
   public:
    SDL_Window* window = nullptr;

    int width = 1920;
    int height = 1080;
    bool resized = false;
    std::string_view title = "Thermite Engine";

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void init(const ApplicationSpecs& specs);

    HWND get_window_handle() const;
};

}  // namespace tmt
