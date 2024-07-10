#include "vk_display.hpp"

#include <core/abort.hpp>
#include <core/logger.hpp>
#include <cassert>

namespace photon::rendering {
    vulkan_display::vulkan_display(const display_config& config) noexcept :
        device{config.device},
        target_surface{config.target_surface},
        preferred_mode{config.preferred_mode},
        min_swapchain_image_count{config.min_swapchain_image_count}
    {
        assert(target_surface && "null target_surface");

        try {
            recreate_swapchain();
        } catch (std::exception &e) {
            P_LOG_E("Failed to create a new Vulkan Swapchain: {}", e.what());
            engine_abort();
        }
    }

    vulkan_display::~vulkan_display() noexcept {
        // SDL_Vulkan_DestroySurface();
        device.get_instance().get_instance().destroySurfaceKHR(target_surface);
        // device.get_device().destroySwapchainKHR(swapchain);
    }

    vk::SurfaceFormatKHR vulkan_display::choose_surface_format() {
        std::vector<vk::SurfaceFormatKHR> formats = device.get_physical_device().getSurfaceFormatsKHR(target_surface);

        for (auto& format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) return format;
        }

        return formats[0];
    }

    vk::PresentModeKHR vulkan_display::choose_present_mode(vk::PresentModeKHR preferred_mode) {
        std::vector<vk::PresentModeKHR> modes = device.get_physical_device().getSurfacePresentModesKHR(target_surface);

        for (auto mode : modes) {
            if (preferred_mode == mode) return mode;
        }

        return vk::PresentModeKHR::eFifo;
    }

    void vulkan_display::recreate_swapchain() {
        std::shared_ptr<swapchain_handle> old_swapchain = swapchain;

        {
            // create the swapchain

            vk::SurfaceCapabilitiesKHR surface_caps = device.get_physical_device().getSurfaceCapabilitiesKHR(target_surface);
            
            surface_format = choose_surface_format();
            present_mode = choose_present_mode(preferred_mode);

            vk::SwapchainCreateInfoKHR swapchain_info{
                .surface = target_surface,
                .minImageCount = min_swapchain_image_count,
                .imageFormat = surface_format.format,
                .imageColorSpace = surface_format.colorSpace,
                .imageExtent = surface_caps.currentExtent,
                .imageArrayLayers = 1,
                .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                .imageSharingMode = vk::SharingMode::eExclusive,
                .preTransform = surface_caps.currentTransform,
                .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                .presentMode = present_mode,
                .clipped = vk::True, // may change, fine for now
                .oldSwapchain = old_swapchain ? old_swapchain->swapchain : vk::SwapchainKHR{},
            };

            swapchain = std::make_shared<swapchain_handle>(device.get_device().createSwapchainKHR(swapchain_info), device.get_device());
        }

        {
            // query sapchain images
        }
    }
}