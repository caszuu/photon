#pragma once

#include <rendering/vk_device.hpp>
#include "streamer.hpp"

#include <string>

namespace photon {
    class texture {
    public:
        texture(rendering::asset_streamer& streamer) noexcept : streamer{streamer}, device{streamer.get_device()} { }
        ~texture() noexcept;

        // allocation
        void create(const vk::ImageCreateInfo& image_info, vk::ImageLayout normal_layout, const VmaAllocationCreateInfo& alloc_info, vk::ImageViewCreateInfo& view_info);
        void destroy() noexcept;

        // data streaming (staging)
        void stream(void* data, VkDeviceSize data_size, vk::ImageSubresourceLayers subresource, bool is_deferred);

        vk::Image get_image() const noexcept { return image; }
        vk::ImageView get_image_view() const noexcept { return image_view; }
        vk::ImageLayout get_normal_layout() const noexcept { return image_normal_layout; }

        vk::Format get_format() const noexcept { return image_format; } // note: will return the format of the image, image_view format might differ
        vk::Extent3D get_extent() const noexcept { return image_extent; } // note: for 1D and 2D images it's guaranteed that unused dimensions are equal to 1

        rendering::multi_fence_view get_ready_fence() noexcept { return ready_fence; }

        // uses stb_image to load and stage a texture to gpu
        static texture load_file(rendering::asset_streamer& streamer, const std::string_view path) noexcept;

    private:
        vk::Image image;
        vk::ImageView image_view;
        VmaAllocation image_alloc = VK_NULL_HANDLE;

        // stores the layout of the image when not in use
        vk::ImageLayout image_normal_layout;

        rendering::multi_fence_view ready_fence;

        rendering::asset_streamer& streamer;
        rendering::vulkan_device& device;

        vk::Format image_format;
        vk::Extent3D image_extent;
    };
}