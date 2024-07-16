#pragma once

#include "vk_device.hpp"

#include <memory>
#include <limits>

namespace photon::rendering {
    // a small helper wrapper around vk::Fence which allows for multiple observants (views)
    // of the same fence while allowing it to be reset without causing the ABA race condition

    // it simply holds a intenal counter so it could be also described as a timeline_fence similar to
    // Vulkan timeline semaphores

    class multi_fence_view;

    class multi_fence {
    public:
        multi_fence(vulkan_device& device, const vk::FenceCreateInfo& fence_info) {
            state = std::make_shared<fence_state>(device.get_device().createFence(fence_info), device);
        }
        multi_fence() noexcept : state{nullptr} { }

        ~multi_fence() noexcept = default;

        void reset() {
            assert(status() == vk::Result::eSuccess && "Tried to reset a multi_fence without it being signaled");
            
            state->device.get_device().resetFences(state->fence);
            state->reuse_index++;
        }

        vk::Result status() const { return state->device.get_device().getFenceStatus(state->fence); }
        vk::Result wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const { return state->device.get_device().waitForFences(state->fence, vk::True, timeout); }

        multi_fence_view view() const noexcept;
        vk::Fence get_fence() noexcept { return state->fence; }

    private:
        struct fence_state {
            ~fence_state() noexcept {
                device.get_device().destroyFence(fence);
            }

            vk::Fence fence;
            vulkan_device& device;

            uint32_t reuse_index = 0;
        };

        std::shared_ptr<fence_state> state;

        friend class multi_fence_view;
    };

    class multi_fence_view {
    public:
        multi_fence_view(std::shared_ptr<multi_fence::fence_state> state) noexcept : state{state}, view_reuse_index{state->reuse_index} { }
        multi_fence_view() noexcept : state{nullptr}, view_reuse_index{0} { }
        
        ~multi_fence_view() noexcept = default;

        vk::Result status() const {
            if (state->reuse_index > view_reuse_index) return vk::Result::eSuccess;

            return state->device.get_device().getFenceStatus(state->fence);
        }

        vk::Result wait(uint64_t timeout = std::numeric_limits<uint64_t>::max()) const {
            if (state->reuse_index > view_reuse_index) return vk::Result::eSuccess;

            return state->device.get_device().waitForFences(state->fence, vk::True, timeout);
        }

    private:
        std::shared_ptr<multi_fence::fence_state> state;
        uint32_t view_reuse_index;
    };

    inline multi_fence_view multi_fence::view() const noexcept { 
        return multi_fence_view(state);
    }
}