#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

#include <rendering/vk_instance.hpp>

namespace photon {
    class window {
    public:
        struct WindowConfig {
            uint32_t width, height;
        };

        window(const WindowConfig& config) noexcept;
        ~window() noexcept;

        SDL_Window* get_window() noexcept { return window_handle; }

        static void add_required_instance_extensions(std::vector<std::pair<const char*, bool>>& extensions) noexcept;
        vk::SurfaceKHR create_surface(rendering::vulkan_instance& instance) noexcept;

    private:
        SDL_Window* window_handle;
    };
}