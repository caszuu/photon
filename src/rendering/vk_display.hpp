#pragma once

#include "vk_device.hpp"
#include <core/window.hpp>

#include <memory>

namespace photon::rendering {
    struct swapchain_handle {
        swapchain_handle(vk::SwapchainKHR swapchain, vk::Device device) noexcept : 
            swapchain{swapchain},
            owning_device{device}
        {

        }

        ~swapchain_handle() noexcept {
            owning_device.destroySwapchainKHR(swapchain);
        }

        vk::SwapchainKHR swapchain;
        vk::Device owning_device;
    };

    class vulkan_display {
    public:
        struct display_config {
            vulkan_device& device;
            vk::SurfaceKHR target_surface;

            // if preferred_mode not supported, fifo (vsync) will be used
            vk::PresentModeKHR preferred_mode;
            uint32_t min_swapchain_image_count;
        };

        vulkan_display(const display_config& config) noexcept;
        ~vulkan_display() noexcept;

        void recreate_swapchain();

        // used for "holding" onto images under a swapchain while there're still operations with it ongoing
        std::shared_ptr<swapchain_handle> get_swapchain() noexcept { return swapchain; }

    private:
        vulkan_device& device;

        vk::SurfaceKHR target_surface;
        std::shared_ptr<swapchain_handle> swapchain;

        vk::SurfaceFormatKHR choose_surface_format();
        vk::PresentModeKHR choose_present_mode(vk::PresentModeKHR preferred_mode);

        vk::SurfaceFormatKHR surface_format;
        vk::PresentModeKHR present_mode;

        vk::PresentModeKHR preferred_mode;
        uint32_t min_swapchain_image_count;
    };
}