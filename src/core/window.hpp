#pragma once

#include <rendering/vk_instance.hpp>
#include <SDL3/SDL.h>

#include <bitset>
#include <cstdint>

namespace photon {
    class window {
    public:
        struct WindowConfig {
            uint32_t width, height;
        };

        window(const WindowConfig& config) noexcept;
        ~window() noexcept;

        // == events ==

        void poll_events() noexcept;
        bool should_close() const noexcept { return flags[window_flags::close_received]; }

        // == input api ==

        bool is_key_down(SDL_Scancode key_code) const noexcept {
            assert(key_code < SDL_Scancode::SDL_NUM_SCANCODES);

            return sdl_keyboard_state[key_code];
        }

        // captures or releases of the mouse from the window, true == capture, false == release
        void set_mouse_mode(bool relative_mode) noexcept {
            flags[window_flags::app_relative_mouse] = relative_mode;
            SDL_SetRelativeMouseMode(relative_mode);
        }

        // returns the mouse pos offset from last frame (must call set_mouse_mode(true) for relative mouse to output non-zero offsets)
        std::pair<float, float> get_relative_mouse_pos() const noexcept { return relative_mouse_pos; }
        
        // returns the mouse pos in screen-space coords
        std::pair<float, float> get_window_mouse_pos() const noexcept { return window_mouse_pos; }

        // == window ==

        SDL_Window* get_window() noexcept { return window_handle; }

        std::pair<uint32_t, uint32_t> get_extent() noexcept {
            std::pair<int, int> ext;
            SDL_GetWindowSizeInPixels(window_handle, &ext.first, &ext.second);

            return ext;
        }

        static void add_required_instance_extensions(std::vector<std::pair<const char*, bool>>& extensions) noexcept;
        vk::SurfaceKHR create_surface(rendering::vulkan_instance& instance) noexcept;

    private:
        SDL_Window* window_handle;

        enum window_flags {
            close_received = 0,
            app_relative_mouse,
            num_window_flags,
        };

        std::bitset<window_flags::num_window_flags> flags;

        // input states

        const uint8_t* sdl_keyboard_state;

        std::pair<float, float> relative_mouse_pos; // last mouse pos in relative_mode
        std::pair<float, float> window_mouse_pos; // last mouse pos in not relative_mode
        SDL_MouseButtonFlags mouse_button_state;
    };
}