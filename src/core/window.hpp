#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

namespace photon {
    class Window {
    public:
        struct WindowConfig {
            uint32_t width, height;
        };

        Window(const WindowConfig& config) noexcept;
        ~Window() noexcept;

        SDL_Window* get_window() noexcept { return window_handle; }

    private:
        SDL_Window* window_handle;
    };
}