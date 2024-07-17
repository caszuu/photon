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
        
    }
}