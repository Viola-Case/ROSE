# ==============================================================================
# Dear ImGui build shim
# ==============================================================================
#
# imgui ships no build system, which is why vcpkg needed a port for it. This is
# the replacement: a small CMake project that the vendor engine points at the
# imgui checkout via -DVENDOR_SOURCE_DIR.
#
# It reproduces what the vcpkg port produced, so no ROSE source has to change:
#
#   * one STATIC library exported as imgui::imgui
#   * headers installed FLAT into include/, because ROSE includes them
#     unqualified -- `#include <imgui_impl_sdl3.h>`, not <backends/...>
#
# STATIC is load-bearing, not incidental. Each module that links imgui gets its
# own GImGui, which is exactly the situation ROSE::AttachImGui() exists to
# handle: ROSE_Core owns a context and an executable drawing into it has to be
# bound to Core's, while ROSE_Editor deliberately owns a separate one.

cmake_minimum_required(VERSION 3.21)

project(imgui LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ------------------------------------------------------------------- inputs
if(NOT DEFINED VENDOR_SOURCE_DIR)
    message(FATAL_ERROR
        "VENDOR_SOURCE_DIR is not set. This project is built by "
        "cmake/vendor.cmake, which points it at the imgui checkout; it is not "
        "meant to be configured directly.")
endif()

if(NOT EXISTS "${VENDOR_SOURCE_DIR}/imgui.cpp")
    message(FATAL_ERROR "No imgui.cpp under VENDOR_SOURCE_DIR=${VENDOR_SOURCE_DIR}")
endif()

# Comma-separated so the manifest never needs a semicolon; see the note about
# semicolons in cmake/vendor/toml.cmake.
set(IMGUI_BACKENDS "sdl3,sdlrenderer3,opengl3"
    CACHE STRING "Comma-separated imgui backends to compile")
string(REPLACE "," ";" imgui_backends "${IMGUI_BACKENDS}")

# Read the version out of the checkout rather than hardcoding it, so bumping the
# tag in dependencies.toml is the only edit a version change needs.
file(STRINGS "${VENDOR_SOURCE_DIR}/imgui.h" imgui_version_line
     REGEX "^#define +IMGUI_VERSION +\"")
if(imgui_version_line MATCHES "\"([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(IMGUI_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
    set(IMGUI_VERSION "0.0.0")
endif()
message(STATUS "imgui ${IMGUI_VERSION}, backends: ${imgui_backends}")

# ------------------------------------------------------------------ sources
set(imgui_sources
    "${VENDOR_SOURCE_DIR}/imgui.cpp"
    "${VENDOR_SOURCE_DIR}/imgui_draw.cpp"
    "${VENDOR_SOURCE_DIR}/imgui_tables.cpp"
    "${VENDOR_SOURCE_DIR}/imgui_widgets.cpp"
    # The demo window is worth having compiled in: it is the fastest way to
    # check that a renderer backend is actually working.
    "${VENDOR_SOURCE_DIR}/imgui_demo.cpp"
)

set(imgui_headers
    "${VENDOR_SOURCE_DIR}/imgui.h"
    "${VENDOR_SOURCE_DIR}/imconfig.h"
    "${VENDOR_SOURCE_DIR}/imgui_internal.h"
    "${VENDOR_SOURCE_DIR}/imstb_rectpack.h"
    "${VENDOR_SOURCE_DIR}/imstb_textedit.h"
    "${VENDOR_SOURCE_DIR}/imstb_truetype.h"
)

set(imgui_needs_sdl FALSE)

foreach(backend IN LISTS imgui_backends)
    set(backend_cpp "${VENDOR_SOURCE_DIR}/backends/imgui_impl_${backend}.cpp")
    set(backend_h   "${VENDOR_SOURCE_DIR}/backends/imgui_impl_${backend}.h")

    if(NOT EXISTS "${backend_cpp}")
        message(FATAL_ERROR
            "imgui backend '${backend}' has no ${backend_cpp}. "
            "Check IMGUI_BACKENDS in dependencies.toml.")
    endif()

    list(APPEND imgui_sources "${backend_cpp}")
    list(APPEND imgui_headers "${backend_h}")

    if(backend MATCHES "^sdl3")
        set(imgui_needs_sdl TRUE)
    endif()

    # imgui_impl_opengl3.cpp uses its own bundled loader by default, so glad is
    # not involved in building imgui even though ROSE loads GL through glad.
    # The header is installed because the backend header includes it.
    if(backend STREQUAL "opengl3")
        list(APPEND imgui_headers
             "${VENDOR_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.h")
    endif()
endforeach()

# ------------------------------------------------------------------- library
add_library(imgui STATIC ${imgui_sources})
add_library(imgui::imgui ALIAS imgui)

target_compile_features(imgui PUBLIC cxx_std_11)

target_include_directories(imgui PUBLIC
    $<BUILD_INTERFACE:${VENDOR_SOURCE_DIR}>
    $<BUILD_INTERFACE:${VENDOR_SOURCE_DIR}/backends>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(imgui_needs_sdl)
    # Comes from the shared vendor prefix, which the engine puts on
    # CMAKE_PREFIX_PATH; `needs = ["SDL3"]` in the manifest guarantees ordering.
    find_package(SDL3 CONFIG REQUIRED)
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
endif()

set_target_properties(imgui PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
)

# ------------------------------------------------------------------- install
install(TARGETS imgui EXPORT imguiTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# Flat, deliberately: matches how ROSE and the vcpkg port both spell the
# includes. Nesting the backends would break every existing #include.
install(FILES ${imgui_headers} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT imguiTargets
    FILE imguiTargets.cmake
    NAMESPACE imgui::
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/imgui"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/imguiConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/imgui"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
    VERSION ${IMGUI_VERSION}
    COMPATIBILITY SameMinorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/imguiConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/imgui"
)
