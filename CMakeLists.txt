set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
set(VCPKG_MANIFEST_MODE ON)

cmake_minimum_required(VERSION 4.2)

project(ROSE VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Editor" CACHE STRING "" FORCE)

if(MSVC)
    add_compile_options(/utf-8)
endif()

file(GLOB_RECURSE ROSE_CORE_SOURCES "Engine/Core/*.cpp" "Core/*.c" "Core/*.h")
add_library(Core STATIC ${ROSE_CORE_SOURCES})

set_target_properties(Core PROPERTIES
    DEBUG_POSTFIX "d"
)

set_target_properties(Core PROPERTIES
    EDITOR_POSTFIX "e"
)

find_package(SDL3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)
#find_package(fmt REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_image REQUIRED)

target_include_directories(Core PUBLIC include/)

set_property(TARGET Core PROPERTY FOLDER "Engine")

target_link_libraries(Core
    PRIVATE SDL3::SDL3
    PRIVATE imgui::imgui
    PRIVATE glad::glad
    #PUBLIC  fmt::fmt
)

if (CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    target_compile_options(Core PRIVATE -mavx2)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(Core PRIVATE /arch:AVX2)
endif()


# add_executable(Example1 "examples/Ex1/main.cpp")
# set_property(TARGET Example1 PROPERTY FOLDER "Examples")
# target_link_libraries(Example1 PRIVATE Core SDL3_ttf::SDL3_ttf)

add_executable(ControllerTest "examples/ControllerTest/main.cpp")
set_property(TARGET ControllerTest PROPERTY FOLDER "Examples")
target_link_libraries(ControllerTest PRIVATE
        Core
        SDL3_ttf::SDL3_ttf
        SDL3_image::SDL3_image
)

file(GLOB_RECURSE ROSE_EDITOR_SOURCES "Engine/Editor/*.cpp" "Engine/Editor/*.c" "Engine/Editor/*.h")
add_executable(Editor ${ROSE_EDITOR_SOURCES})
target_include_directories(Editor PUBLIC include/)
set_property(TARGET Editor PROPERTY FOLDER "Engine")

add_custom_target(ENGINE_BUILD)
add_dependencies(ENGINE_BUILD Core Editor)

target_compile_definitions(Core PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:_NDEBUG>
    $<$<CONFIG:Editor>:_NDEBUG>
)

target_compile_definitions(Core
    PUBLIC  $<$<CONFIG:Editor>:ROSE_EDITOR>
	PRIVATE $<$<CONFIG:Editor>:_NDEBUG>
)

target_compile_definitions(Editor
    PUBLIC  $<$<CONFIG:Editor>:ROSE_EDITOR>
    PRIVATE $<$<CONFIG:Editor>:_NDEBUG>
)

