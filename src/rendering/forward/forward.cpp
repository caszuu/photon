#include "forward.hpp"

namespace photon::rendering {
    forward_renderer::forward_renderer(vulkan_device& device, vulkan_display& display, batch_buffer& shared_batch_buffer, uint32_t max_frames_in_flight) noexcept :
        device{device},
        display{display},
        batcher{shared_batch_buffer},
        max_frames_in_flight{max_frames_in_flight}
    {
        // create depth buffers

        depth_images.reserve(max_frames_in_flight);
        depth_views.reserve(max_frames_in_flight);

        vk::Format depth_format = vk::Format::eD32Sfloat;
        vk::Extent2D display_extent = display.get_display_extent();

        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            vk::ImageCreateInfo image_info{
                .imageType = vk::ImageType::e2D,
                .format = depth_format,
                .extent = { display_extent.width, display_extent.height, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = vk::SampleCountFlagBits::e1,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                .sharingMode = vk::SharingMode::eExclusive,
                .initialLayout = vk::ImageLayout::eUndefined,
            };

            VmaAllocationCreateInfo alloc_info{
                // .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
            };

            VkImage image;
            VmaAllocation alloc;

            VkResult res = vmaCreateImage(device.get_allocator(), &static_cast<VkImageCreateInfo&>(image_info), &alloc_info, &image, &alloc, nullptr);
            vk::resultCheck(static_cast<vk::Result>(res), "vmaCreateImage");

            vk::ImageViewCreateInfo view_info{
                .image = image,
                .viewType = vk::ImageViewType::e2D,
                .format = depth_format,
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eDepth, // | vk::ImageAspectFlagBits::eStencil
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            vk::ImageView view = device.get_device().createImageView(view_info);

            depth_images.emplace_back(image, alloc);
            depth_views.emplace_back(view);
        }
    }

    forward_renderer::~forward_renderer() noexcept {
        for (auto view : depth_views) {
            device.get_device().destroyImageView(view);
        }
        
        for (auto& image : depth_images) {
            vmaDestroyImage(device.get_allocator(), image.first, image.second);
        }
    }
}