#pragma once

#include <core/window.hpp>

namespace photon {
    class PhotonApp {
    public:
        PhotonApp() noexcept;
        ~PhotonApp() noexcept;
    
        void launch() noexcept;

    private:
        Window app_window;
    };
}