#pragma once

#include <rendering/vk_device.hpp>
#include <rendering/batch_buffer.hpp>
#include <rendering/multi_fence.hpp>

#include <variant>
#include <vector>

namespace photon::rendering {
    // TODO: the multi cmd model might serialize all transfer copy commands if some barriers exist in them (eg. image transitions)

    class asset_streamer {
    public:
        asset_streamer(rendering::vulkan_device& device, uint32_t max_frames_in_flight) noexcept;
        ~asset_streamer() noexcept;

        // submits the submit batch to the device transfer queue, the previous submit by the current batch must *not* be in flight by this point
        // the streamer will automatically reset to the batch [next_frame_index] and can be used immidiatelly after submit (even if that batch is in flight)
        // returns the semaphore used for waiting for the blocking part of the stream batch
        vk::Semaphore submit_batch(uint32_t next_frame_index);

        struct buffer_stream_info {
            vk::Buffer staging_buf; 
            VmaAllocation staging_alloc;

            vk::Buffer buf;
            vk::BufferCopy transfer_region;
        };

        struct image_stream_info {
            vk::Buffer staging_buf; 
            VmaAllocation staging_alloc;

            vk::Image image;
            vk::ImageLayout normal_layout;
            vk::BufferImageCopy transfer_region;
            vk::ImageSubresourceLayers image_subresource;
        };

        // schedule a new stream, blocks the next frame until finished if [is_deferred] is false
        // returns ready_fence which checks if the stream batch containing this stream is finished
        multi_fence_view stream(const buffer_stream_info& stream, bool is_deferred) noexcept;
        multi_fence_view stream(const image_stream_info& stream, bool is_deferred) noexcept;

        vulkan_device& get_device() noexcept { return device; }
    private:
        struct frame_buffer {
            frame_buffer(multi_fence fence, vk::Semaphore block_semaphore) noexcept : ready_fence{std::move(fence)}, blocking_ready_semaphore{block_semaphore} { }

            struct streams_buffer {
                std::vector<buffer_stream_info> buffer_infos;
                std::vector<image_stream_info> image_infos;
            };

            streams_buffer blocking;
            streams_buffer deferred;

            streams_buffer retired_blocking;
            streams_buffer retired_deferred;

            multi_fence ready_fence;
            vk::Semaphore blocking_ready_semaphore;
        };

        // returns a in-recording state cmd used for [stream_info] (must be ended before forwarding to [stream_info])
        vk::CommandBuffer begin_stream_recording() {
            vk::CommandBufferBeginInfo begin_info{
                .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
            };

            return batch_cmd_buffer.begin_recording(begin_info);
        }

        void record_streams(vk::CommandBuffer cmd, const frame_buffer::streams_buffer& streams) noexcept;

        rendering::vulkan_device& device;

        std::vector<frame_buffer> batch_buffers;
        batch_buffer batch_cmd_buffer;

        uint32_t current_frame_index = 0;
        uint32_t max_frames_in_flight;
    };
}