cmake_minimum_required(VERSION 4.2)

# ==============================================================================
# Compiler selection  (must run before project())
# ==============================================================================

# ROSE builds with LLVM on every host. On Windows that means clang-cl -- the
# MSVC-compatible clang driver -- so we build with cl-style flags and the MSVC
# ABI. Off Windows it means the GNU-driver clang/clang++, picked explicitly
# because the platform default (`cc`/`c++`) is gcc on most Linux distributions.
# An explicit -DCMAKE_CXX_COMPILER or a CXX environment variable always wins.
#
# The C++ runtime is left alone: clang uses the system libstdc++ here, which is
# what the vcpkg dependencies are built against.
#
# Both are resolved to absolute paths before they reach the cache. A bare name
# there is not reliably resolved by CMake's compiler detection, and when it is
# not, the sibling-tool lookup that keys off the compiler's directory comes up
# empty -- CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS ends up unset and the C++20 module
# scan runs `""` as its command, failing with "/bin/sh: : command not found".
if (NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
    if (CMAKE_HOST_WIN32)
        set(_rose_c_compiler clang-cl)
        set(_rose_cxx_compiler clang-cl)
    else ()
        set(_rose_c_compiler clang)
        set(_rose_cxx_compiler clang++)
    endif ()

    find_program(ROSE_C_COMPILER NAMES ${_rose_c_compiler})
    find_program(ROSE_CXX_COMPILER NAMES ${_rose_cxx_compiler})

    if (NOT ROSE_C_COMPILER OR NOT ROSE_CXX_COMPILER)
        message(FATAL_ERROR
                "ROSE builds with LLVM, but '${_rose_c_compiler}'/'${_rose_cxx_compiler}' "
                "were not found on PATH.\n"
                "Install LLVM, or point the build at another toolchain with "
                "-DCMAKE_C_COMPILER=... -DCMAKE_CXX_COMPILER=...")
    endif ()

    set(CMAKE_C_COMPILER "${ROSE_C_COMPILER}" CACHE FILEPATH "C compiler")
    set(CMAKE_CXX_COMPILER "${ROSE_CXX_COMPILER}" CACHE FILEPATH "C++ compiler")
endif ()

# ==============================================================================
# Dependency source  (must run before project())
# ==============================================================================
#
# Two ways to get dependencies, and they are mutually exclusive:
#
#   OFF (default)  .vendor/install, built from the pinned tags in
#                  dependencies.toml by ./vendor.ps1 or ./vendor.sh. A re-run
#                  with nothing changed is a no-op.
#   ON             vcpkg in manifest mode, driven by vcpkg.json. Kept as a
#                  fallback; the *-vcpkg presets select it.

option(ROSE_USE_VCPKG "Use vcpkg for dependency management instead of .vendor" OFF)

if (ROSE_USE_VCPKG)
    # Skipped when a toolchain file was already supplied -- the vcpkg presets
    # pass one themselves.
    if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        if (NOT DEFINED ENV{VCPKG_ROOT})
            message(FATAL_ERROR "ROSE_USE_VCPKG=ON but VCPKG_ROOT environment variable is not set")
        endif ()

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
        if (_vcpkg_baseline_missing)
            message(FATAL_ERROR
                    "vcpkg HEAD is behind the required builtin-baseline '${_vcpkg_baseline}'.\n"
                    "Run: git -C \"$ENV{VCPKG_ROOT}\" pull")
        endif ()
    endif ()
else ()
    # Included by path rather than by name: CMAKE_MODULE_PATH is not set up
    # until after project(), and CMAKE_PREFIX_PATH wants to be in place first.
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ROSEVendor.cmake")
endif ()

# Pin the vcpkg triplet on Windows. x64-windows means dynamic CRT and dynamic
# dependency DLLs: the executable stays small and vcpkg deploys dependency DLLs
# next to it, at the cost of shipping the VC++ runtime alongside the game (see
# the CRT deployment block further down). x64 is safe to hardcode: the math code
# requires AVX2. Set outside the block above because the presets supply
# CMAKE_TOOLCHAIN_FILE themselves, which skips it.
#
# x64-windows-static is no longer an option. ROSE_Core is a DLL, and the engine
# allocates across that boundary in both directions -- UniquePtr/MakeUnique and
# BasicString::allocate are header-inline, so an executable can new an object that
# Core deletes and vice versa. That only works while both modules share one heap,
# which is exactly what the dynamic CRT gives us. A static CRT would give the DLL
# and each executable their own heap and turn every such pair into a crash.
# (It also cost the mimalloc `override` feature and sdl3-image's `webp` feature,
# and produced a much larger binary.)
#
# FORCE is deliberate: vcpkg's toolchain caches VCPKG_TARGET_TRIPLET on the first
# configure, so a `NOT DEFINED` guard would never fire again and a stale triplet
# would stick across a change. Override the choice with -DROSE_VCPKG_TRIPLET=...
#
# The vendored path has no triplets; it makes the same choices directly (shared
# libraries, dynamic CRT) in cmake/vendor.cmake.
if (CMAKE_HOST_WIN32 AND ROSE_USE_VCPKG)
    set(ROSE_VCPKG_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet used on Windows")
    set(VCPKG_TARGET_TRIPLET "${ROSE_VCPKG_TRIPLET}" CACHE STRING "Vcpkg target triplet" FORCE)
endif ()

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

# ROSE_Editor is shelved pending a redesign. The target, the configuration check
# that selected it, and the ENGINE_BUILD dependency are all commented out --
# restore the three blocks together, since the other two key off the variable
# this one sets.
#[[
# ROSE_Editor is added to the build tree only in the Editor configuration; see the
# target further down. That is a configure-time decision, so it keys off
# CMAKE_BUILD_TYPE -- which only a single-config generator sets. Under a
# multi-config generator the target would silently not exist, so say so instead.
get_property(_rose_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (_rose_multi_config)
    set(ROSE_BUILD_EDITOR OFF)
    message(WARNING
            "Multi-config generator '${CMAKE_GENERATOR}': ROSE_Editor is left out of this "
            "build tree. It is added only when CMAKE_BUILD_TYPE is Editor, which a "
            "multi-config generator does not set. Configure with the 'editor' preset "
            "(single-config Ninja) to build it.")
elseif (CMAKE_BUILD_TYPE STREQUAL "Editor")
    set(ROSE_BUILD_EDITOR ON)
else ()
    set(ROSE_BUILD_EDITOR OFF)
endif ()
]]

if (MSVC)
    add_compile_options(/utf-8)
else ()
    add_compile_options(-include stddef.h)
endif ()

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
if (MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
endif ()

# Copies the VC++ runtime DLLs next to <target>'s executable after it is built.
# Note this deploys the *release* runtime; Debug builds link the debug CRT
# (msvcp140d.dll etc.), which is not redistributable and is only present on
# machines with Visual Studio installed.
function(rose_deploy_crt target)
    if (MSVC AND CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
                "$<TARGET_FILE_DIR:${target}>"
                COMMAND_EXPAND_LISTS
                VERBATIM
        )
    endif ()
endfunction()

# Copies the dependency DLLs <target> loads at runtime next to its binary.
#
# vcpkg used to do this itself through applocal deployment, which is one of the
# things that disappears with ROSE_USE_VCPKG=OFF. TARGET_RUNTIME_DLLS walks the
# target's imported shared libraries transitively, so SDL3 and its satellites
# land beside every executable without naming any of them here.
#
# Empty for a target with no DLL dependencies, and the copy is a no-op then --
# hence the generator expression guard, since `copy_if_different` with no source
# arguments is an error.
function(rose_deploy_dlls target)
    if (NOT WIN32)
        return()
    endif ()

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>:${CMAKE_COMMAND}>"
            -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:${target}>"
            "$<TARGET_FILE_DIR:${target}>"
            COMMAND_EXPAND_LISTS
            VERBATIM
    )

    # DLLs that no link edge describes, so TARGET_RUNTIME_DLLS cannot see them.
    # Set by cmake/ROSEVendor.cmake; empty under vcpkg, whose applocal
    # deployment finds them by scanning PE imports instead.
    if (ROSE_EXTRA_RUNTIME_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${ROSE_EXTRA_RUNTIME_DLLS}
                "$<TARGET_FILE_DIR:${target}>"
                COMMAND_EXPAND_LISTS
                VERBATIM
        )
    endif ()
endfunction()

# Copies the third-party license texts next to <target>'s binary, as licenses/.
#
# Every dependency ROSE links is under terms that require its notice to be
# distributed with the binaries -- MIT, BSD, Zlib and Apache-2.0 all say so, and
# mpg123's LGPL says more (see THIRD_PARTY_NOTICES.md). Those binaries are copied
# out of the vendor prefix by rose_deploy_dlls() above, so the notices follow the
# same route: cmake/vendor.cmake collects them into <prefix>/share/licenses and
# this drops that tree beside the executable.
#
# copy_directory_if_different needs CMake 3.26; below that the plain copy runs
# every build, which costs a few dozen small files and is not worth a branch
# anyone has to maintain.
#
# ROSE's own LICENSE rides along, since a licenses/ directory holding only other
# people's terms invites exactly the wrong conclusion about the rest.
function(rose_deploy_licenses target)
    if (NOT ROSE_VENDOR_DIR OR NOT IS_DIRECTORY "${ROSE_VENDOR_DIR}/share/licenses")
        return()
    endif ()

    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
        set(copy_dir copy_directory_if_different)
    else ()
        set(copy_dir copy_directory)
    endif ()

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E ${copy_dir}
            "${ROSE_VENDOR_DIR}/share/licenses"
            "$<TARGET_FILE_DIR:${target}>/licenses"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSE"
            "$<TARGET_FILE_DIR:${target}>/licenses/ROSE-LICENSE.txt"
            VERBATIM
    )
endfunction()

# Everything a built target needs sitting next to it to actually run: the VC++
# runtime, the dependency DLLs, and the notices those DLLs are distributed under.
function(rose_deploy_runtime target)
    rose_deploy_crt(${target})
    rose_deploy_dlls(${target})
    rose_deploy_licenses(${target})
endfunction()

function(multiplatform_compile_options target msvc_options gnu_options)
    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE ${msvc_options})
    else ()
        target_compile_options(${target} PRIVATE ${gnu_options})
    endif ()
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
    if (NOT IS_DIRECTORY "${src_dir}")
        message(STATUS "rose_deploy_assets: ${target} has no assets directory (${src_dir}), skipping")
        return()
    endif ()

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

# Core includes <nlohmann/json.hpp> in application.cpp, paramview.cpp and
# scene.cpp, but nothing used to look for it: it resolved only because vcpkg
# installs every package into one flat include/ that arrived through some other
# target's interface. That accident does not survive vendoring, where each
# package contributes its own include directory.
find_package(nlohmann_json REQUIRED)

# ==============================================================================
# ROSE_Core  --  the engine library
# ==============================================================================

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/.git/HEAD
        ${CMAKE_SOURCE_DIR}/.git/index)

# CONFIGURE_DEPENDS on every source glob in this file, without exception. A plain
# glob is evaluated once, at configure time, and the generator has no idea the
# answer can change -- add a translation unit and the build keeps happily linking
# the old file list until someone remembers to re-run CMake by hand, which shows
# up as an unresolved external rather than as anything pointing at the cause.
# With it, the generator re-runs the glob before each build and re-configures when
# the result differs, so a new .cpp is picked up by the next `cmake --build`.
#
# The cost is a directory scan per build, which is nothing next to a compile, and
# the check is per-glob rather than per-file. Supported by Ninja, the Makefile
# generators and Visual Studio; other generators fall back to the once-only
# behaviour, so a new file there still needs a manual re-configure.
file(GLOB_RECURSE ROSE_CORE_SOURCES CONFIGURE_DEPENDS "src/Core/*.cpp" "src/Core/*.c" "src/Core/*.h")

# SHARED, not STATIC: a static Core gets copied into every module that links it,
# which means BehaviorFactory, InputSystem, MeshRegistry and Time exist once per
# module with no diagnostic. One DLL means one of each.
add_library(ROSE_Core SHARED ${ROSE_CORE_SOURCES})

execute_process(
        COMMAND git rev-parse HEAD
        OUTPUT_VARIABLE ROSE_BASELINE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_compile_definitions(ROSE_Core PRIVATE ROSE_BASELINE="${ROSE_BASELINE}")

set_target_properties(ROSE_Core PROPERTIES CXX_SCAN_FOR_MODULES OFF)

target_include_directories(ROSE_Core PUBLIC include/)

target_link_libraries(ROSE_Core
        PRIVATE SDL3::SDL3
        PRIVATE SDL3_image::SDL3_image
        PRIVATE SDL3_mixer::SDL3_mixer
        PRIVATE SDL3_ttf::SDL3_ttf
        PRIVATE imgui::imgui
        PRIVATE glad::glad
        PRIVATE nlohmann_json::nlohmann_json
        PUBLIC $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>
)

target_compile_definitions(ROSE_Core
        PUBLIC $<$<CONFIG:Editor>:ROSE_EDITOR>
        # Flips ROSE_API(CORE) from dllimport to dllexport; see api.h.
        PRIVATE ROSE_CORE_BUILD
        PRIVATE $<$<CONFIG:Debug>:_DEBUG>
        PRIVATE $<$<CONFIG:Release>:NDEBUG>
        PRIVATE $<$<CONFIG:Editor>:NDEBUG>
)

# TODO make avx an optional thing without breaking ABI

# The math code assumes AVX2 + FMA are available.
if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # cl.exe or clang-cl: MSVC-style flags. /arch:AVX2 also enables FMA.
    target_compile_options(ROSE_Core PRIVATE /arch:AVX2 /O2)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(ROSE_Core PRIVATE -Wno-unknown-attributes)
    endif ()
else ()
    # gcc or clang++ (GNU driver)
    target_compile_options(ROSE_Core PRIVATE -mavx2 -mfma -Wno-unknown-attributes -Ofast)
endif ()

# The visibility properties are no-ops under clang-cl, where nothing is exported
# unless it is marked; they are what makes the ELF branch of ROSE_API_EXPORT in
# api.h mean something once this builds outside Windows.
set_target_properties(ROSE_Core PROPERTIES
        DEBUG_POSTFIX "d"
        EDITOR_POSTFIX "e"
        FOLDER "src"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON
)

rose_deploy_runtime(ROSE_Core)

# ==============================================================================
# Tools
# ==============================================================================

# ------------------------------------------------------------------ ROSE_Editor
#
# Commented out pending a redesign; see the note by ROSE_BUILD_EDITOR further up.
#[[
# Added only in the Editor configuration: editor.h refuses to compile unless
# ROSE_EDITOR is defined, and that happens in exactly that configuration. See the
# ROSE_BUILD_EDITOR block near the top of this file.
if (ROSE_BUILD_EDITOR)
    file(GLOB_RECURSE ROSE_EDITOR_SOURCES CONFIGURE_DEPENDS
            "src/Tools/Editor/*.cpp"
            "src/Tools/Editor/*.c"
    )

    add_executable(ROSE_Editor ${ROSE_EDITOR_SOURCES})

    target_include_directories(ROSE_Editor PUBLIC include/)

    # ROSE_Core's own dependencies are PRIVATE and no longer propagate now that it is
    # a shared library, so anything this target uses directly is listed here. It drives
    # SDL and ImGui itself and never constructs a ROSE::Application, so it owns its own
    # ImGui context -- do not call ROSE::AttachImGui() from here.
    target_link_libraries(ROSE_Editor PRIVATE
            ROSE_Core
            SDL3::SDL3
            imgui::imgui
    )

    # No $<CONFIG:Editor> guard needed: this block only runs in that configuration.
    target_compile_definitions(ROSE_Editor PRIVATE ROSE_EDITOR NDEBUG)

    set_property(TARGET ROSE_Editor PROPERTY FOLDER "Tools")

    rose_deploy_runtime(ROSE_Editor)
endif ()
]]

# --------------------------------------------------------------- ROSE_AssetMaker
file(GLOB_RECURSE ROSE_ASSET_MAKER_SOURCES CONFIGURE_DEPENDS
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
        FOLDER "Tools"
)

rose_deploy_runtime(ROSE_AssetMaker)

# ----------------------------------------------------------- ROSE_UUID_Generator
file(GLOB_RECURSE ROSE_UUID_GENERATOR_SOURCES CONFIGURE_DEPENDS
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

rose_deploy_runtime(ROSE_UUID_Generator)

# --------------------------------------------------------- aggregate build target
add_custom_target(ENGINE_BUILD)
add_dependencies(ENGINE_BUILD ROSE_Core ROSE_UUID_Generator ROSE_AssetMaker)
# Commented out with ROSE_Editor; see the note near the top of this file.
#[[
if (ROSE_BUILD_EDITOR)
    add_dependencies(ENGINE_BUILD ROSE_Editor)
endif ()
]]

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

rose_deploy_runtime(ControllerTest)

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

rose_deploy_runtime(KeyboardTest)

# ------------------------------------------------------------------------ Game1
file(GLOB_RECURSE GAME1_SOURCES CONFIGURE_DEPENDS "examples/game1/*.cpp")

add_executable(Game1 ${GAME1_SOURCES})

# Ball and Paddle render through SDL directly, and FpsCounter draws with ImGui, so
# both are first-party dependencies of this target rather than something inherited
# from ROSE_Core. FpsCounter runs inside a ROSE::Application, which means it draws
# into Core's ImGui context -- main.cpp calls ROSE::AttachImGui() to bind it.
target_link_libraries(Game1 PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        imgui::imgui
)

set_property(TARGET Game1 PROPERTY FOLDER "Examples")

rose_deploy_runtime(Game1)

rose_deploy_assets(Game1 "${CMAKE_CURRENT_SOURCE_DIR}/examples/game1/assets")

# ----------------------------------------------------------------------- Orbits
file(GLOB_RECURSE ORBITS_SOURCES CONFIGURE_DEPENDS "examples/orbits/*.cpp")

add_executable(Orbits ${ORBITS_SOURCES})

# No SDL3 here, deliberately. The point cloud used to call the render API directly, which made SDL a first-party
# dependency of the demo; it now hands geometry up as a Renderable and never names a backend. If SDL3 ever needs to
# come back to this line, something has leaked through the abstraction.
target_link_libraries(Orbits PRIVATE
        ROSE_Core
        imgui::imgui
)

set_property(TARGET Orbits PROPERTY FOLDER "Examples")

rose_deploy_runtime(Orbits)

rose_deploy_assets(Orbits "${CMAKE_CURRENT_SOURCE_DIR}/examples/orbits/assets")

# ------------------------------------------------------------------------ Game2
file(GLOB_RECURSE GAME2_SOURCES CONFIGURE_DEPENDS "examples/game2/*.cpp")

add_executable(Game2 ${GAME2_SOURCES})

# vnstuff.h includes SDL_ttf and nlohmann/json.hpp directly.
target_link_libraries(Game2 PRIVATE
        ROSE_Core
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        nlohmann_json::nlohmann_json
)

set_property(TARGET Game2 PROPERTY FOLDER "Examples")

rose_deploy_runtime(Game2)

rose_deploy_assets(Game2 "${CMAKE_CURRENT_SOURCE_DIR}/examples/game2/assets")

add_custom_target(EXAMPLES_BUILD)
add_dependencies(EXAMPLES_BUILD Game1 Game2 Orbits ControllerTest KeyboardTest)
