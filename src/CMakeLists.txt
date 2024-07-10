add_executable(photon-app
        core/entry_point.cpp
        
        core/app.cpp
        core/window.cpp
        
        rendering/rendering_stack.cpp
        rendering/vk_instance.cpp
        rendering/vk_device.cpp
        rendering/vk_display.cpp
        rendering/batch_buffer.cpp)

target_compile_features(photon-app PRIVATE cxx_std_20)
target_compile_definitions(photon-app PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC)
target_compile_definitions(photon-app PRIVATE VULKAN_HPP_NO_CONSTRUCTORS)

target_include_directories(photon-app PRIVATE .)

target_link_libraries(photon-app PRIVATE SDL3::SDL3)