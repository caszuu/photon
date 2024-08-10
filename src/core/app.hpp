#pragma once

#include <core/window.hpp>
#include <rendering/rendering_stack.hpp>

#include <client/player.hpp>

namespace photon {
    class photon_app {
    public:
        photon_app() noexcept;
        ~photon_app() noexcept;
    
        void launch() noexcept;

        window& get_window() noexcept { return app_window; }
        rendering::rendering_stack& get_rendering_stack() noexcept { return rstack; }

    private:
        window app_window;
        rendering::rendering_stack rstack;

        player_state player;
    };
}