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
            for (auto view : image_views) {
                owning_device.destroyImageView(view);
            }

            owning_device.destroySwapchainKHR(swapchain);
        }

        vk::SwapchainKHR swapchain;
        vk::Device owning_device;

        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_views;
    };

    class vulkan_display {
    public:
        struct swapchain_config {
            // if preferred_mode not supported, fifo (vsync) will be used
            vk::PresentModeKHR preferred_mode;
        };

        struct display_config {
            vulkan_device& device;
            vk::SurfaceKHR target_surface;

            swapchain_config initial_swapchain_config;

            // only used when surface has no current extent and is *ignored* otherwise
            vk::Extent2D initial_swapchain_extent;
            uint32_t min_swapchain_image_count;
        };

        vulkan_display(const display_config& config) noexcept;
        ~vulkan_display() noexcept;

        void recreate_swapchain(const std::optional<swapchain_config>& config);

        // used for "holding" onto images under a swapchain while there're still operations with it ongoing
        std::shared_ptr<swapchain_handle> get_swapchain() noexcept { return swapchain; }

        vk::SurfaceFormatKHR get_display_format() const noexcept { return surface_format; }
        vk::PresentModeKHR get_present_mode() const noexcept { return present_mode; }
        vk::Extent2D get_display_extent() const noexcept { return surface_extent; }

    private:
        vulkan_device& device;

        vk::SurfaceKHR target_surface;
        std::shared_ptr<swapchain_handle> swapchain;

        vk::SurfaceFormatKHR choose_surface_format();
        vk::PresentModeKHR choose_present_mode(vk::PresentModeKHR preferred_mode);

        vk::SurfaceFormatKHR surface_format;
        vk::PresentModeKHR present_mode;
        vk::Extent2D surface_extent;

        vk::Extent2D initial_swapchain_extent;
        uint32_t min_swapchain_image_count;
    };
}