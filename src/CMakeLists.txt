add_executable(photon-app
        core/entry_point.cpp
        
        core/app.cpp
        core/window.cpp)

target_compile_features(photon-app PRIVATE cxx_std_20)

target_compile_definitions(photon-app PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)
target_compile_definitions(photon-app PRIVATE VULKAN_HPP_NO_EXCEPTIONS)
target_compile_definitions(photon-app PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)

target_include_directories(photon-app PRIVATE .)

target_link_libraries(photon-app PRIVATE SDL3::SDL3)
target_link_libraries(photon-app PRIVATE spdlog::spdlog)