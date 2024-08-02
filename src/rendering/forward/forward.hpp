#pragma once

#include "../vk_device.hpp"
#include "../vk_display.hpp"

#include <resources/texture.hpp>

namespace photon::rendering {
    struct frame_context {
        std::vector<vk::CommandBuffer>& cmds;
        std::shared_ptr<swapchain_handle> active_swapchain;

        uint32_t frame_index; 
    };

    // a simple straigthforward forward photon renderer implementation (single-threaded, single-cmd and single-pass)

    class forward_renderer {
    public:
        forward_renderer(vulkan_device& device, vulkan_display& display, batch_buffer& shared_batch_buffer, uint32_t max_frames_in_flight) noexcept;
        ~forward_renderer() noexcept;

        void frame(const frame_context& ctx);
        void refresh(uint32_t frame_index);

        // query vulkan images and image layouts that will contain rendering outputs (color, depth_stencil, normal, etc.), used by the post-processing stack
        // photon_buffers_layout get_rendering_layout() noexcept;

    private:
        void create_frame_resources(uint32_t frame_index);

        vulkan_device& device;
        vulkan_display& display;
        batch_buffer& batcher;

        std::vector<std::pair<vk::Image, VmaAllocation>> depth_images;
        std::vector<vk::ImageView> depth_views;

        uint32_t max_frames_in_flight;
    };
}