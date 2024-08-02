#include "app.hpp"

namespace photon {
    photon_app::photon_app() noexcept :
        app_window{ { 1280, 720 } },
        rstack{app_window, 3}
    {
        
    }

    photon_app::~photon_app() noexcept {

    }

    void photon_app::launch() noexcept {
        while (!app_window.should_close()) {
            rstack.frame();

            app_window.poll_events();
        }
    }
}