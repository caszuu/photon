#include "window.hpp"
#include <SDL3/SDL_vulkan.h>

#include "abort.hpp"
#include "logger.hpp"

#include <span>

namespace photon {
    window::window(const WindowConfig& config) noexcept {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            // TODO: logger
        }

        window_handle = SDL_CreateWindow("photon app", config.width, config.height, SDL_WINDOW_VULKAN);

        if (!window_handle) {
            P_LOG_E("Failed to init a SDL window: {}", SDL_GetError());
            engine_abort();
        }
    }

    window::~window() noexcept {
        SDL_DestroyWindow(window_handle);
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