set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
set(VCPKG_MANIFEST_MODE ON)

cmake_minimum_required(VERSION 4.2)

project(ROSE_CORE VERSION 0.1)

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

file(GLOB_RECURSE ROSE_CORE_SOURCES "Core/src/*.cpp" "Core/src/*.c" "Core/src/*.h")
add_library(ROSE_CORE STATIC ${ROSE_CORE_SOURCES})

set_target_properties(ROSE_CORE PROPERTIES
    DEBUG_POSTFIX "d"
)

find_package(SDL3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)
find_package(fmt REQUIRED)
find_package(SDL3_ttf REQUIRED)

target_include_directories(ROSE_CORE PUBLIC include/)

target_link_libraries(ROSE_CORE
    PRIVATE SDL3::SDL3
    PRIVATE imgui::imgui
    PRIVATE glad::glad
    PUBLIC  fmt::fmt
)

add_executable(Example2 "examples/Ex1/main.cpp")
target_link_libraries(Example2 PRIVATE ROSE_CORE SDL3_ttf::SDL3_ttf)

add_executable(Example2 "examples/Ex2/main.cpp")
target_link_libraries(Example2 PRIVATE ROSE_CORE SDL3_ttf::SDL3_ttf)
