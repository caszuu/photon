#include "batch_buffer.hpp"

namespace photon::rendering {
    batch_buffer::batch_buffer(vulkan_device& device, uint32_t max_frame_in_flight, bool is_transfer) :
        device{device},
        active_batch_index{max_frame_in_flight /*set to invalid*/},
        max_frame_in_flight{max_frame_in_flight},
        is_transfer{is_transfer}
    {
        frame_batches.reserve(max_frame_in_flight);

        for (uint32_t i = 0; i < max_frame_in_flight; i++) {
            vk::CommandPoolCreateInfo pool_info;
            pool_info.queueFamilyIndex = device.get_queue_family(is_transfer);

            vk::FenceCreateInfo fence_info {
                .flags = vk::FenceCreateFlagBits::eSignaled,
            };
            
            frame_batches.emplace_back(std::vector<vk::CommandBuffer>{}, device.get_device().createCommandPool(pool_info), device.get_device().createFence(fence_info), 0);
        }
    }
    
    batch_buffer::~batch_buffer() noexcept {
        // assume device is idle

        for (auto& batch : frame_batches) {
            device.get_device().destroyCommandPool(batch.batch_pool);
        }
    }

    vk::CommandBuffer batch_buffer::begin_recording(const vk::CommandBufferBeginInfo& begin_info) {
        assert(active_batch_index != max_frame_in_flight);
        FrameBatch& batch = frame_batches[active_batch_index];
        
        if (batch.cmds_in_use + 1 == batch.batch_cmds.size()) {
            // ran out of idle cmds, allocate a new one

            vk::CommandBufferAllocateInfo alloc_info{
                .commandPool = batch.batch_pool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
            };

            vk::CommandBuffer cmd;
            vk::Result res = device.get_device().allocateCommandBuffers(&alloc_info, &cmd); // avoid pointless std::vector alloc
            vk::resultCheck(res, "Failed to allocate a new command buffer");

            batch.batch_cmds.emplace_back(cmd);
        }
        
        vk::CommandBuffer cmd = batch.batch_cmds[batch.cmds_in_use++];
        cmd.begin(begin_info);

        return cmd;
    }

    // FIXME: overall submit semaphores, fences
    void batch_buffer::submit_batch(std::span<vk::CommandBuffer> cmds) {
        assert(active_batch_index != max_frame_in_flight);
        FrameBatch& batch = frame_batches[active_batch_index];
        active_batch_index = max_frame_in_flight;

        vk::SubmitInfo submit_info{
            /*.waitSemaphoreCount = ,
            .pWaitDstStageMask = ,
            .pWaitSemaphores = ,
            .signalSemaphoreCount = ,
            .pSignalSemaphores = , */
            .commandBufferCount = cmds.size(),
            .pCommandBuffers = cmds.data(),
        };

        device.submit(std::span(&submit_info, 1), batch.frame_fence, is_transfer);
    }
}