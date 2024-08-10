#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>

namespace photon {
    class camera_state {
    public:
        camera_state() noexcept = default;
        ~camera_state() noexcept = default;

        // sets up and recalculates the projection matrix with parameters which are not expected to change often
        void configure_camera(float fov, float aspect, float near_plane, float far_plane) noexcept {
            projection_mat = glm::perspective(glm::radians(fov), aspect, near_plane, far_plane);
        }

        // returns the finished View Projection matrix, note: up_dir should have inverted y axis to align with Vulkan coord space
        glm::f32mat4x4 view_project(glm::f32vec3 pos, glm::f32vec3 dir, glm::f32vec3 up_dir = glm::f32vec3(0.f, -1.f, 0.f)) noexcept {
            assert(projection_mat != glm::identity<glm::f32mat4x4>() && "Tried to use camera without calling configure_camera first!");

            return projection_mat * glm::lookAt(pos, pos + dir, up_dir) /*view_matrix*/;
        }

    private:
        glm::f32mat4x4 projection_mat = {};
    };
}