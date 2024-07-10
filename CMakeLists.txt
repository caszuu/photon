cmake_minimum_required(VERSION 3.10)

project(photon)
set(CMAKE_CXX_STANDARD 20)

option(ENABLE_COLORED_OUTPUT "Always enable colored output from gpu/clang compilers, mostly used with ninja" ON)

if(ENABLE_COLORED_OUTPUT)
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		add_compile_options(-fcolor-diagnostics)
	endif()
endif()

# include deps
add_subdirectory(ext/SDL)
# find_package(Vulkan REQUIRED)
add_subdirectory(ext/Vulkan-Hpp)

add_subdirectory(src)
