#include "rendering_stack.hpp"

#include <core/abort.hpp>
#include <core/logger.hpp>
#include <limits>

namespace photon::rendering {
    inline static vulkan_instance create_vk_instance() noexcept {
        std::vector<std::pair<const char*, bool>> extensions;
        window::add_required_instance_extensions(extensions);
        
        vulkan_instance::instance_config config{
            .requested_extensions = std::move(extensions),
            .requested_layers = {},
        };

        return vulkan_instance(config);
    }

    inline static vulkan_device create_vk_device(vulkan_instance& instance) noexcept {
        std::vector<std::pair<const char*, bool>> extensions;
        extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
        
        vulkan_device::device_config config{
            .instance = instance,
            .requested_extensions = std::move(extensions),
        };

        return vulkan_device(config);
    }

    rendering_stack::rendering_stack(window& target_window, uint32_t max_frames_in_flight) noexcept :
        target_window{target_window},
        vk_instance{create_vk_instance()},
        vk_device{create_vk_device(vk_instance)},
        vk_display{vulkan_display::display_config{
            .device = vk_device,
            .target_surface = target_window.create_surface(vk_instance),
            .initial_swapchain_config{
                .preferred_mode = vk::PresentModeKHR::eFifo,
            },
            .initial_swapchain_extent = {target_window.get_extent().first, target_window.get_extent().second},
            .min_swapchain_image_count = 3,
        }},
        shared_batch_buffer{vk_device, max_frames_in_flight, false},
        streamer{vk_device, max_frames_in_flight},
        renderer{vk_device, vk_display, shared_batch_buffer, max_frames_in_flight},
        max_frames_in_flight{max_frames_in_flight}
    {
        {
            // init sync objects

            vk::FenceCreateInfo fence_info{
                .flags = vk::FenceCreateFlagBits::eSignaled,
            };

            vk::SemaphoreCreateInfo sem_info{};

            frame_fences.reserve(max_frames_in_flight);
            frame_acquire_sems.reserve(max_frames_in_flight);
            frame_ready_sems.reserve(max_frames_in_flight);

            for (uint32_t i = 0; i < max_frames_in_flight; i++) {
                frame_fences.emplace_back(vk_device.get_device().createFence(fence_info));
                frame_acquire_sems.emplace_back(vk_device.get_device().createSemaphore(sem_info));
                frame_ready_sems.emplace_back(vk_device.get_device().createSemaphore(sem_info));
            }
        }

        frame_swapchains.resize(max_frames_in_flight);
        frame_invalidated.resize(max_frames_in_flight, false);
    }

    rendering_stack::~rendering_stack() noexcept {
        // no resource must be in use when cleaning up
        vk_device.get_device().waitIdle();

        for (auto fence : frame_fences) {
            vk_device.get_device().destroyFence(fence);
        }

        for (auto sem : frame_acquire_sems) {
            vk_device.get_device().destroySemaphore(sem);
        }

        for (auto sem : frame_ready_sems) {
            vk_device.get_device().destroySemaphore(sem);
        }
    }

    void rendering_stack::frame() noexcept {
        try {
            // acquire swapchain frame
    
            uint32_t acq_image_index;
                
            vk::Result res = vk_device.get_device().acquireNextImageKHR(vk_display.get_swapchain()->swapchain, std::numeric_limits<uint64_t>::max(), frame_acquire_sems[current_frame_index], {}, &acq_image_index);
            if (res == vk::Result::eErrorOutOfDateKHR) {
                vk_display.refresh_swapchain(std::nullopt);
                for (uint32_t i = 0; i < max_frames_in_flight; i++) frame_invalidated[i] = true;

                return;
            } else if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
                P_LOG_E("Failed to acquire a vulkan image: {}", static_cast<int32_t>(res));
                engine_abort();
            }

            // wait for in flight frame and reset

            res = vk_device.get_device().waitForFences(frame_fences[current_frame_index], vk::True, std::numeric_limits<uint64_t>::max());
            vk::resultCheck(res, "waitForFences");
    
            vk_device.get_device().resetFences(frame_fences[current_frame_index]);
    
            shared_batch_buffer.reset_batch(current_frame_index);
            vk::Semaphore streamer_finished_sem = streamer.submit_batch((current_frame_index + 1) % max_frames_in_flight);
            
            current_image_index = acq_image_index;
            frame_swapchains[current_frame_index] = vk_display.get_swapchain();

            if (frame_invalidated[current_frame_index]) {
                renderer.refresh(current_frame_index);

                frame_invalidated[current_frame_index] = false;
            }
    
            // record frame cmds
    
            std::vector<vk::CommandBuffer> cmds;
    
            frame_context frame_ctx{
                .cmds = cmds,
                .active_swapchain = frame_swapchains[current_frame_index],
                .frame_index = current_frame_index,
            };

            renderer.frame(frame_ctx);
    
            // post_processing_pass.record_frame(cmds);
    
            // imgui_pass.record_frame(cmds);
    
            // submit frame cmds
    
            vk::Semaphore submit_wait_sems[] = { streamer_finished_sem, frame_acquire_sems[current_frame_index] };
            vk::PipelineStageFlags submit_wait_stages[] = { vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader };
    
            vk::SubmitInfo submit_info{
                .waitSemaphoreCount = 2,
                .pWaitSemaphores = submit_wait_sems,
                .pWaitDstStageMask = submit_wait_stages,
                .commandBufferCount = cmds.size(),
                .pCommandBuffers = cmds.data(),
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &frame_ready_sems[current_frame_index],
            };
    
            shared_batch_buffer.submit_batch(std::span(&submit_info, 1), frame_fences[current_frame_index]);
    
            // present frame
    
            vk::PresentInfoKHR present_info{
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &frame_ready_sems[current_frame_index],
                .swapchainCount = 1,
                .pSwapchains = &frame_swapchains[current_frame_index]->swapchain,
                .pImageIndices = &current_image_index,
            };
    
            res = vk_device.get_queue(false).presentKHR(&present_info);
    
            if (res == vk::Result::eSuboptimalKHR | res == vk::Result::eErrorOutOfDateKHR) {
                vk_display.refresh_swapchain(std::nullopt);
                for (uint32_t i = 0; i < max_frames_in_flight; i++) frame_invalidated[i] = true;
            } else if (res != vk::Result::eSuccess) {
                P_LOG_E("Failed to present a vulkan image: {}", static_cast<int32_t>(res));
                engine_abort();
            }
        } catch (std::exception& e) {
            P_LOG_E("Error while rendering a frame in rendering_stack: {}", e.what());
            engine_abort();
        }

        // flip to next frame

        current_frame_index = (current_frame_index + 1) % max_frames_in_flight;
    }
}