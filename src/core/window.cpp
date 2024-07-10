#include "window.hpp"

namespace photon {
    Window::Window(const WindowConfig& config) noexcept {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            // TODO: logger
        }

        window_handle = SDL_CreateWindow("photon app", config.width, config.height, 0);

        
    }

    Window::~Window() noexcept {

    }
}