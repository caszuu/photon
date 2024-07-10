#include "rendering_stack.hpp"

namespace photon::rendering {
    inline static vulkan_instance create_vk_instance() noexcept {
        std::vector<std::pair<const char*, bool>> extensions;
        window::add_required_instance_extensions(extensions);
        
        vulkan_instance::instance_config config{
            .requested_extensions = std::move(extensions),
            .requested_layers = {},
        };

        return vulkan_instance(config);
    }

    inline static vulkan_device create_vk_device(vulkan_instance& instance) noexcept {
        std::vector<std::pair<const char*, bool>> extensions;
        extensions.emplace_back(std::make_pair(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true));
        
        vulkan_device::device_config config{
            .instance = instance,
            .requested_extensions = std::move(extensions),
        };

        return vulkan_device(config);
    }

    rendering_stack::rendering_stack(window& target_window) noexcept :
        target_window{target_window},
        vk_instance{create_vk_instance()},
        vk_device{create_vk_device(vk_instance)},
        vk_display{vulkan_display::display_config{
            .device = vk_device,
            .target_surface = target_window.create_surface(vk_instance),
            .preferred_mode = vk::PresentModeKHR::eFifo,
            .min_swapchain_image_count = 2,
        }}
    {

    }

    rendering_stack::~rendering_stack() noexcept {

    }
}