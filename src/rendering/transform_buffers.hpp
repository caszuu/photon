#pragma once

#include "vk_device.hpp"
#include "utils.hpp"

#include <core/logger.hpp>
#include <core/abort.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <map>
#include <optional>
#include <cassert>

namespace photon::rendering {
    using transform_id = uint32_t;
    
    class transform_buffers {
    public:
        transform_buffers(vulkan_device& device, uint32_t max_frames_in_flight);
        ~transform_buffers() noexcept;

        void write_out(uint32_t frame_index) noexcept;

        transform_id create_transform(glm::f32mat4x4 initial_data) noexcept {
            std::optional<transform_id> id = id_alloc.alloc();

            if (id) {
                update_transform(id.value(), initial_data);
                return id.value();
            } else {
                P_LOG_E("Ran out of device transform instances in transform_buffers!");
                engine_abort();
            }
        }

        void destroy_transform(transform_id id) noexcept {
            auto iter = updates_in_progress_map.find(id);
            if (iter != updates_in_progress_map.end()) {
                updates_in_progress_buf[iter->second] = std::move(updates_in_progress_buf.back());
                updates_in_progress_buf.pop_back();

                updates_in_progress_map[updates_in_progress_buf[iter->second].id] = iter->second;
                updates_in_progress_map.erase(iter);
            }

            id_alloc.dealloc(id);
        }

        void update_transform(transform_id id, glm::f32mat4x4 initial_data) noexcept;
    private:
        vulkan_device& device;

        std::vector<std::pair<vk::Buffer, VmaAllocation>> device_buffers;
        std::vector<void*> device_mapped_data;

        struct transform_update {
            glm::f32mat4x4 data;
            transform_id id;
            uint32_t frames_to_write;
        };

        // holds updates which weren't written for *all* frames yet
        std::vector<transform_update> updates_in_progress_buf;
        std::map<transform_id, uint32_t /*[update_in_progress] index for that id*/> updates_in_progress_map;

        pool_index_alloc<transform_id> id_alloc;

        uint32_t buffer_instance_count;
        uint32_t instance_stride;

        uint32_t max_frames_in_flight;
    };
}