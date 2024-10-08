#include "vk_device.hpp"

#include <core/abort.hpp>
#include <core/logger.hpp>
#include <span>

namespace photon::rendering {
    vulkan_device::vulkan_device(const device_config& config) noexcept :
        instance{config.instance}
    {
        try {
            // pick a physical device

            {
                std::vector<vk::PhysicalDevice> available_devices = instance.get_instance().enumeratePhysicalDevices();

                physical_device = VK_NULL_HANDLE;
                for (auto device : available_devices) {
                    if (is_physical_device_suitable(device, config)) {
                        physical_device = device;
                        break;
                    }
                }

                if (!physical_device) {
                    P_LOG_E("No usable Vulkan gpu found!");
                    engine_abort();
                }
            }

            vk::PhysicalDeviceProperties device_props = physical_device.getProperties();
            P_LOG_I("Using Vulkan {}.{}.{} device: {}", VK_VERSION_MAJOR(instance.get_instance_api_version()), VK_VERSION_MINOR(instance.get_instance_api_version()), VK_VERSION_PATCH(instance.get_instance_api_version()), device_props.deviceName.data());

            // create a logical device

            {
                std::array<vk::DeviceQueueCreateInfo, 2> queue_infos;
                std::array<float, 1> queue_priorities = { 1.f };

                {
                    std::vector<vk::QueueFamilyProperties2> queue_families = physical_device.getQueueFamilyProperties2();

                    auto main_queue = get_queue_info(queue_families, queue_priorities.data(), vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute, {}, config.target_surface);

                    if (!main_queue) {
                        P_LOG_E("No usable Vulkan queue found!");
                        engine_abort();
                    }
                    
                    queue_infos[0] = main_queue.value();
                    graphics_queue_family_index = queue_infos[0].queueFamilyIndex;

                    auto transfer_queue = get_queue_info(queue_families, queue_priorities.data(), vk::QueueFlagBits::eTransfer, vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);

                    if (transfer_queue) {
                        queue_infos[1] = transfer_queue.value();
                        transfer_queue_family_index = queue_infos[1].queueFamilyIndex;

                        P_LOG_D("Using a second Vulkan transfer queue!");
                    } else {
                        P_LOG_D("No usable transfer queue found, using main queue for transfers");
                    }
                }

                vk::PhysicalDeviceFeatures enabled_features{
                    .samplerAnisotropy = vk::True,
                };                

                std::vector<const char*> extensions = enable_extensions(config);
                active_extensions.insert(extensions.begin(), extensions.end());

                vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features> device_info{
                    vk::DeviceCreateInfo {
                        .queueCreateInfoCount = queue_infos.size(),
                        .pQueueCreateInfos = queue_infos.data(),
                        .enabledExtensionCount = extensions.size(),
                        .ppEnabledExtensionNames = extensions.data(),
                        .pEnabledFeatures = &enabled_features,
                    },
                    vk::PhysicalDeviceVulkan13Features{
                        .synchronization2 = vk::True,
                        .dynamicRendering = vk::True,
                    },
                };

                device = physical_device.createDevice(device_info.get<vk::DeviceCreateInfo>());

                // query allocated queues from device 

                graphics_queue = device.getQueue(graphics_queue_family_index, 0);

                if (~transfer_queue_family_index) {
                    transfer_queue = device.getQueue(transfer_queue_family_index, 0);
                }

                // load extension fn pointers
                VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
            }

            {
                // init VMA

                VmaVulkanFunctions vk_functions{
                    .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
                    .vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
                    .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
                    .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
                    .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
                    .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
                    .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
                    .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
                    .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
                    .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
                    .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
                    .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
                    .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
                    .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
                    .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
                    .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
                    .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
                    .vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2,
                    .vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2,
                    .vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2,
                    .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2,
                    .vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2,
                };

                if (instance.get_instance_api_version() >= VK_API_VERSION_1_3) {
                    vk_functions.vkGetDeviceBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceBufferMemoryRequirements;
                    vk_functions.vkGetDeviceImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceImageMemoryRequirements;
                }

                VmaAllocatorCreateInfo allocator_info{
                    .physicalDevice = physical_device,
                    .device = device,
                    .pVulkanFunctions = &vk_functions,
                    .instance = instance.get_instance(),
                    .vulkanApiVersion = instance.get_instance_api_version(),
                };

                VkResult res = vmaCreateAllocator(&allocator_info, &allocator);
                vk::resultCheck(static_cast<vk::Result>(res), "Failed to create a VMA allocator");
            }

        } catch (std::exception& e) {
            P_LOG_E("Failed to init Vulkan: {}", e.what());
            engine_abort();
        }
    }

    vulkan_device::~vulkan_device() noexcept {
        vmaDestroyAllocator(allocator);
        device.destroy();
    }

    void vulkan_device::submit(std::span<vk::SubmitInfo> submit_infos, vk::Fence fence, bool is_transfer) {
        if (is_transfer && transfer_queue) {
            transfer_queue.submit(submit_infos, fence);
            return;
        }

        graphics_queue.submit(submit_infos, fence);
    }

    bool vulkan_device::is_physical_device_suitable(vk::PhysicalDevice device, const device_config& config) noexcept {
        return true; // assume always capable (allow the user to reorder devices if wanted)
    }

    std::vector<const char*> vulkan_device::enable_extensions(const device_config& config) {
        std::vector<vk::ExtensionProperties> available_extensions = physical_device.enumerateDeviceExtensionProperties();
        std::vector<const char*> enabled_extensions;

        for (auto req_ext : config.requested_extensions) {
            bool found = false;

            for (const auto& ext : available_extensions) {
                if (std::string_view(req_ext.first) == ext.extensionName) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (!req_ext.second) continue;

                throw std::runtime_error(std::format("Requested Vulkan device extension not supported: {}", req_ext.first));
            }

            enabled_extensions.emplace_back(req_ext.first);
        }

        return enabled_extensions;
    }

    std::optional<vk::DeviceQueueCreateInfo> vulkan_device::get_queue_info(std::span<vk::QueueFamilyProperties2> queue_families, const float* queue_priorities, vk::QueueFlags required_flags, vk::QueueFlags exclude_flags, vk::SurfaceKHR compat_surface) noexcept {
        for (uint32_t i = 0; i < queue_families.size(); i++) {
            if (!queue_families[i].queueFamilyProperties.queueCount) continue;

            if (queue_families[i].queueFamilyProperties.queueFlags & required_flags && !(queue_families[i].queueFamilyProperties.queueFlags & exclude_flags)) {
                if (compat_surface) {
                    if (!physical_device.getSurfaceSupportKHR(i, compat_surface)) continue;
                }
                
                vk::DeviceQueueCreateInfo queue{
                    .queueFamilyIndex = i,
                    .queueCount = 1,
                    .pQueuePriorities = queue_priorities,
                };

                // queue_families[i].queueFamilyProperties.queueCount--;
                return queue;
            }
        }

        return std::nullopt;
    }
}