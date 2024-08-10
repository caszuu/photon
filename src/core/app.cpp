#include "app.hpp"

#include <chrono>

namespace photon {
    photon_app::photon_app() noexcept :
        app_window{ { 1280, 720 } },
        rstack{app_window, 3},
        player{*this}
    {
        
    }

    photon_app::~photon_app() noexcept {

    }

    void photon_app::launch() noexcept {
        std::chrono::time_point t = std::chrono::steady_clock::now();

        app_window.set_mouse_mode(true);

        while (!app_window.should_close()) {
            app_window.poll_events();

            float delta_time = std::chrono::duration<float>(std::chrono::steady_clock::now() - t).count();
            t = std::chrono::steady_clock::now();

            player.tick();

            rstack.frame();
        }
    }
}