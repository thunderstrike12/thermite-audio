#include "window.hpp"

#include <SDL3/SDL.h>

#include "logger.hpp"

namespace tmt {

Window::~Window() {
    if (window && SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_DestroyWindow(window);
    }
}

void Window::init(const ApplicationSpecs& spec) {
    SDL_SetAppMetadata("Thermite Engine", "0.1", "com.thermite.engine");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        Log::error(Log::Scope::ENGINE, "Couldn't initialize SDL: %s", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow(spec.name.c_str(), width, height, SDL_WINDOW_VULKAN);
    if (!window) {
        Log::error(Log::Scope::ENGINE, "Couldn't create window: %s", SDL_GetError());
        return;
    }

    if (!SDL_SetWindowResizable(window, true)) {
        Log::error(Log::Scope::ENGINE, "Couldn't set the window to resize %s", SDL_GetError());
        return;
    }

    if (!SDL_MaximizeWindow(window)) {
        Log::error(Log::Scope::ENGINE, "Couldn't maximize the window %s", SDL_GetError());
        return;
    }
}

void Window::fullscreen_window(bool value) {
    if (value == fullscreen) return;
    if (!SDL_SetWindowFullscreen(window, value)) {
        Log::error(Log::Scope::ENGINE, "Couldn't set fullscreen mode: %s", SDL_GetError());
        return;
    }

    fullscreen = value;
    SDL_SyncWindow(window);
}

void Window::toggle_fullscreen() {
    fullscreen_window(!fullscreen);
}

void* Window::get_window_handle() const {
    const SDL_PropertiesID props = SDL_GetWindowProperties(window);

    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
}

}  // namespace tmt
