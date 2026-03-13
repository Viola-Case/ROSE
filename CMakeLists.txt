set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
set(VCPKG_MANIFEST_MODE ON)

cmake_minimum_required(VERSION 4.2)

project(ROSE VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/utf-8)
endif()

file(GLOB_RECURSE ROSE_SOURCES "src/*.cpp")
add_library(ROSE STATIC ${ROSE_SOURCES})

set_target_properties(ROSE PROPERTIES
    DEBUG_POSTFIX "d"
)

find_package(SDL3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)
find_package(fmt REQUIRED)
find_package(SDL3_ttf REQUIRED)

target_include_directories(ROSE PUBLIC include/)

target_link_libraries(ROSE
    PRIVATE SDL3::SDL3
    PRIVATE imgui::imgui
    PRIVATE glad::glad
    PUBLIC  fmt::fmt
)

add_executable(Example2 "examples/Ex2/Example 2 - main.cpp")
target_link_libraries(Example2 PRIVATE ROSE SDL3_ttf::SDL3_ttf)
