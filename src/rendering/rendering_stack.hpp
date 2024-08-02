#pragma once

#include "vk_instance.hpp"
#include "vk_device.hpp"
#include "vk_display.hpp"

#include "forward/forward.hpp"

#include <resources/streamer.hpp>
#include <vector>

namespace photon::rendering {
    class rendering_stack {
    public:
        rendering_stack(window& target_window, uint32_t max_frames_in_flight) noexcept;
        ~rendering_stack() noexcept;

        void frame() noexcept;

    private:
        window& target_window;

        vulkan_instance vk_instance;
        vulkan_device vk_device;
        vulkan_display vk_display;

        batch_buffer shared_batch_buffer;
        asset_streamer streamer;

        // std::unique<renderer_interface> active_renderer;
        forward_renderer renderer;

        std::vector<std::shared_ptr<swapchain_handle>> frame_swapchains;
        std::vector<bool> frame_invalidated;

        // indexed by frame_index
        std::vector<vk::Fence> frame_fences;
        std::vector<vk::Semaphore> frame_acquire_sems;
        std::vector<vk::Semaphore> frame_ready_sems;

        uint32_t current_frame_index = 0; // for indexing resources [0, max_frames_in_flight)
        uint32_t current_image_index = 0; // for indexing swapchain [0, swapchain_image_count)
        uint32_t max_frames_in_flight;
    };
}