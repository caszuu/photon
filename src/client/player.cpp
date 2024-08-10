#include "player.hpp"
#include <core/app.hpp>

#include <SDL3/SDL_mouse.h>

namespace photon {
    player_state::player_state(photon_app& app) noexcept : app_state{app} {
        player_camera.configure_camera(90.f, 16.f/9.f, .05f, 100.f);
    }

    player_state::~player_state() noexcept {

    }

    void player_state::tick() noexcept {
        float delta_time;

        // freecam math

        auto mouse_pos = app_state.get_window().get_relative_mouse_pos();
        P_LOG_I("mouse: {} {}", mouse_pos.first, mouse_pos.second);

        // app_state.get_rendering_stack().write_camera(player_camera.view_project(player_pos, player_dir));
    }
}