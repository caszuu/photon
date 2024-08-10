#include "transform_buffers.hpp"
#include <cstring>

namespace photon::rendering {
    transform_buffers::transform_buffers(vulkan_device& device, uint32_t max_frames_in_flight) :
        device{device},
        id_alloc{},
        buffer_instance_count{1024 /*temp.*/},
        max_frames_in_flight{max_frames_in_flight}
    {
        instance_stride = sizeof(glm::f32mat4x4); // FIXME: min aligment size
        id_alloc.extend(buffer_instance_count);

        device_buffers.resize(max_frames_in_flight);
        device_mapped_data.resize(max_frames_in_flight);

        vk::BufferCreateInfo buffer_info{
            .size = instance_stride * buffer_instance_count,
            .usage = vk::BufferUsageFlagBits::eUniformBuffer, // | vk::BufferUsageFlagBits::eStorageBuffer,
            .sharingMode = vk::SharingMode::eExclusive, // main queue usage only
        };

        VmaAllocationCreateInfo alloc_cinfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            VmaAllocationInfo alloc_info;
            VkBuffer buf;

            VkResult res = vmaCreateBuffer(device.get_allocator(), &static_cast<VkBufferCreateInfo&>(buffer_info), &alloc_cinfo, &buf, &device_buffers[i].second, &alloc_info);
            vk::resultCheck(static_cast<vk::Result>(res), "vmaCreateBuffer");

            device_mapped_data[i] = alloc_info.pMappedData;
            device_buffers[i].first = buf;
        }
    }

    transform_buffers::~transform_buffers() noexcept {
        for (auto& buf : device_buffers) {
            vmaDestroyBuffer(device.get_allocator(), buf.first, buf.second);
        }
    }

    void transform_buffers::write_out(uint32_t frame_index) noexcept {
        for (uint32_t i = updates_in_progress_buf.size(); i-- > 0; ) {
            transform_update& update = updates_in_progress_buf[i];

            std::memcpy(device_mapped_data[frame_index] + update.id * instance_stride, &update.data, sizeof(glm::f32mat4x4));

            if (--update.frames_to_write == 0) {
                updates_in_progress_buf[i] = std::move(updates_in_progress_buf.back());
                updates_in_progress_buf.pop_back();

                updates_in_progress_map[updates_in_progress_buf[i].id] = i;
                updates_in_progress_map.erase(update.id);
            }
        }
    }

    void transform_buffers::update_transform(transform_id id, glm::f32mat4x4 data) noexcept {
        auto iter = updates_in_progress_map.find(id);

        if (iter != updates_in_progress_map.end()) {
            // refresh existing in-progress update
            updates_in_progress_buf[iter->second].frames_to_write = max_frames_in_flight;
        } else {
            updates_in_progress_map.emplace(id, updates_in_progress_buf.size());
            updates_in_progress_buf.emplace_back(data, id, max_frames_in_flight);
        }
    }
}