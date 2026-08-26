# ==============================================================================
# glad -- OpenGL 4.5 compatibility loader
# ==============================================================================
#
# glad is a code generator, not a library, so there is nothing to fetch at a tag.
# The loader below was generated once and committed; regenerating it is a manual
# step that should reproduce these files byte for byte:
#
#     pip install glad==0.1.36
#     python -m glad --profile="compatibility" --api="gl=4.5" --generator="c" \
#                    --spec="gl" --extensions="" --reproducible --out-path=.
#
# --reproducible is what keeps the output stable (it pins the packaged gl.xml and
# blanks the generation timestamp), and the result is byte-identical to what the
# vcpkg `glad[gl-api-45]` port installs, so switching between the two changes
# nothing for consumers.
#
# The vendor engine (cmake/vendor.cmake) builds this like any other dependency;
# it is declared with `path =` instead of `repo`/`tag` in dependencies.toml.

cmake_minimum_required(VERSION 3.21)

project(glad VERSION 0.1.36 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# STATIC regardless of BUILD_SHARED_LIBS. glad's function-pointer table is
# process-global mutable state, and exporting it from a DLL would need a
# GLAD_API decoration the generated source does not have. Only ROSE_Core links
# it, so one static copy is exactly one table.
add_library(glad STATIC src/glad.c)
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
