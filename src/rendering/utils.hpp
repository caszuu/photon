#pragma once

#include <vector>
#include <optional>

#include <cassert>

namespace photon::rendering {
    // a small helper class for "allocating" indexes from a buffer using the "pool" allocator method
    
    template<typename index_t>
    class pool_index_alloc {
    public:
        pool_index_alloc() noexcept : pool_size{0} { }
        pool_index_alloc(index_t initial_size) noexcept : pool_size{initial_size} { }
        ~pool_index_alloc() noexcept = default;

        std::optional<index_t> alloc() noexcept {
            if (!free_indices.empty()) {
                index_t i = free_indices.back();
                free_indices.pop_back();

                return i;
            }

            index_t i = pool_head++;

            return i == pool_size ? std::optional<index_t>((pool_head--, std::nullopt)) : std::optional<index_t>(i);
        }
        
        void dealloc(index_t index) noexcept {
            if (index + 1 == pool_head) {
                pool_head--;
                return;
            }

            free_indices.emplace_back(index);
        }

        void reset() noexcept {
            pool_head = 0;
            free_indices.clear();
        }

        void extend(index_t new_size) noexcept {
            assert(new_size > pool_size);
            pool_size = new_size;
        }

    private:
        std::vector<index_t> free_indices;
        index_t pool_head = 0;

        index_t pool_size;
    };
}