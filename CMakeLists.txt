cmake_minimum_required(VERSION 4.2)

option(ROSE_USE_VCPKG "Use vcpkg for dependency management" OFF)
if(ROSE_USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
        set(VCPKG_MANIFEST_MODE ON)
        file(READ "${CMAKE_SOURCE_DIR}/vcpkg.json" _vcpkg_json)
        string(JSON _vcpkg_baseline GET "${_vcpkg_json}" "builtin-baseline")
        execute_process(
            COMMAND git -C "$ENV{VCPKG_ROOT}" merge-base --is-ancestor "${_vcpkg_baseline}" HEAD
            RESULT_VARIABLE _vcpkg_baseline_missing
        )
        if(_vcpkg_baseline_missing)
            message(FATAL_ERROR
                "vcpkg HEAD is behind the required builtin-baseline '${_vcpkg_baseline}'.\n"
                "Run: git -C \"$ENV{VCPKG_ROOT}\" pull")
        endif()
    else()
        message(FATAL_ERROR "ROSE_USE_VCPKG=ON but VCPKG_ROOT environment variable is not set")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(ROSE VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Editor" CACHE STRING "" FORCE)

if(MSVC)
    add_compile_options(/utf-8)
endif()

file(GLOB_RECURSE ROSE_CORE_SOURCES "src/Core/*.cpp" "src/Core/*.c" "src/Core/*.h")
add_library(ROSE_Core STATIC ${ROSE_CORE_SOURCES})

set_target_properties(ROSE_Core PROPERTIES
    DEBUG_POSTFIX "d"
)

set_target_properties(ROSE_Core PROPERTIES
    EDITOR_POSTFIX "e"
)

find_package(SDL3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(mimalloc REQUIRED)

target_include_directories(ROSE_Core PUBLIC include/)

set_property(TARGET ROSE_Core PROPERTY FOLDER "src")

target_link_libraries(ROSE_Core
    PRIVATE SDL3::SDL3
    PRIVATE SDL3_image::SDL3_image
    PRIVATE SDL3_mixer::SDL3_mixer
    PRIVATE SDL3_ttf::SDL3_ttf
    PRIVATE imgui::imgui
    PRIVATE glad::glad
    PUBLIC  $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>
)

if (CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    target_compile_options(ROSE_Core PRIVATE -mavx2 -mfma -Wno-unknown-attributes)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(ROSE_Core PRIVATE /arch:AVX2)
endif()

add_executable(ControllerTest "examples/ControllerTest/main.cpp"
)
set_property(TARGET ControllerTest PROPERTY FOLDER "Examples")
target_link_libraries(ControllerTest PRIVATE
        ROSE_Core
        SDL3_ttf::SDL3_ttf
        SDL3_image::SDL3_image
)

file(GLOB_RECURSE game1srcs "examples/game1/*.cpp")
add_executable(Game1 ${game1srcs})


add_executable(KeyboardTest "examples/KeyboardTest/main.cpp"
)
set_property(TARGET KeyboardTest PROPERTY FOLDER "Examples")
target_link_libraries(KeyboardTest PRIVATE
        ROSE_Core
        SDL3_ttf::SDL3_ttf
        SDL3_image::SDL3_image
)

set_property(TARGET Game1 PROPERTY FOLDER "Examples")
target_link_libraries(Game1 PRIVATE
        ROSE_Core
)
add_custom_command(TARGET Game1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/game1/assets"
        "$<TARGET_FILE_DIR:Game1>/assets"
)

file(GLOB_RECURSE ROSE_EDITOR_SOURCES
        "src/Tools/Editor/*.cpp"
        "src/Tools/Editor/*.c"
)
add_executable(ROSE_Editor ${ROSE_EDITOR_SOURCES})
target_include_directories(ROSE_Editor PUBLIC include/)
set_property(TARGET ROSE_Editor PROPERTY FOLDER "Tools")

add_custom_target(ENGINE_BUILD)
add_dependencies(ENGINE_BUILD ROSE_Core ROSE_Editor)

target_compile_definitions(ROSE_Core PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Editor>:NDEBUG>
)

target_compile_definitions(ROSE_Core
    PUBLIC  $<$<CONFIG:Editor>:ROSE_EDITOR>
	PRIVATE $<$<CONFIG:Editor>:NDEBUG>
)

target_compile_definitions(ROSE_Editor
    PUBLIC  $<$<CONFIG:Editor>:ROSE_EDITOR>
    PRIVATE $<$<CONFIG:Editor>:NDEBUG>
)

target_link_libraries(ROSE_Editor
        PUBLIC ROSE_Core
        PRIVATE SDL3::SDL3
)

file(GLOB_RECURSE ROSE_ASSET_MAKER_SOURCES
        "src/Tools/AssetMaker/*.cpp"
        "src/Tools/AssetMaker/*.c"
)
add_executable(ROSE_AssetMaker ${ROSE_ASSET_MAKER_SOURCES})
target_include_directories(ROSE_AssetMaker PUBLIC include/)
set_property(TARGET ROSE_AssetMaker PROPERTY FOLDER "Tools")

find_package(CLI11 REQUIRED)

target_link_libraries(ROSE_AssetMaker PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        ROSE_Core
        CLI11::CLI11
)
set_property(TARGET ROSE_AssetMaker PROPERTY OUTPUT_NAME ROSE-masset)

file(GLOB_RECURSE ROSE_UUID_GENERATOR_SOURCES
        "src/Tools/UUIDGenerator/*.cpp"
        "src/Tools/UUIDGenerator/*.c"
)
add_executable(ROSE_UUID_Generator ${ROSE_UUID_GENERATOR_SOURCES})
target_include_directories(ROSE_UUID_Generator PUBLIC include/)
set_property(TARGET ROSE_AssetMaker PROPERTY FOLDER "Tools")

target_link_libraries(ROSE_UUID_Generator PRIVATE
        CLI11::CLI11
        ROSE_Core
)
