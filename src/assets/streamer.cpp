#include "streamer.hpp"
#include <cassert>

#include <core/logger.hpp>

namespace photon::rendering {
    asset_streamer::asset_streamer(rendering::vulkan_device& device, uint32_t max_frames_in_flight) noexcept :
        device{device},
        batch_cmd_buffer{device, max_frames_in_flight, true},
        max_frames_in_flight{max_frames_in_flight}
    {
        vk::FenceCreateInfo fence_info{
            .flags = vk::FenceCreateFlagBits::eSignaled,
        };

        vk::SemaphoreCreateInfo semaphore_info{};

        batch_buffers.reserve(max_frames_in_flight);
        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            multi_fence fence(device, fence_info);

            batch_buffers.emplace_back(fence, device.get_device().createSemaphore(semaphore_info));
        }
    }

    asset_streamer::~asset_streamer() noexcept {
        for (auto& batch : batch_buffers) {
            assert(batch.ready_fence.status() == vk::Result::eSuccess); // assume device is idle
            
            for (auto& stream : batch.blocking.buffer_infos) {
                vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
            }
            for (auto& stream : batch.blocking.image_infos) {
                vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
            }

            for (auto& stream : batch.deferred.buffer_infos) {
                vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
            }
            for (auto& stream : batch.deferred.image_infos) {
                vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
            }

            device.get_device().destroySemaphore(batch.blocking_ready_semaphore);
        }
    }

    vk::Semaphore asset_streamer::submit_batch() {
        frame_buffer& batch = batch_buffers[current_frame_index];
        // assert(batch.ready_fence.status() == vk::Result::eSuccess);

        vk::CommandBuffer blocking_cmd = begin_stream_recording();
        record_streams(blocking_cmd, batch.blocking);
        blocking_cmd.end();

        vk::CommandBuffer deferred_cmd = begin_stream_recording();
        record_streams(deferred_cmd, batch.deferred);
        deferred_cmd.end();

        // submit current batch

        std::array<vk::SubmitInfo, 2> submit_infos{
            // blocking batch submit
            vk::SubmitInfo{
                .waitSemaphoreCount = 0,

                .commandBufferCount = 1,
                .pCommandBuffers = &blocking_cmd,

                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &batch.blocking_ready_semaphore,
            },

            // deferred batch submit
            vk::SubmitInfo{
                .waitSemaphoreCount = 0,

                .commandBufferCount = 1,
                .pCommandBuffers = &deferred_cmd,

                .signalSemaphoreCount = 0,
            },
        };

        device.submit(submit_infos, batch.ready_fence.get_fence(), true);
        current_frame_index = max_frames_in_flight;

        return batch.blocking_ready_semaphore;
    }

    void asset_streamer::reset_batch(uint32_t frame_index) noexcept {
        // assert(current_frame_index == max_frames_in_flight); // assume batches can't be reset without a submit
        current_frame_index = frame_index;

        // cleanup old batch
        frame_buffer& batch = batch_buffers[current_frame_index];

        assert(batch.ready_fence.status() == vk::Result::eSuccess && "Stream batch reset before being finished");

        for (auto& stream : batch.blocking.buffer_infos) {
            vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
        }
        batch.blocking.buffer_infos.clear();
        
        for (auto& stream : batch.blocking.image_infos) {
            vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
        }
        batch.blocking.image_infos.clear();

        for (auto& stream : batch.deferred.buffer_infos) {
            vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
        }
        batch.deferred.buffer_infos.clear();
        
        for (auto& stream : batch.deferred.image_infos) {
            vmaDestroyBuffer(device.get_allocator(), stream.staging_buf, stream.staging_alloc);
        }
        batch.deferred.image_infos.clear();

        batch_cmd_buffer.reset_batch(frame_index);

        batch.ready_fence.reset();
    }

    multi_fence_view asset_streamer::stream(const buffer_stream_info& stream, bool is_deferred) noexcept {
        frame_buffer& batch = batch_buffers[current_frame_index];
        assert(current_frame_index != max_frames_in_flight && "Tried to stream to a buffer which is already submited!");
        
        if (is_deferred) {
            batch.deferred.buffer_infos.emplace_back(stream);
        } else {
            batch.blocking.buffer_infos.emplace_back(stream);
        }

        return batch.ready_fence.view();
    }

    multi_fence_view asset_streamer::stream(const image_stream_info& stream, bool is_deferred) noexcept {
        frame_buffer& batch = batch_buffers[current_frame_index];
        assert(current_frame_index != max_frames_in_flight && "Tried to stream to a buffer which is already submited!");
        
        if (is_deferred) {
            batch.deferred.image_infos.emplace_back(stream);
        } else {
            batch.blocking.image_infos.emplace_back(stream);
        }

        return batch.ready_fence.view();
    }

    void asset_streamer::record_streams(vk::CommandBuffer cmd, const frame_buffer::streams_buffer& streams) noexcept {
        // perform buffer transfers

        for (uint32_t i = 0; i < streams.buffer_infos.size(); i++) {
            cmd.copyBuffer(streams.buffer_infos[i].staging_buf, streams.buffer_infos[i].buf, streams.buffer_infos[i].transfer_region);
        }

        // transition images to dst optimal

        std::vector<vk::ImageMemoryBarrier2> image_transitions; 
        image_transitions.resize(streams.image_infos.size());

        for (uint32_t i = 0; i < streams.image_infos.size(); i++) {
            image_transitions[i] = vk::ImageMemoryBarrier2{
                .srcStageMask = {},
                .srcAccessMask = {},
                .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .oldLayout = vk::ImageLayout::eUndefined, // no need to preserve data
                .newLayout = vk::ImageLayout::eTransferDstOptimal,
                .image = streams.image_infos[i].image,
                .subresourceRange{
                    .aspectMask = streams.image_infos[i].image_subresource.aspectMask,
                    .baseMipLevel = streams.image_infos[i].image_subresource.mipLevel,
                    .levelCount = 1,
                    .baseArrayLayer = streams.image_infos[i].image_subresource.layerCount,
                    .layerCount = streams.image_infos[i].image_subresource.layerCount,
                }
            };
        }

        vk::DependencyInfo in_dep{
            .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            .imageMemoryBarrierCount = image_transitions.size(),
            .pImageMemoryBarriers = image_transitions.data(),
        };

        cmd.pipelineBarrier2(in_dep);

        // perform staging to dst copy

        for (uint32_t i = 0; i < streams.image_infos.size(); i++) {
            cmd.copyBufferToImage(streams.image_infos[i].staging_buf, streams.image_infos[i].image, vk::ImageLayout::eTransferDstOptimal, streams.image_infos[i].transfer_region);
        }

        // transition to normal layout and transfer ownership

        for (uint32_t i = 0; i < streams.image_infos.size(); i++) {
            image_transitions[i] = vk::ImageMemoryBarrier2{
                .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .dstStageMask = {}, // vk::PipelineStageFlagBits2::eBottomOfPipe,
                .dstAccessMask = {}, // vk::AccessFlagBits2::eMemoryRead,
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = streams.image_infos[i].normal_layout,
                .image = streams.image_infos[i].image,
                .subresourceRange{
                    .aspectMask = streams.image_infos[i].image_subresource.aspectMask,
                    .baseMipLevel = streams.image_infos[i].image_subresource.mipLevel,
                    .levelCount = 1,
                    .baseArrayLayer = streams.image_infos[i].image_subresource.layerCount,
                    .layerCount = streams.image_infos[i].image_subresource.layerCount,
                },
            };  
        }

        vk::DependencyInfo out_dep{
            .dependencyFlags = vk::DependencyFlagBits::eByRegion,
            .imageMemoryBarrierCount = image_transitions.size(),
            .pImageMemoryBarriers = image_transitions.data(),
        };

        cmd.pipelineBarrier2(out_dep); 
    }
}