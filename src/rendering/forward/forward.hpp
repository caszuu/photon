#pragma once

#include "../vk_device.hpp"
#include "../vk_display.hpp"

#include <assets/texture.hpp>

namespace photon::rendering {
    struct frame_context {
        std::vector<vk::CommandBuffer>& cmds;
        std::shared_ptr<swapchain_handle> active_swapchain;

        uint32_t frame_index; 
    };

    class forward_renderer {
    public:
        forward_renderer(vulkan_device& device, vulkan_display& display, batch_buffer& shared_batch_buffer, uint32_t max_frames_in_flight) noexcept;
        ~forward_renderer() noexcept;

        void frame(const frame_context& ctx);
        void recreate(uint32_t frame_index);

    private:
        vulkan_device& device;
        vulkan_display& display;
        batch_buffer& batcher;

        std::vector<std::pair<vk::Image, VmaAllocation>> depth_images;
        std::vector<vk::ImageView> depth_views;

        uint32_t max_frames_in_flight;
    };
}