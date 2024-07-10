#pragma once

#include <iostream>
#include <string>
#include <format>

namespace photon::logging {
    constexpr static std::string_view reset_escape = "\033[0m";

    constexpr static std::string_view debug_escape = "\033[96m";
    constexpr static std::string_view info_escape = "\033[92m";
    constexpr static std::string_view warn_escape = "\033[33m";
    constexpr static std::string_view err_escape = "\033[31m";

    inline void debug(std::string formated_message) noexcept {
        std::cout << " [" << debug_escape << "debug" << reset_escape << "] " << formated_message << '\n';
    }

    inline void info(std::string formated_message) noexcept {
        std::cout << " [" << info_escape << "info" << reset_escape << "] " << formated_message << '\n';
    }

    inline void warn(std::string formated_message) noexcept {
        std::cout << " [" << warn_escape << "warning" << reset_escape << "] " << formated_message << '\n';
    }

    inline void err(std::string formated_message) noexcept {
        std::cout << " [" << err_escape << "error" << reset_escape << "] " << formated_message << '\n';
    }
}

#define P_LOG_D(fmt, ...) ::photon::logging::debug(std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#define P_LOG_I(fmt, ...) ::photon::logging::info(std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#define P_LOG_W(fmt, ...) ::photon::logging::warn(std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#define P_LOG_E(fmt, ...) ::photon::logging::err(std::format(fmt __VA_OPT__(,) __VA_ARGS__))