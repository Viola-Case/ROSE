cmake_minimum_required(VERSION 4.2)

# ==============================================================================
# Compiler selection  (must run before project())
# ==============================================================================

# On Windows, default to clang-cl -- the MSVC-compatible clang driver -- so we
# build with cl-style flags and the MSVC ABI. Off Windows (Linux/macOS) this is
# skipped and the native clang++/gcc is used. An explicit -DCMAKE_CXX_COMPILER
# or a CXX environment variable always wins.
if(CMAKE_HOST_WIN32
        AND NOT DEFINED CMAKE_CXX_COMPILER
        AND NOT DEFINED ENV{CXX})
    set(CMAKE_C_COMPILER   clang-cl CACHE STRING "C compiler")
    set(CMAKE_CXX_COMPILER clang-cl CACHE STRING "C++ compiler")
endif()

# ==============================================================================
# vcpkg bootstrap  (must run before project())
# ==============================================================================

option(ROSE_USE_VCPKG "Use vcpkg for dependency management" OFF)

if(ROSE_USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(NOT DEFINED ENV{VCPKG_ROOT})
        message(FATAL_ERROR "ROSE_USE_VCPKG=ON but VCPKG_ROOT environment variable is not set")
    endif()

    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
    set(VCPKG_MANIFEST_MODE ON)

    # Fail early (with a fix) if the local vcpkg checkout predates our baseline.
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
endif()

# Pin the vcpkg triplet on Windows. x64-windows means dynamic CRT and dynamic
# dependency DLLs: the executable stays small and vcpkg deploys dependency DLLs
# next to it, at the cost of shipping the VC++ runtime alongside the game (see
# the CRT deployment block further down). x64 is safe to hardcode: the math code
# requires AVX2. Set outside the block above because the presets supply
# CMAKE_TOOLCHAIN_FILE themselves, which skips it.
#
# x64-windows-static builds a single self-contained exe with no redistributable,
# but costs the mimalloc `override` feature and sdl3-image's `webp` feature --
# neither is supported under static library linkage -- and produces a much larger
# binary. Switching back also means restoring CMAKE_MSVC_RUNTIME_LIBRARY below.
#
# FORCE is deliberate: vcpkg's toolchain caches VCPKG_TARGET_TRIPLET on the first
# configure, so a `NOT DEFINED` guard would never fire again and a stale triplet
# would stick across a change. Override the choice with -DROSE_VCPKG_TRIPLET=...
if(CMAKE_HOST_WIN32)
    set(ROSE_VCPKG_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet used on Windows")
    set(VCPKG_TARGET_TRIPLET "${ROSE_VCPKG_TRIPLET}" CACHE STRING "Vcpkg target triplet" FORCE)
endif()

# ==============================================================================
# Project
# ==============================================================================

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(ROSE VERSION 0.0.1)

# ------------------------------------------------------------------ toolchain
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------------------- build layout
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# "Editor" is a custom configuration; see the matching preset in CMakePresets.json.
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Editor" CACHE STRING "" FORCE)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# ------------------------------------------------------- VC++ runtime deployment
# vcpkg's applocal deployment copies dependency DLLs (SDL3, libpng, ...) next to
# the executable automatically, but deliberately skips system DLLs, so the VC++
# runtime is on us. InstallRequiredSystemLibraries locates the redistributable
# DLLs for the active toolset and puts them in CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS.
#
# _SKIP stops it from generating install() rules; we only want the variable, and
# copy the DLLs ourselves so the build tree is runnable too.
#
# The UCRT (ucrtbase.dll) is intentionally not included: it ships as an OS
# component on Windows 10 and later. Set CMAKE_INSTALL_UCRT_LIBRARIES to bundle it.
if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
endif()

# Copies the VC++ runtime DLLs next to <target>'s executable after it is built.
# Note this deploys the *release* runtime; Debug builds link the debug CRT
# (msvcp140d.dll etc.), which is not redistributable and is only present on
# machines with Visual Studio installed.
function(rose_deploy_crt target)
    if(MSVC AND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
                "$<TARGET_FILE_DIR:${target}>"
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
    endif()
endfunction()

# Mirrors <src_dir> into <target>'s output directory as "assets/".
#
# This deliberately does not use a POST_BUILD command: those hang off the link
# step, so touching an asset leaves the executable up to date and the generator
# has no work to do -- the edit never reaches the build tree. Instead the copy is
# a real build rule with the asset files as dependencies, driven by a stamp file
# (the destination tree is not a single output the generator can check).
#
# CONFIGURE_DEPENDS re-runs the glob at build time, so newly added assets are
# picked up without a manual re-configure. Assets *deleted* from the source tree
# are left behind in the build tree; wipe the output directory to clear them.
function(rose_deploy_assets target src_dir)
    # Examples without an assets directory yet are not an error, but adding one
    # later needs a re-configure for this check to see it.
    if(NOT IS_DIRECTORY "${src_dir}")
        message(STATUS "rose_deploy_assets: ${target} has no assets directory (${src_dir}), skipping")
        return()
    endif()

    file(GLOB_RECURSE assets CONFIGURE_DEPENDS "${src_dir}/*")

    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_assets.stamp")

    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${src_dir}"
            "$<TARGET_FILE_DIR:${target}>/assets"
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${assets}
        COMMENT "Deploying assets for ${target}"
        VERBATIM
    )

    add_custom_target(${target}_assets DEPENDS "${stamp}")
    set_property(TARGET ${target}_assets PROPERTY FOLDER "Assets")
    add_dependencies(${target} ${target}_assets)
endfunction()

# ==============================================================================
# Dependencies
# ==============================================================================

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(imgui REQUIRED)
find_package(glad REQUIRED)
find_package(mimalloc REQUIRED)
find_package(CLI11 REQUIRED)

# ==============================================================================
# ROSE_Core  --  the engine library
# ==============================================================================

file(GLOB_RECURSE ROSE_CORE_SOURCES "src/Core/*.cpp" "src/Core/*.c" "src/Core/*.h")

add_library(ROSE_Core STATIC ${ROSE_CORE_SOURCES})

target_include_directories(ROSE_Core PUBLIC include/)

target_link_libraries(ROSE_Core
    PRIVATE SDL3::SDL3
    PRIVATE SDL3_image::SDL3_image
    PRIVATE SDL3_mixer::SDL3_mixer
    PRIVATE SDL3_ttf::SDL3_ttf
    PRIVATE imgui::imgui
    PRIVATE glad::glad
    PUBLIC  $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>
)

target_compile_definitions(ROSE_Core
    PUBLIC  $<$<CONFIG:Editor>:ROSE_EDITOR>
    PRIVATE $<$<CONFIG:Debug>:_DEBUG>
    PRIVATE $<$<CONFIG:Release>:NDEBUG>
    PRIVATE $<$<CONFIG:Editor>:NDEBUG>
)

# The math code assumes AVX2 + FMA are available.
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # cl.exe or clang-cl: MSVC-style flags. /arch:AVX2 also enables FMA.
    target_compile_options(ROSE_Core PRIVATE /arch:AVX2)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(ROSE_Core PRIVATE -Wno-unknown-attributes)
    endif()
else()
    # gcc or clang++ (GNU driver)
    target_compile_options(ROSE_Core PRIVATE -mavx2 -mfma -Wno-unknown-attributes)
endif()

set_target_properties(ROSE_Core PROPERTIES
    DEBUG_POSTFIX  "d"
    EDITOR_POSTFIX "e"
    FOLDER         "src"
)

# ==============================================================================
# Tools
# ==============================================================================

# ------------------------------------------------------------------ ROSE_Editor
file(GLOB_RECURSE ROSE_EDITOR_SOURCES
        "src/Tools/Editor/*.cpp"
        "src/Tools/Editor/*.c"
)

add_executable(ROSE_Editor ${ROSE_EDITOR_SOURCES})

target_include_directories(ROSE_Editor PUBLIC include/)

# Everything this target uses directly is listed here rather than inherited from
# ROSE_Core, whose own dependencies are PRIVATE. It drives SDL and ImGui itself
# and never constructs a ROSE::Application, so it owns its own ImGui context.
target_link_libraries(ROSE_Editor PRIVATE
        ROSE_Core
        SDL3::SDL3
        imgui::imgui
)

target_compile_definitions(ROSE_Editor
    PUBLIC  $<$<CONFIG:Editor>:ROSE_EDITOR>
    PRIVATE $<$<CONFIG:Editor>:NDEBUG>
)

set_property(TARGET ROSE_Editor PROPERTY FOLDER "Tools")

rose_deploy_crt(ROSE_Editor)

# --------------------------------------------------------------- ROSE_AssetMaker
file(GLOB_RECURSE ROSE_ASSET_MAKER_SOURCES
        "src/Tools/AssetMaker/*.cpp"
        "src/Tools/AssetMaker/*.c"
)

add_executable(ROSE_AssetMaker ${ROSE_ASSET_MAKER_SOURCES})

target_include_directories(ROSE_AssetMaker PUBLIC include/)

target_link_libraries(ROSE_AssetMaker PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        CLI11::CLI11
)

set_target_properties(ROSE_AssetMaker PROPERTIES
    OUTPUT_NAME "ROSE-masset"
    FOLDER      "Tools"
)

rose_deploy_crt(ROSE_AssetMaker)

# ----------------------------------------------------------- ROSE_UUID_Generator
file(GLOB_RECURSE ROSE_UUID_GENERATOR_SOURCES
        "src/Tools/UUIDGenerator/*.cpp"
        "src/Tools/UUIDGenerator/*.c"
)

add_executable(ROSE_UUID_Generator ${ROSE_UUID_GENERATOR_SOURCES})

target_include_directories(ROSE_UUID_Generator PUBLIC include/)

target_link_libraries(ROSE_UUID_Generator PRIVATE
        ROSE_Core
        CLI11::CLI11
)

set_property(TARGET ROSE_UUID_Generator PROPERTY FOLDER "Tools")

rose_deploy_crt(ROSE_UUID_Generator)

# --------------------------------------------------------- aggregate build target
add_custom_target(ENGINE_BUILD)
add_dependencies(ENGINE_BUILD ROSE_Core ROSE_Editor)

# ==============================================================================
# Examples
# ==============================================================================

# --------------------------------------------------------------- ControllerTest
add_executable(ControllerTest "examples/ControllerTest/main.cpp")

# Standalone SDL/ImGui program that borrows only ROSE's RTL. It creates its own
# ImGui context, so the two copies of ImGui here are independent by design.
target_link_libraries(ControllerTest PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        imgui::imgui
)

set_property(TARGET ControllerTest PROPERTY FOLDER "Examples")

rose_deploy_crt(ControllerTest)

# ----------------------------------------------------------------- KeyboardTest
add_executable(KeyboardTest "examples/KeyboardTest/main.cpp")

# Same shape as ControllerTest: its own SDL init, its own ImGui context.
target_link_libraries(KeyboardTest PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        imgui::imgui
)

set_property(TARGET KeyboardTest PROPERTY FOLDER "Examples")

rose_deploy_crt(KeyboardTest)

# ------------------------------------------------------------------------ Game1
file(GLOB_RECURSE GAME1_SOURCES "examples/game1/*.cpp")

add_executable(Game1 ${GAME1_SOURCES})

# Ball and Paddle render through SDL directly and FpsCounter draws with ImGui, so
# both are first-party dependencies of this target rather than implementation
# details of ROSE_Core.
target_link_libraries(Game1 PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        imgui::imgui
)

set_property(TARGET Game1 PROPERTY FOLDER "Examples")

rose_deploy_crt(Game1)

rose_deploy_assets(Game1 "${CMAKE_CURRENT_SOURCE_DIR}/examples/game1/assets")

# ----------------------------------------------------------------------- Orbits
file(GLOB_RECURSE ORBITS_SOURCES "examples/orbits/*.cpp")

add_executable(Orbits ${ORBITS_SOURCES})

# SDL3 is linked explicitly rather than picked up through ROSE_Core: this example calls the render API directly to
# batch its point cloud, so it is a first-party dependency of the target and not an implementation detail.
target_link_libraries(Orbits PRIVATE
        ROSE_Core
        SDL3::SDL3
)

set_property(TARGET Orbits PROPERTY FOLDER "Examples")

rose_deploy_crt(Orbits)

rose_deploy_assets(Orbits "${CMAKE_CURRENT_SOURCE_DIR}/examples/orbits/assets")

# ------------------------------------------------------------------------ Game2
file(GLOB_RECURSE GAME2_SOURCES "examples/game2/*.cpp")

add_executable(Game2 ${GAME2_SOURCES})

# vnstuff.h includes SDL_ttf directly.
target_link_libraries(Game2 PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
)

set_property(TARGET Game2 PROPERTY FOLDER "Examples")

rose_deploy_crt(Game2)

rose_deploy_assets(Game2 "${CMAKE_CURRENT_SOURCE_DIR}/examples/game2/assets")
