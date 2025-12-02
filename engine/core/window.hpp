#pragma once

#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "core/application.hpp"

struct SDL_Window;

namespace tmt {

class Window {
   public:
    SDL_Window* window = nullptr;

    int width = 1280;
    int height = 720;
    std::string_view title {};

    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void init(const ApplicationSpecs& specs);

    HWND get_window_handle() const;
};

}  // namespace tmt
