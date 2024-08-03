#include "forward.hpp"

#include <cassert>

namespace photon::rendering {
    forward_renderer::forward_renderer(vulkan_device& device, vulkan_display& display, batch_buffer& shared_batch_buffer, uint32_t max_frames_in_flight) noexcept :
        device{device},
        display{display},
        batcher{shared_batch_buffer},
        max_frames_in_flight{max_frames_in_flight}
    {
        // create depth buffers

        depth_images.resize(max_frames_in_flight);
        depth_views.resize(max_frames_in_flight);

        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            create_frame_resources(i);
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

    void forward_renderer::frame(const frame_context& ctx) {
        // begin cmd and dynamic rendering
        
        vk::CommandBuffer cmd;

        {
            vk::CommandBufferBeginInfo begin_info{
                .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
            };

            cmd = batcher.begin_recording(begin_info);
        }

        {
            // transition frame resources

            std::array<vk::ImageMemoryBarrier2, 2> resource_barriers;

            resource_barriers[0] = vk::ImageMemoryBarrier2{ /* color (swapchain) image */
                .srcStageMask = {},
                .srcAccessMask = {},
                .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite, // assume no shader reads
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .image = ctx.active_swapchain->images[ctx.swapchain_image_index],
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                }
            };

            resource_barriers[1] = vk::ImageMemoryBarrier2{ /* depth image */
                .srcStageMask = {},
                .srcAccessMask = {},
                .dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite, // assume no shader reads
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                .image = depth_images[ctx.frame_index].first,
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eDepth, // | vk::ImageAspectFlagBits::eStencil,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                }
            };

            vk::DependencyInfo dep_info{
                .dependencyFlags = vk::DependencyFlagBits::eByRegion, // eViewLocal?
                .imageMemoryBarrierCount = resource_barriers.size(),
                .pImageMemoryBarriers = resource_barriers.data(),
            };

            cmd.pipelineBarrier2(dep_info);

            // start dynamic rendering

            vk::RenderingAttachmentInfo color_info{
                .imageView = ctx.active_swapchain->image_views[ctx.swapchain_image_index],
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = {
                    .color = { .float32 = std::array<float, 4>{.1f, .1f, .1f, 1.f} }
                }
            };

            vk::RenderingAttachmentInfo depth_info{
                .imageView = depth_views[ctx.frame_index],
                .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eDontCare,
                .clearValue = {
                    .depthStencil = { .depth = 1.f, .stencil = 0 }
                }
            };

            vk::RenderingInfo rendering_info{
                // .flags = vk::RenderingFlagBits::eSuspending,
                .renderArea = { .extent = display.get_display_extent() },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_info,
                .pDepthAttachment = &depth_info,
                // .pStencilAttachment = &depth_info,
            };

            cmd.beginRendering(rendering_info);
        }

        // draw photon scene
        // TODO

        /* for (auto& draw : scene.drawables) {
            
        } */

        // finish recording and submit

        cmd.endRendering();

        {
            // transition render outputs to desired layouts
            // TODO: replace with photon_buffer_layout (externaly managed buffer transitions and usage)

            std::array<vk::ImageMemoryBarrier2, 1> resource_barriers;

            resource_barriers[0] = vk::ImageMemoryBarrier2{ /* color (swapchain) image */
                .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
                .dstStageMask = {},
                .dstAccessMask = {},
                .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .newLayout = vk::ImageLayout::ePresentSrcKHR,
                .image = ctx.active_swapchain->images[ctx.swapchain_image_index],
                .subresourceRange{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                }
            };

            // note: depth buffer is not being stored so no transition

            vk::DependencyInfo dep_info{
                .dependencyFlags = vk::DependencyFlagBits::eByRegion, // eViewLocal?
                .imageMemoryBarrierCount = resource_barriers.size(),
                .pImageMemoryBarriers = resource_barriers.data(),
            };

            cmd.pipelineBarrier2(dep_info);
        }

        cmd.end();

        ctx.cmds.emplace_back(cmd);
    }

    void forward_renderer::refresh(uint32_t frame_index) {
        // destroy old frame resources

        device.get_device().destroyImageView(depth_views[frame_index]);
        vmaDestroyImage(device.get_allocator(), depth_images[frame_index].first, depth_images[frame_index].second);

        // create new resources
        create_frame_resources(frame_index);
    }

    void forward_renderer::create_frame_resources(uint32_t frame_index) {
        // depth images

        vk::Format depth_format = vk::Format::eD32Sfloat; // TODO: depth format selection
        vk::Extent2D display_extent = display.get_display_extent();

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

        depth_images[frame_index] = std::make_pair(image, alloc);
        depth_views[frame_index] = view;
    }
}