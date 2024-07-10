#pragma once

#include "vk_instance.hpp"
#include "vk_device.hpp"
#include "vk_display.hpp"

namespace photon::rendering {
    class rendering_stack {
    public:
        rendering_stack(window& target_window) noexcept;
        ~rendering_stack() noexcept;

    private:
        window& target_window;

        vulkan_instance vk_instance;
        vulkan_device vk_device;
        vulkan_display vk_display;

        // std::unique<renderer_interface> active_renderer;
    };
}