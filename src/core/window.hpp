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

        void poll_events() noexcept {
            SDL_Event event;

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    close_received = true;
                }
            }
        }

        SDL_Window* get_window() noexcept { return window_handle; }
        std::pair<uint32_t, uint32_t> get_extent() noexcept {
            std::pair<int, int> ext;
            SDL_GetWindowSizeInPixels(window_handle, &ext.first, &ext.second);

            return ext;
        }
        bool should_close() const noexcept { return close_received; }

        static void add_required_instance_extensions(std::vector<std::pair<const char*, bool>>& extensions) noexcept;
        vk::SurfaceKHR create_surface(rendering::vulkan_instance& instance) noexcept;

    private:
        SDL_Window* window_handle;
        bool close_received = false;
    };
}