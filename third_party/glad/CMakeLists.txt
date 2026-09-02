# ==============================================================================
# glad -- OpenGL 4.5 compatibility loader
# ==============================================================================
#
# glad is a code generator, not a library, so there is nothing to fetch at a tag.
# The loader below was generated once and committed; regenerating it is a manual
# step that should *probably* reproduce these files byte for byte:
#
# Go online to
# http://glad.sh/#api=gl%3Acore%3D4.5&extensions=GL_ARB_bindless_texture%2CGL_ARB_parallel_shader_compile%2CGL_EXT_texture_filter_anisotropic&generator=c&options=LOADER
#
# and download the zip file.
#
# ------------------------------------------------------------------- licensing
#
# GLAD-generated code is distributed under WTFPL:
#
# ===================================================================
#            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
#                    Version 2, December 2004
#
# Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
#
# Everyone is permitted to copy and distribute verbatim or modified
# copies of this license document, and changing it is allowed as long
# as the name is changed.
#
#            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
#   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
#
#  0. You just DO WHAT THE FUCK YOU WANT TO.
# ===================================================================
#
# Now Khronos has some of their specifications under Apache Version 2.0 license, which is
# included in the accompanying LICENSE file next to this one. In addition, their khrplatform.h
# file includes a license at the top which overrides any other licenses applied to it.
#
#
#
# If I have misrepresented any of the license agreements of the code here, I hope the IP owners will let me know.
# You can reach me at <ajr.acnt@gmail.com>.

cmake_minimum_required(VERSION 3.21)

project(glad VERSION 2.0.8 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# STATIC regardless of BUILD_SHARED_LIBS. glad's function-pointer table is
# process-global mutable state, and exporting it from a DLL would need a
# GLAD_API decoration the generated source does not have. Only ROSE_Core links
# it, so one static copy is exactly one table.
add_library(glad STATIC src/gl.c)
add_library(glad::glad ALIAS glad)

target_include_directories(glad PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# glad.c calls into the platform GL loader (wglGetProcAddress / dlopen).
if(WIN32)
    target_link_libraries(glad PUBLIC opengl32)
else()
    target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()

set_target_properties(glad PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
)

# ------------------------------------------------------------------- install
install(TARGETS glad EXPORT gladTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Attribution travels with the binary. cmake/vendor.cmake collects the license
# files of every fetched dependency into this same directory; glad has no
# checkout to collect from, so it installs its own.
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/glad")

install(EXPORT gladTargets
    FILE gladTargets.cmake
    NAMESPACE glad::
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/glad"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake.in" [[
@PACKAGE_INIT@
include("${CMAKE_CURRENT_LIST_DIR}/gladTargets.cmake")
check_required_components(glad)
]])

configure_package_config_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/glad"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gladConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gladConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/gladConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/glad"
)
