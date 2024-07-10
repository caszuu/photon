#pragma once

#include <core/window.hpp>
#include <rendering/rendering_stack.hpp>

namespace photon {
    class photon_app {
    public:
        photon_app() noexcept;
        ~photon_app() noexcept;
    
        void launch() noexcept;

    private:
        window app_window;
        rendering::rendering_stack rstack;
    };
}