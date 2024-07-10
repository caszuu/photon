#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>
#include <set>

namespace photon::rendering {
    // a class containing the app-wide Vulkan instance, used for loading and "linking" the Vulkan API

    class vulkan_instance {
    public:
        struct instance_config {
            std::vector<std::pair<const char*, bool /*is_required*/>> requested_extensions;
            std::vector<std::pair<const char*, bool /*is_required*/>> requested_layers;
        };

        vulkan_instance(const instance_config& config) noexcept;
        ~vulkan_instance() noexcept;

        static uint32_t get_supported_api_version() noexcept {
            VULKAN_HPP_DEFAULT_DISPATCHER.init();

            try { return vk::enumerateInstanceVersion(); }
            catch(std::exception &e) { return 0; } // note: ignoring vk 1.0 here
        }

        // note: only the VK_ prefixed name marcos must be used as the pointers are used for the lookup
        bool is_extension_enabled(const char* name) noexcept { return enabled_extensions.contains(name); }
        bool is_layer_enabled(const char* name) noexcept { return enabled_layers.contains(name); }

        vk::Instance get_instance() const noexcept { return vk_instance; } 

    private:
        static std::vector<const char*> enable_extensions(const instance_config& config);
        static std::vector<const char*> enable_layers(const instance_config& config);

        vk::Instance vk_instance;
        vk::DebugUtilsMessengerEXT debug_messenger;

        std::set<const char*> enabled_extensions;
        std::set<const char*> enabled_layers;
    };
}