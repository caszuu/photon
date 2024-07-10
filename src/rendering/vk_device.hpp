#pragma once

#include "vk_instance.hpp"

#include <optional>
#include <span>

namespace photon::rendering {
    // a monolith class containing a Vulkan physical/logical device and its queues
    
    class vulkan_device {
    public:
        struct device_config {
            vulkan_instance& instance;

            std::vector<std::pair<const char*, bool /*is_optional*/>> requested_extensions;
        };

        vulkan_device(const device_config& config) noexcept;
        ~vulkan_device() noexcept;

        vk::Device get_device() noexcept { return device; }

        MultiFenceView submit(bool is_transfer = true);
    
        // note: only the VK_ prefixed name marcos must be used as the pointers are used for the lookup
        bool has_extension(const char* name) const noexcept { return active_extensions.contains(name); }
        uint32_t get_queue_family() const noexcept { return  }

    private:
        static bool is_physical_device_suitable(vk::PhysicalDevice device, const device_config& config) noexcept;
        
        std::vector<const char*> enable_extensions(const device_config& config);
        std::optional<vk::DeviceQueueCreateInfo> get_queue_info(std::span<vk::QueueFamilyProperties2> queue_families, vk::QueueFlags required_flags, vk::QueueFlags exclude_flags) noexcept;

        vk::Device device;

        vk::Queue graphics_queue; // will also support present
        vk::Queue transfer_queue = {}; // note: will be the null if no special transfer queue family is supported
        
        vulkan_instance& instance;
        vk::PhysicalDevice physical_device;

        std::set<const char*> active_extensions;

        uint32_t graphics_queue_family_index = ~0U;
        uint32_t transfer_queue_family_index = ~0U;
    };
}