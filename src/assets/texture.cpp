#include "texture.hpp"
#include <core/abort.hpp>
#include <core/logger.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace photon {
    texture::~texture() noexcept {
        if (image) destroy();
    }

    void texture::create(const vk::ImageCreateInfo& image_info, vk::ImageLayout normal_layout, const VmaAllocationCreateInfo& alloc_info, vk::ImageViewCreateInfo& view_info) {
        if (image) {
            P_LOG_E("Failed to create() a texture: already allocated!");
            engine_abort();
        }

        image_normal_layout = normal_layout,
        image_format = image_info.format;
        image_extent = image_info.extent;

        if (image_info.sharingMode != vk::SharingMode::eConcurrent) {
            P_LOG_E("Photon streaming resources only support Vulkan concurrent sharing mode!");
            engine_abort();
        }

        if (image_info.imageType == vk::ImageType::e2D) {
            image_extent.depth = 1;
        } else if (image_info.imageType == vk::ImageType::e1D) {
            image_extent.height = 1;
            image_extent.depth = 1;
        }

        VkImage img;
        VkResult res = vmaCreateImage(device.get_allocator(), &static_cast<const VkImageCreateInfo&>(image_info), &alloc_info, &img, &image_alloc, nullptr);
        vk::resultCheck(static_cast<vk::Result>(res), "vmaCreateImage");

        image = img;
        view_info.image = img;
        image_view = device.get_device().createImageView(view_info);
    }

    void texture::destroy() noexcept {
        if (!image) {
            P_LOG_E("Failed to destroy() a texture: texture not allocated! (double destroy?)");
            engine_abort();
        }

        device.get_device().destroyImageView(image_view);
        vmaDestroyImage(device.get_allocator(), static_cast<VkImage>(image), image_alloc);

        ready_fence = rendering::multi_fence_view();

        image_normal_layout = vk::ImageLayout::eUndefined;
        image_format = {};
        image_extent = vk::Extent3D{ 0, 0, 0 };
    }

    void texture::stream(void* data, VkDeviceSize data_size, vk::ImageSubresourceLayers subresource, bool is_deferred) {
        size_t image_size = 4 * image_extent.width * image_extent.height * image_extent.depth;

        if (image_size != data_size) {
            P_LOG_E("Unexpected texture size while streaming a texture! (expected: {} received: {})", image_size, data_size);
            engine_abort();
        }

        // alloc staging buf

        vk::BufferCreateInfo staging_info{
            .size = image_size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
        };

        VmaAllocationCreateInfo alloc_create_info{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        VkBuffer buf;
        VmaAllocation alloc;
        VmaAllocationInfo alloc_info;

        VkResult res = vmaCreateBuffer(device.get_allocator(), &static_cast<VkBufferCreateInfo&>(staging_info), &alloc_create_info, &buf, &alloc, &alloc_info);
        vk::resultCheck(static_cast<vk::Result>(res), "vmaCreateBuffer");

        std::memcpy(alloc_info.pMappedData, data, image_size);

        // submit to streamer

        rendering::asset_streamer::image_stream_info info{
            .staging_buf = buf,
            .staging_alloc = alloc,
            
            .image = image,
            .normal_layout = image_normal_layout,
            .transfer_region = {
                .imageSubresource = subresource,
                .imageExtent = image_extent,
            },
            .image_subresource = subresource,
        };

        ready_fence = streamer.stream(info, is_deferred);
    }

    texture texture::load_file(rendering::asset_streamer& streamer, const std::string_view path) noexcept {
        int x, y, comp;
        uint8_t* data = stbi_load(path.data(), &x, &y, &comp, 4 /*reformat to rgba*/);

        if (!data) {
            P_LOG_E("Failed to load texture: {}", path);
            engine_abort();
        }

        /* if (comp != 4) {
            P_LOG_E("Loaded texture is not a RBGA image! {}", comp);
            engine_abort();
        } */

        vk::ImageCreateInfo image_info{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Srgb,
            .extent = { static_cast<uint32_t>(x), static_cast<uint32_t>(y), 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eLinear,
            .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eConcurrent,
            .initialLayout = vk::ImageLayout::eUndefined,
        };

        VmaAllocationCreateInfo alloc_info{
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        vk::ImageViewCreateInfo view_info{
            .viewType = vk::ImageViewType::e2D,
            .format = image_info.format,
            .components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
            .subresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        texture tex(streamer);
        tex.create(image_info, vk::ImageLayout::eShaderReadOnlyOptimal, alloc_info, view_info);

        tex.stream(data, 4 * x * y, {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }, false);

        stbi_image_free(data);

        return tex;
    }
}