#include "window.hpp"
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_mouse.h>

#include "abort.hpp"
#include "logger.hpp"

#include <span>

namespace photon {
    window::window(const WindowConfig& config) noexcept {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            // TODO: logger
        }

        window_handle = SDL_CreateWindow("photon app", config.width, config.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        if (!window_handle) {
            P_LOG_E("Failed to init a SDL window: {}", SDL_GetError());
            engine_abort();
        }

        sdl_keyboard_state = SDL_GetKeyboardState(nullptr);
    }

    window::~window() noexcept {
        SDL_DestroyWindow(window_handle);
    }

    void window::poll_events() noexcept {
        bool recaptured_mouse = false;

        // event processing

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                flags[window_flags::close_received] = true;
                break;
                
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                SDL_SetRelativeMouseMode(SDL_FALSE);
                relative_mouse_pos = std::make_pair(0.f, 0.f);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (flags[window_flags::app_relative_mouse] && !SDL_GetRelativeMouseMode()) {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    recaptured_mouse = true;
                }

                break;

            default:
                break;
            }
        }

        // mouse input processing

        if (SDL_GetRelativeMouseMode()) {
            // handle relative mouse
                
            int mid_x, mid_y;
            SDL_GetWindowSize(window_handle, &mid_x, &mid_y);
            mid_x /= 2; mid_y /= 2;

            if (!recaptured_mouse) {
                mouse_button_state = SDL_GetMouseState(&relative_mouse_pos.first, &relative_mouse_pos.second);

                relative_mouse_pos.first -= mid_x;
                relative_mouse_pos.second -= mid_y;
            }

            SDL_WarpMouseInWindow(window_handle, mid_x, mid_y);
        } else {
            // handle window mouse

            mouse_button_state = SDL_GetMouseState(&window_mouse_pos.first, &window_mouse_pos.second);
        }
    }

    void window::add_required_instance_extensions(std::vector<std::pair<const char*, bool>>& extensions) noexcept {
        uint32_t ext_count;
        const char *const * ext_data = SDL_Vulkan_GetInstanceExtensions(&ext_count);
        
        std::span<const char *const> sdl_extensions(ext_data, ext_count);
        for (auto* ext : sdl_extensions) {
            extensions.emplace_back(ext, true);
        }
    }

    vk::SurfaceKHR window::create_surface(rendering::vulkan_instance& instance) noexcept {
        VkSurfaceKHR surface;
        int err = SDL_Vulkan_CreateSurface(window_handle, instance.get_instance(), nullptr, &surface);

        if (err) {
            P_LOG_E("Failed to create a Vulkan surface from SDL: {}", SDL_GetError());
            engine_abort();
        }

        return static_cast<vk::SurfaceKHR>(surface);
    }
}