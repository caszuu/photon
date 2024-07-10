#pragma once

#include "vk_device.hpp"

#include <vector>
#include <cassert>

namespace photon::rendering {
    class batch_buffer {
    public:
        batch_buffer(vulkan_device& device, uint32_t max_frame_in_flight, bool is_transfer);
        ~batch_buffer() noexcept;

        // resets batch buffer and sets a active_batch_index for new cmds
        void reset_batch(uint32_t frame_index) noexcept {
            assert(active_batch_index == max_frame_in_flight);

            active_batch_index = frame_index;
            frame_batches[active_batch_index].cmds_in_use = 0;

            device.get_device().resetCommandPool(frame_batches[active_batch_index].batch_pool);
        }

        vk::CommandBuffer begin_recording(const vk::CommandBufferBeginInfo& begin_info);

        // submits cmds to the vulkan queue; after a submit it's incorect to begin_recording() before calling reset_batch() on the next frame
        void submit_batch(std::span<vk::CommandBuffer> cmds);

    private:
        vulkan_device& device;

        struct FrameBatch {
            std::vector<vk::CommandBuffer> batch_cmds;
            vk::CommandPool batch_pool;

            vk::Fence frame_fence;
            uint32_t cmds_in_use;
        };

        std::vector<FrameBatch> frame_batches;
        
        uint32_t active_batch_index; // note: active_batch_index == max_frame_in_flight means no active batch (waiting for reset)
        uint32_t max_frame_in_flight;
        bool is_transfer;
    };
}