# ==============================================================================
# Consume the vendored dependency tree
# ==============================================================================
#
# Included by the root CMakeLists when ROSE_USE_VCPKG is OFF (the default). It
# points find_package at the prefix that cmake/vendor.cmake installs into, so
# every find_package(SDL3), find_package(imgui) and so on resolves there instead
# of through a toolchain file.
#
# Populate the prefix with ./vendor.ps1 or ./vendor.sh; see dependencies.toml.

set(ROSE_VENDOR_DIR "${CMAKE_SOURCE_DIR}/.vendor/install"
    CACHE PATH "Prefix holding the vendored dependencies")

if(NOT IS_DIRECTORY "${ROSE_VENDOR_DIR}")
    if(CMAKE_HOST_WIN32)
        set(_rose_vendor_cmd "./vendor.ps1")
    else()
        set(_rose_vendor_cmd "./vendor.sh")
    endif()
    message(FATAL_ERROR
        "No vendored dependencies found at:\n"
        "    ${ROSE_VENDOR_DIR}\n\n"
        "Run ${_rose_vendor_cmd} to fetch and build them, or configure with "
        "-DROSE_USE_VCPKG=ON to use vcpkg instead.")
endif()

# PREPEND so the vendored tree beats anything already installed system-wide --
# a stray SDL3 in /usr/local must not win over the version pinned in
# dependencies.toml.
list(PREPEND CMAKE_PREFIX_PATH "${ROSE_VENDOR_DIR}")

# ------------------------------------------------------------ Editor config
# "Editor" is a ROSE-only build configuration, so imported targets have no
# Editor entry and CMake falls back to whichever config it finds first -- in
# practice Debug. That is what produced the link failure documented in
# docs/internal/README.md:
#
#   lld-link: error: /failifmismatch: mismatch detected for '_ITERATOR_DEBUG_LEVEL'
#
# Editor compiles with NDEBUG and the release CRT, so it must import the release
# artifacts. Only targets linking a real static archive noticed, but the mapping
# is correct for every dependency.
set(CMAKE_MAP_IMPORTED_CONFIG_EDITOR Release "")

# Dependency DLLs live in the prefix's bin/ and are copied next to each
# executable by rose_deploy_dlls() at build time -- the job vcpkg's applocal
# deployment used to do. Exported here so that helper, and anyone debugging a
# missing-DLL failure, has the path.
set(ROSE_VENDOR_BIN_DIR "${ROSE_VENDOR_DIR}/bin")

# ------------------------------------------------- DLLs with no link edge
# rose_deploy_dlls() finds dependency DLLs through $<TARGET_RUNTIME_DLLS:...>,
# which walks CMake's own link graph. That misses any DLL a dependency loads
# without being linked against it, because no CMake target describes the
# relationship. vcpkg's applocal deployment caught these by scanning the PE
# import tables instead; here they have to be named.
#
# mimalloc-redirect.dll is the one that matters: with MI_OVERRIDE=ON, mimalloc
# uses it to patch the CRT's allocator entry points, and it must sit beside the
# executable. Without it the override silently does not happen -- or the process
# fails to start.
set(ROSE_EXTRA_RUNTIME_DLLS "")

file(GLOB _rose_loose_dlls "${ROSE_VENDOR_BIN_DIR}/mimalloc-redirect*.dll")
list(APPEND ROSE_EXTRA_RUNTIME_DLLS ${_rose_loose_dlls})
unset(_rose_loose_dlls)

if(ROSE_EXTRA_RUNTIME_DLLS)
    message(STATUS "ROSE: also deploying ${ROSE_EXTRA_RUNTIME_DLLS}")
endif()

message(STATUS "ROSE: using vendored dependencies from ${ROSE_VENDOR_DIR}")
