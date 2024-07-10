cmake_minimum_required(VERSION 3.10)

project(photon)

# include deps
add_subdirectory(ext/SDL)
add_subdirectory(ext/spdlog)

add_subdirectory(src)
target_include_directories(photon-app PRIVATE ext/Vulkan-Hpp)
