#pragma once

#include "logger.hpp"

#include <mutex>
#include <thread>
#include <iostream>

namespace photon {
    [[noreturn]] inline void engine_abort() noexcept {
        static std::mutex abort_mutex;
        std::lock_guard<std::mutex> l(abort_mutex);

        P_LOG_E("An unrecoverable error occured! Aborting...");

        std::cout.flush();
        std::exit(255);
    }
}