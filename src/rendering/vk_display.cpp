#include "vk_display.hpp"

#include <core/abort.hpp>
#include <core/logger.hpp>
#include <cassert>

namespace photon::rendering {
    vulkan_display::vulkan_display(const display_config& config) noexcept :
        device{config.device},
        target_surface{config.target_surface},
        initial_swapchain_extent{config.initial_swapchain_extent},
        min_swapchain_image_count{config.min_swapchain_image_count}
    {
        assert(target_surface && "null target_surface");

        try {
            refresh_swapchain(config.initial_swapchain_config);
        } catch (std::exception &e) {
            P_LOG_E("Failed to create a new Vulkan Swapchain: {}", e.what());
            engine_abort();
        }
    }

    vulkan_display::~vulkan_display() noexcept {
        assert(swapchain.unique() && "Swapchain handles still exists while destroying its surface"); // assume all swapchain handles are released
        swapchain.reset();

        device.get_instance().get_instance().destroySurfaceKHR(target_surface);
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

    void vulkan_display::refresh_swapchain(const std::optional<swapchain_config>& config) {
        std::shared_ptr<swapchain_handle> old_swapchain = swapchain;

        {
            // create the swapchain

            vk::SurfaceCapabilitiesKHR surface_caps = device.get_physical_device().getSurfaceCapabilitiesKHR(target_surface);
            
            surface_format = choose_surface_format();
            surface_extent = surface_caps.currentExtent.width == ~0U && surface_caps.currentExtent.height == ~0U ? initial_swapchain_extent : surface_caps.currentExtent;

            if (config)
                present_mode = choose_present_mode(config->preferred_mode);

            vk::SwapchainCreateInfoKHR swapchain_info{
                .surface = target_surface,
                .minImageCount = min_swapchain_image_count,
                .imageFormat = surface_format.format,
                .imageColorSpace = surface_format.colorSpace,
                .imageExtent = surface_extent,
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

            swapchain->images = device.get_device().getSwapchainImagesKHR(swapchain->swapchain);

            vk::ImageViewCreateInfo view_info{
                .viewType = vk::ImageViewType::e2D,
                .format = surface_format.format,
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            swapchain->image_views.reserve(swapchain->images.size());
            for (auto image : swapchain->images) {
                view_info.image = image;
                swapchain->image_views.emplace_back(device.get_device().createImageView(view_info));
            }
        }
    }
}