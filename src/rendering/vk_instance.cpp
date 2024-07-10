#include "vk_instance.hpp"

#include <core/logger.hpp>
#include <core/abort.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace photon::rendering {
    std::vector<const char*> vulkan_instance::enable_extensions(const instance_config& config) {
        std::vector<vk::ExtensionProperties> available_extensions = vk::enumerateInstanceExtensionProperties();
        std::vector<const char*> enabled_extensions;

        for (const auto& req_ext : config.requested_extensions) {
            bool found = false;

            for (const auto& ext : available_extensions) {
                if (std::string_view(req_ext.first) == ext.extensionName) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (req_ext.second) continue;

                throw std::runtime_error(std::format("Requested instance extension not supported by device: {}", req_ext.first));
            }

            enabled_extensions.emplace_back(req_ext.first);
        }

        return enabled_extensions;
    }

    std::vector<const char*> vulkan_instance::enable_layers(const instance_config& config) {
        std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();
        std::vector<const char*> enabled_layers;

        for (const auto& req_layer : config.requested_layers) {
            bool found = false;

            for (const auto& ext : available_layers) {
                if (std::string_view(req_layer.first) == ext.layerName) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (req_layer.second) continue;

                throw std::runtime_error(std::format("Requested instance layer not supported by device: {}", req_layer.first));
            }

            enabled_layers.emplace_back(req_layer.first);
        }

        return enabled_layers;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) noexcept {

        if (messageType == VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
                P_LOG_D("[Vulkan Validation]: {}", pCallbackData->pMessage);
            } else if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
                P_LOG_I("[Vulkan Validation]: {}", pCallbackData->pMessage);
            } else if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                P_LOG_W("[Vulkan Validation]: {}", pCallbackData->pMessage);
            } else {
                P_LOG_E("[Vulkan Validation]: {}", pCallbackData->pMessage);
            }
        } else {
            if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
                P_LOG_D("[Vulkan Debug]: {}", pCallbackData->pMessage);
            } else if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
                P_LOG_I("[Vulkan Debug]: {}", pCallbackData->pMessage);
            } else if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                P_LOG_W("[Vulkan Debug]: {}", pCallbackData->pMessage);
            } else {
                P_LOG_E("[Vulkan Debug]: {}", pCallbackData->pMessage);
            }
        }

        return VK_FALSE;
    }

    vulkan_instance::vulkan_instance(const instance_config& config) noexcept {
        if (get_supported_api_version() < VK_API_VERSION_1_3) {

            P_LOG_E("Vulkan 1.3 is required and not supported on this device!");
            engine_abort();
        }

        try {
            {
                vk::ApplicationInfo app_info{
                    .pApplicationName = "photon app",
                    .applicationVersion = 1,
                    .pEngineName = "photon",
                    .engineVersion = 1,
                    .apiVersion = VK_API_VERSION_1_3,
                };

                std::vector<const char*> extensions = enable_extensions(config);
                std::vector<const char*> layers = enable_layers(config);

                vk::InstanceCreateInfo instance_info{
                    .pApplicationInfo = &app_info,
                    .enabledLayerCount = layers.size(),
                    .ppEnabledLayerNames = layers.data(),
                    .enabledExtensionCount = extensions.size(),
                    .ppEnabledExtensionNames = extensions.data(),
                };

                vk_instance = vk::createInstance(instance_info);

                enabled_extensions.insert(extensions.begin(), extensions.end());
                enabled_layers.insert(layers.begin(), layers.end());
            }
        
            // load instance extension func pointers
            VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_instance);
            
            if (enabled_extensions.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
                vk::DebugUtilsMessengerCreateInfoEXT debug_message_info{
                    .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                    .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                    .pfnUserCallback = &vulkan_messenger_callback,
                };

                debug_messenger = vk_instance.createDebugUtilsMessengerEXT(debug_message_info);
            }

        } catch(std::exception &e) {
            P_LOG_E("Failed to init Vulkan: {}", e.what());
            engine_abort();
        }
    }

    vulkan_instance::~vulkan_instance() {
        vk_instance.destroy();
    }
}