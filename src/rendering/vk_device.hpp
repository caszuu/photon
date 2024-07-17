#pragma once

#include "vk_instance.hpp"
#include "vma_usage.hpp"

#include <optional>
#include <span>

namespace photon::rendering {
    // a monolith class containing a Vulkan physical/logical device and its queues
    
    class vulkan_device {
    public:
        struct device_config {
            vulkan_instance& instance;

            // the [graphics_queue] will have support for presenting to this surface, null surface will skip the check
            vk::SurfaceKHR target_surface;

            std::vector<std::pair<const char*, bool /*is_required*/>> requested_extensions;
        };

        vulkan_device(const device_config& config) noexcept;
        ~vulkan_device() noexcept;

        vk::Device get_device() noexcept { return device; }
        vk::PhysicalDevice get_physical_device() noexcept { return physical_device; }
        vk::Queue get_queue(bool is_transfer) noexcept { return is_transfer && transfer_queue ? transfer_queue : graphics_queue; }
        vulkan_instance& get_instance() noexcept { return instance; }
        VmaAllocator get_allocator() noexcept { return allocator; }

        // [is_transfer] controls if should be submited to the transfer queue (if available)
        // note: fence is optional according to vulkan spec
        void submit(std::span<vk::SubmitInfo> submit_infos, vk::Fence fence, bool is_transfer = false);
    
        // note: only the VK_ prefixed name marcos must be used as the pointers are used for the lookup
        bool has_extension(const char* name) const noexcept { return active_extensions.contains(name); }
        uint32_t get_queue_family(bool is_transfer) const noexcept { return is_transfer && transfer_queue ? transfer_queue_family_index : graphics_queue_family_index; }

    private:
        static bool is_physical_device_suitable(vk::PhysicalDevice device, const device_config& config) noexcept;
        
        std::vector<const char*> enable_extensions(const device_config& config);

        // if [compat_surface] is supplied, the returned queue family will support presenting to that surface
        std::optional<vk::DeviceQueueCreateInfo> get_queue_info(std::span<vk::QueueFamilyProperties2> queue_families, const float* queue_priorities, vk::QueueFlags required_flags, vk::QueueFlags exclude_flags = {}, vk::SurfaceKHR compat_surface = {}) noexcept;

        vk::Device device;

        vk::Queue graphics_queue; // will also support present
        vk::Queue transfer_queue = {}; // note: will be the null if no special transfer queue family is supported
        
        vulkan_instance& instance;
        vk::PhysicalDevice physical_device;
        VmaAllocator allocator = VK_NULL_HANDLE;

        std::set<const char*> active_extensions;

        uint32_t graphics_queue_family_index = ~0U;
        uint32_t transfer_queue_family_index = ~0U; // note: same as [transfer_queue], will be ~0U if not supported
    };
}