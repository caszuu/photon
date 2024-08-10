#pragma once

#include "camera.hpp"
#include <glm/glm.hpp>

namespace photon {
    class photon_app;

    class player_state {
    public:
        player_state(photon_app& app_state) noexcept;
        ~player_state() noexcept;

        void tick() noexcept;

    private:
        photon_app& app_state;

        glm::f32vec3 player_pos;
        glm::f32vec3 player_dir;

        camera_state player_camera;
    };
}