# ==============================================================================
# ROSE dependency vendoring engine
# ==============================================================================
#
# Reads dependencies.toml, checks each dependency out at its pinned tag, builds
# it, and installs everything into one shared prefix that the root CMakeLists
# puts on CMAKE_PREFIX_PATH.
#
# Run it through the launchers rather than directly:
#
#     ./vendor.ps1  [flags]        (Windows)
#     ./vendor.sh   [flags]        (Linux, macOS)
#
# or, equivalently:
#
#     cmake -P cmake/vendor.cmake -- [flags]
#
# The design goal is that the *second* run does nothing. Each dependency has a
# stamp holding a hash of everything that could affect its output -- tag, cmake
# arguments, configurations, generator, compilers, prefix -- and a build is
# skipped outright when the stamp still matches and the install is intact. A
# no-op run makes no network calls and finishes in well under a second.
#
# There is deliberately no dependency solver. See the header of
# dependencies.toml for how transitive dependencies are handled instead.

cmake_minimum_required(VERSION 3.21)

# Bumping this invalidates every stamp, which is the escape hatch for a change in
# how the engine builds things (new default flag, changed layout) that no
# per-dependency key would otherwise capture.
set(VENDOR_FORMAT_VERSION 1)

get_filename_component(VENDOR_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(REPO_ROOT "${VENDOR_CMAKE_DIR}/.." ABSOLUTE)

include("${VENDOR_CMAKE_DIR}/vendor/toml.cmake")

# ==============================================================================
# Small helpers
# ==============================================================================

# A single named parameter rather than ARGN. "${ARGN}" would put a stray ';'
# between the pieces of a call split across several string arguments, and
# list(JOIN) instead swallows any ';' that is genuinely part of the text --
# a named argument preserves the string exactly as written.
function(vendor_say text)
    message(STATUS "${text}")
endfunction()

function(vendor_die text)
    message(FATAL_ERROR "vendor: ${text}")
endfunction()

# Content hash of a directory tree. Used so that editing a build shim or the
# in-tree glad sources invalidates the stamp exactly like bumping a tag would.
function(vendor_hash_tree dir out)
    if(NOT IS_DIRECTORY "${dir}")
        set(${out} "absent" PARENT_SCOPE)
        return()
    endif()

    file(GLOB_RECURSE files RELATIVE "${dir}" "${dir}/*")
    list(SORT files)

    set(acc "")
    foreach(f IN LISTS files)
        if(NOT IS_DIRECTORY "${dir}/${f}")
            file(SHA256 "${dir}/${f}" h)
            string(APPEND acc "${f}:${h}|")
        endif()
    endforeach()

    string(SHA256 result "${acc}")
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

function(vendor_read_file path out)
    if(EXISTS "${path}")
        file(READ "${path}" value)
        string(STRIP "${value}" value)
        set(${out} "${value}" PARENT_SCOPE)
    else()
        set(${out} "" PARENT_SCOPE)
    endif()
endfunction()

# Splits text into a list of lines.
#
# file(STRINGS) and a plain REPLACE to ";" both get this wrong, because CMake's
# list splitting treats [ ... ] as a group and refuses to split inside it -- and
# ninja output ("[1/240] Building ...") and install manifests are full of
# brackets. Hiding them across the split is the fix; see cmake/vendor/toml.cmake.
function(vendor_split_lines text out)
    string(ASCII 2 lbracket)
    string(ASCII 3 rbracket)
    string(REPLACE "[" "${lbracket}" text "${text}")
    string(REPLACE "]" "${rbracket}" text "${text}")
    string(REPLACE ";" "\\;" text "${text}")
    string(REPLACE "\r" "" text "${text}")
    string(REPLACE "\n" ";" text "${text}")

    set(lines "")
    foreach(line IN LISTS text)
        string(REPLACE "${lbracket}" "[" line "${line}")
        string(REPLACE "${rbracket}" "]" line "${line}")
        list(APPEND lines "${line}")
    endforeach()

    set(${out} "${lines}" PARENT_SCOPE)
endfunction()

# Runs a command, streaming its output unless --quiet. On failure the caller gets
# rc != 0 and, in quiet mode, a log file to point at.
function(vendor_run)
    cmake_parse_arguments(RUN "" "WORKING_DIRECTORY;LOG;RESULT" "COMMAND" ${ARGN})

    set(extra "")
    if(RUN_WORKING_DIRECTORY)
        list(APPEND extra WORKING_DIRECTORY "${RUN_WORKING_DIRECTORY}")
    endif()

    if(VENDOR_QUIET AND RUN_LOG)
        get_filename_component(log_dir "${RUN_LOG}" DIRECTORY)
        file(MAKE_DIRECTORY "${log_dir}")
        execute_process(
            COMMAND ${RUN_COMMAND}
            ${extra}
            RESULT_VARIABLE rc
            OUTPUT_FILE "${RUN_LOG}"
            ERROR_FILE  "${RUN_LOG}"
        )
    else()
        execute_process(
            COMMAND ${RUN_COMMAND}
            ${extra}
            RESULT_VARIABLE rc
        )
    endif()

    set(${RUN_RESULT} "${rc}" PARENT_SCOPE)
endfunction()

# ==============================================================================
# Command line
# ==============================================================================

set(VENDOR_HELP [[
ROSE dependency vendoring

  ./vendor.ps1 [flags]        Windows
  ./vendor.sh  [flags]        Linux, macOS

Flags:
  --list                 Show every dependency and its state; touches no network
  --force                Rebuild everything, ignoring stamps
  --only <name>          Act on one dependency (repeatable); implies its needs
  --clean [<name>]       Drop build trees and stamps so the next run rebuilds
  --purge                Delete .vendor entirely, sources and install included
  --config <cfg>         Build only this configuration (default: from manifest).
                         The stamp records what was installed, so a later run
                         without this flag rebuilds the configurations it skipped
  --jobs <n>             Parallel build jobs (default: CPU count)
  --generator <name>     CMake generator (default: Ninja when available)
  --c-compiler <path>    C compiler for dependencies (default: CC, else CMake's)
  --cxx-compiler <path>  C++ compiler for dependencies (default: CXX, else CMake's)
  --manifest <path>      Manifest to read (default: dependencies.toml)
  --quiet                Log dependency build output to .vendor/logs instead of
                         streaming it; the log is shown if a build fails
  --help                 This message
]])

set(VENDOR_FORCE       FALSE)
set(VENDOR_LIST        FALSE)
set(VENDOR_CLEAN       FALSE)
set(VENDOR_PURGE       FALSE)
set(VENDOR_QUIET       FALSE)
set(VENDOR_ONLY        "")
set(VENDOR_CLEAN_NAMES "")
set(VENDOR_CONFIG      "")
set(VENDOR_JOBS        "")
set(VENDOR_GENERATOR   "")
set(VENDOR_CC          "$ENV{CC}")
set(VENDOR_CXX         "$ENV{CXX}")
set(VENDOR_MANIFEST    "${REPO_ROOT}/dependencies.toml")

# Everything after the `--` that separates cmake's own arguments from ours.
set(args "")
set(seen_separator FALSE)
math(EXPR last_arg "${CMAKE_ARGC} - 1")
foreach(i RANGE 1 ${last_arg})
    set(a "${CMAKE_ARGV${i}}")
    if(seen_separator)
        list(APPEND args "${a}")
    elseif(a STREQUAL "--")
        set(seen_separator TRUE)
    endif()
endforeach()

# `--clean` takes an optional value, so the parse is a hand-rolled walk rather
# than cmake_parse_arguments.
list(LENGTH args argc)
set(i 0)
while(i LESS argc)
    list(GET args ${i} arg)
    math(EXPR i "${i} + 1")

    # Pulls the next argument as <arg>'s value, erroring if it is missing.
    macro(_vendor_take out)
        if(i GREATER_EQUAL argc)
            vendor_die("${arg} needs a value")
        endif()
        list(GET args ${i} ${out})
        math(EXPR i "${i} + 1")
    endmacro()

    if(arg STREQUAL "--help" OR arg STREQUAL "-h")
        message("${VENDOR_HELP}")
        return()
    elseif(arg STREQUAL "--force")
        set(VENDOR_FORCE TRUE)
    elseif(arg STREQUAL "--list")
        set(VENDOR_LIST TRUE)
    elseif(arg STREQUAL "--purge")
        set(VENDOR_PURGE TRUE)
    elseif(arg STREQUAL "--quiet")
        set(VENDOR_QUIET TRUE)
    elseif(arg STREQUAL "--clean")
        set(VENDOR_CLEAN TRUE)
        # Optional value: consume the next argument only if it is not a flag.
        if(i LESS argc)
            list(GET args ${i} peek)
            if(NOT peek MATCHES "^--")
                list(APPEND VENDOR_CLEAN_NAMES "${peek}")
                math(EXPR i "${i} + 1")
            endif()
        endif()
    elseif(arg STREQUAL "--only")
        _vendor_take(value)
        list(APPEND VENDOR_ONLY "${value}")
    elseif(arg STREQUAL "--config")
        _vendor_take(VENDOR_CONFIG)
    elseif(arg STREQUAL "--jobs" OR arg STREQUAL "-j")
        _vendor_take(VENDOR_JOBS)
    elseif(arg STREQUAL "--generator" OR arg STREQUAL "-G")
        _vendor_take(VENDOR_GENERATOR)
    elseif(arg STREQUAL "--c-compiler")
        _vendor_take(VENDOR_CC)
    elseif(arg STREQUAL "--cxx-compiler")
        _vendor_take(VENDOR_CXX)
    elseif(arg STREQUAL "--manifest")
        _vendor_take(VENDOR_MANIFEST)
        get_filename_component(VENDOR_MANIFEST "${VENDOR_MANIFEST}" ABSOLUTE)
    else()
        message("${VENDOR_HELP}")
        vendor_die("unknown flag '${arg}'")
    endif()
endwhile()

# ==============================================================================
# Environment
# ==============================================================================

set(VENDOR_ROOT     "${REPO_ROOT}/.vendor")
set(VENDOR_SRC_DIR  "${VENDOR_ROOT}/src")
set(VENDOR_BUILD_DIR "${VENDOR_ROOT}/build")
set(VENDOR_STAMP_DIR "${VENDOR_ROOT}/stamps")
set(VENDOR_LOG_DIR   "${VENDOR_ROOT}/logs")

if(VENDOR_PURGE)
    if(EXISTS "${VENDOR_ROOT}")
        vendor_say("removing ${VENDOR_ROOT}")
        file(REMOVE_RECURSE "${VENDOR_ROOT}")
    endif()
    vendor_say("purged.")
    return()
endif()

vendor_read_manifest("${VENDOR_MANIFEST}")

if(NOT VENDOR_DEPS)
    vendor_die("${VENDOR_MANIFEST} declares no dependencies")
endif()

# Install prefix: manifest-relative to the repo root unless absolute.
set(install_dir "${VENDOR_SETTINGS_INSTALL_DIR}")
if(install_dir STREQUAL "")
    set(install_dir ".vendor/install")
endif()
if(NOT IS_ABSOLUTE "${install_dir}")
    set(install_dir "${REPO_ROOT}/${install_dir}")
endif()
get_filename_component(VENDOR_INSTALL_DIR "${install_dir}" ABSOLUTE)

set(VENDOR_DEFAULT_CONFIGS "${VENDOR_SETTINGS_CONFIGS}")
if(NOT VENDOR_DEFAULT_CONFIGS)
    set(VENDOR_DEFAULT_CONFIGS "Release")
endif()

# --------------------------------------------------------------- host platform
if(CMAKE_HOST_WIN32)
    set(VENDOR_PLATFORM "windows")
elseif(CMAKE_HOST_APPLE)
    set(VENDOR_PLATFORM "darwin")
else()
    set(VENDOR_PLATFORM "linux")
endif()

# ------------------------------------------------------------------- generator
if(VENDOR_GENERATOR STREQUAL "")
    find_program(VENDOR_NINJA NAMES ninja ninja-build)
    if(VENDOR_NINJA)
        set(VENDOR_GENERATOR "Ninja")
    endif()
endif()

# ------------------------------------------------------------------- compilers
# Normally CMake's own choice is best -- it is what vcpkg effectively used, and
# clang-cl and cl.exe share the MSVC ABI, so dependencies built by either link
# fine into ROSE. The one case worth handling is a Windows box with LLVM but no
# Visual Studio C++ tools, where CMake's default would fail with a confusing
# "no CMAKE_C_COMPILER could be found" from inside a dependency.
if(VENDOR_PLATFORM STREQUAL "windows" AND VENDOR_CC STREQUAL "" AND VENDOR_CXX STREQUAL "")
    find_program(VENDOR_CL cl)
    if(NOT VENDOR_CL)
        find_program(VENDOR_CLANG_CL clang-cl)
        if(VENDOR_CLANG_CL)
            set(VENDOR_CC  "${VENDOR_CLANG_CL}")
            set(VENDOR_CXX "${VENDOR_CLANG_CL}")
        endif()
    endif()
endif()

# ------------------------------------------------------------------------ jobs
if(VENDOR_JOBS STREQUAL "")
    include(ProcessorCount)
    ProcessorCount(VENDOR_JOBS)
    if(VENDOR_JOBS EQUAL 0)
        set(VENDOR_JOBS 1)
    endif()
endif()

# ------------------------------------------------------------------------- git
find_program(VENDOR_GIT git)
if(NOT VENDOR_GIT)
    vendor_die("git was not found on PATH")
endif()

# ==============================================================================
# Resolve each dependency's settings
# ==============================================================================

# Filter out dependencies that do not apply to this host.
set(active_deps "")
foreach(dep IN LISTS VENDOR_DEPS)
    set(platforms "${VENDOR_DEP_${dep}_PLATFORMS}")
    if(platforms AND NOT VENDOR_PLATFORM IN_LIST platforms)
        continue()
    endif()
    list(APPEND active_deps "${dep}")
endforeach()

# Validate, and resolve per-dependency source dirs and argument lists.
foreach(dep IN LISTS active_deps)
    set(repo "${VENDOR_DEP_${dep}_REPO}")
    set(path "${VENDOR_DEP_${dep}_PATH}")

    if(repo AND path)
        vendor_die("${dep}: 'repo' and 'path' are mutually exclusive")
    endif()
    if(NOT repo AND NOT path)
        vendor_die("${dep}: needs either 'repo' + 'tag' or 'path'")
    endif()
    if(repo AND NOT VENDOR_DEP_${dep}_TAG)
        vendor_die("${dep}: 'repo' requires a 'tag'")
    endif()

    if(path)
        if(NOT IS_ABSOLUTE "${path}")
            set(path "${REPO_ROOT}/${path}")
        endif()
        get_filename_component(path "${path}" ABSOLUTE)
        if(NOT IS_DIRECTORY "${path}")
            vendor_die("${dep}: path '${VENDOR_DEP_${dep}_PATH}' does not exist")
        endif()
        set(VENDOR_DEP_${dep}_SRC "${path}")
    else()
        set(VENDOR_DEP_${dep}_SRC "${VENDOR_SRC_DIR}/${dep}")
    endif()

    # A shim replaces the checkout as the CMake source directory, and the
    # checkout is handed to it as VENDOR_SOURCE_DIR.
    set(shim "${VENDOR_DEP_${dep}_SHIM}")
    if(shim)
        if(NOT IS_ABSOLUTE "${shim}")
            set(shim "${REPO_ROOT}/${shim}")
        endif()
        get_filename_component(shim "${shim}" ABSOLUTE)
        if(NOT EXISTS "${shim}/CMakeLists.txt")
            vendor_die("${dep}: shim '${VENDOR_DEP_${dep}_SHIM}' has no CMakeLists.txt")
        endif()
        set(VENDOR_DEP_${dep}_SHIM_ABS "${shim}")
        set(VENDOR_DEP_${dep}_CMAKE_SRC "${shim}")
    elseif(VENDOR_DEP_${dep}_SUBDIR)
        set(VENDOR_DEP_${dep}_CMAKE_SRC
            "${VENDOR_DEP_${dep}_SRC}/${VENDOR_DEP_${dep}_SUBDIR}")
    else()
        set(VENDOR_DEP_${dep}_CMAKE_SRC "${VENDOR_DEP_${dep}_SRC}")
    endif()

    # Configurations: per-dependency override, then --config, then the default.
    set(configs "${VENDOR_DEP_${dep}_CONFIGS}")
    if(NOT configs)
        set(configs "${VENDOR_DEFAULT_CONFIGS}")
    endif()
    # --config narrows the list where it can. A dependency that does not offer
    # the requested configuration -- a header-only Release-only entry, say --
    # keeps its own list rather than being skipped, since consumers still need
    # it installed either way.
    if(VENDOR_CONFIG AND VENDOR_CONFIG IN_LIST configs)
        set(configs "${VENDOR_CONFIG}")
    endif()
    set(VENDOR_DEP_${dep}_RESOLVED_CONFIGS "${configs}")

    # Arguments, in increasing order of specificity so that a dependency can
    # override a project-wide default just by repeating the option:
    # [settings] first, then this dependency's, each followed by its
    # host-specific additions.
    set(dep_args "${VENDOR_SETTINGS_CMAKE_ARGS}")
    if(VENDOR_PLATFORM STREQUAL "windows")
        list(APPEND dep_args ${VENDOR_SETTINGS_CMAKE_ARGS_WINDOWS})
    elseif(VENDOR_PLATFORM STREQUAL "darwin")
        list(APPEND dep_args ${VENDOR_SETTINGS_CMAKE_ARGS_DARWIN})
    else()
        list(APPEND dep_args ${VENDOR_SETTINGS_CMAKE_ARGS_LINUX})
    endif()

    list(APPEND dep_args ${VENDOR_DEP_${dep}_CMAKE_ARGS})
    if(VENDOR_PLATFORM STREQUAL "windows")
        list(APPEND dep_args ${VENDOR_DEP_${dep}_CMAKE_ARGS_WINDOWS})
    elseif(VENDOR_PLATFORM STREQUAL "darwin")
        list(APPEND dep_args ${VENDOR_DEP_${dep}_CMAKE_ARGS_DARWIN})
    else()
        list(APPEND dep_args ${VENDOR_DEP_${dep}_CMAKE_ARGS_LINUX})
    endif()

    set(VENDOR_DEP_${dep}_RESOLVED_ARGS "${dep_args}")

    # Every declared need must exist and be active on this host.
    foreach(need IN LISTS VENDOR_DEP_${dep}_NEEDS)
        if(NOT need IN_LIST VENDOR_DEPS)
            vendor_die("${dep}: needs '${need}', which is not in the manifest")
        endif()
        if(NOT need IN_LIST active_deps)
            vendor_die("${dep}: needs '${need}', which is disabled on ${VENDOR_PLATFORM}")
        endif()
    endforeach()
endforeach()

# ==============================================================================
# Order by `needs`
# ==============================================================================

set(ordered "")
set(visiting "")

# Iterative post-order DFS. CMake has no recursion limit worth relying on and a
# macro cannot recurse, so the stack is explicit: each entry is visited twice,
# once to push its needs and once to emit it.
function(vendor_toposort deps out)
    set(result "")
    foreach(root IN LISTS deps)
        if(root IN_LIST result)
            continue()
        endif()

        set(stack "${root}")
        set(path "")
        while(stack)
            list(GET stack -1 node)

            if(node MATCHES "^!(.*)$")
                # Emit marker: needs are done, so this node can be added.
                set(real "${CMAKE_MATCH_1}")
                list(REMOVE_AT stack -1)
                list(REMOVE_ITEM path "${real}")
                if(NOT real IN_LIST result)
                    list(APPEND result "${real}")
                endif()
                continue()
            endif()

            list(REMOVE_AT stack -1)

            if(node IN_LIST result)
                continue()
            endif()
            if(node IN_LIST path)
                string(REPLACE ";" " -> " cycle "${path};${node}")
                message(FATAL_ERROR "vendor: dependency cycle: ${cycle}")
            endif()

            list(APPEND path "${node}")
            list(APPEND stack "!${node}")
            foreach(need IN LISTS VENDOR_DEP_${node}_NEEDS)
                if(NOT need IN_LIST result)
                    list(APPEND stack "${need}")
                endif()
            endforeach()
        endwhile()
    endforeach()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

vendor_toposort("${active_deps}" ordered)

# --only narrows the set, but keeps whatever those dependencies need ahead of
# them -- building SDL3_image alone against a missing SDL3 would just fail.
if(VENDOR_ONLY)
    foreach(name IN LISTS VENDOR_ONLY)
        if(NOT name IN_LIST VENDOR_DEPS)
            string(REPLACE ";" ", " known "${VENDOR_DEPS}")
            vendor_die("--only '${name}' is not in the manifest.\n  Known: ${known}")
        endif()
        if(NOT name IN_LIST active_deps)
            vendor_die("--only '${name}' is not built on ${VENDOR_PLATFORM}")
        endif()
    endforeach()
    vendor_toposort("${VENDOR_ONLY}" selected)
    set(filtered "")
    foreach(dep IN LISTS ordered)
        if(dep IN_LIST selected)
            list(APPEND filtered "${dep}")
        endif()
    endforeach()
    set(ordered "${filtered}")
endif()

# ==============================================================================
# --clean
# ==============================================================================

if(VENDOR_CLEAN)
    set(targets "${VENDOR_CLEAN_NAMES}")
    if(NOT targets)
        set(targets "${VENDOR_DEPS}")
    endif()
    foreach(dep IN LISTS targets)
        if(NOT dep IN_LIST VENDOR_DEPS)
            vendor_die("--clean '${dep}' is not in the manifest")
        endif()
        file(REMOVE_RECURSE "${VENDOR_BUILD_DIR}/${dep}")
        file(REMOVE
            "${VENDOR_STAMP_DIR}/${dep}.key"
            "${VENDOR_STAMP_DIR}/${dep}.verify"
            "${VENDOR_STAMP_DIR}/${dep}.config"
        )
        vendor_say("cleaned ${dep}")
    endforeach()
    vendor_say("")
    vendor_say("Sources and the install prefix were left alone; the next run rebuilds and overwrites. Use --purge to remove everything.")
    return()
endif()

# ==============================================================================
# The build key
# ==============================================================================

# Everything that could change a dependency's installed output, hashed. If this
# matches the stored stamp and the install is intact, the dependency is skipped.
function(vendor_build_key dep out)
    set(parts "")
    list(APPEND parts "format=${VENDOR_FORMAT_VERSION}")
    list(APPEND parts "name=${dep}")
    list(APPEND parts "repo=${VENDOR_DEP_${dep}_REPO}")
    list(APPEND parts "tag=${VENDOR_DEP_${dep}_TAG}")
    list(APPEND parts "submodules=${VENDOR_DEP_${dep}_SUBMODULES}")
    list(APPEND parts "subdir=${VENDOR_DEP_${dep}_SUBDIR}")
    list(APPEND parts "args=${VENDOR_DEP_${dep}_RESOLVED_ARGS}")
    list(APPEND parts "configs=${VENDOR_DEP_${dep}_RESOLVED_CONFIGS}")
    list(APPEND parts "generator=${VENDOR_GENERATOR}")
    list(APPEND parts "cc=${VENDOR_CC}")
    list(APPEND parts "cxx=${VENDOR_CXX}")
    list(APPEND parts "prefix=${VENDOR_INSTALL_DIR}")
    list(APPEND parts "platform=${VENDOR_PLATFORM}")

    # In-tree sources and shims are inputs like any other, so hash their content.
    if(VENDOR_DEP_${dep}_PATH)
        vendor_hash_tree("${VENDOR_DEP_${dep}_SRC}" tree)
        list(APPEND parts "tree=${tree}")
    endif()
    if(VENDOR_DEP_${dep}_SHIM_ABS)
        vendor_hash_tree("${VENDOR_DEP_${dep}_SHIM_ABS}" shim_tree)
        list(APPEND parts "shim=${shim_tree}")
    endif()

    string(REPLACE ";" "\n" text "${parts}")
    string(SHA256 key "${text}")
    set(${out} "${key}" PARENT_SCOPE)
endfunction()

# A dependency is up to date when its key matches and the file recorded at
# install time is still there -- the cheap way to notice a wiped prefix.
# The subset of the build key that a CMake cache cannot be talked out of.
#
# Reconfiguring an existing build tree does NOT drop cache entries that are no
# longer passed on the command line -- removing `-DCMAKE_CXX_FLAGS=...` from the
# manifest leaves the old value in CMakeCache.txt and the change silently does
# nothing. So when any of this differs from last time, the build tree is thrown
# away rather than reconfigured.
#
# Kept separate from the full build key so that a dependency whose *source*
# changed (a new tag, an edited shim) still reconfigures in place and rebuilds
# incrementally -- and so an interrupted build resumes instead of starting over.
function(vendor_config_key dep out)
    set(parts "")
    list(APPEND parts "format=${VENDOR_FORMAT_VERSION}")
    list(APPEND parts "args=${VENDOR_DEP_${dep}_RESOLVED_ARGS}")
    list(APPEND parts "configs=${VENDOR_DEP_${dep}_RESOLVED_CONFIGS}")
    list(APPEND parts "generator=${VENDOR_GENERATOR}")
    list(APPEND parts "cc=${VENDOR_CC}")
    list(APPEND parts "cxx=${VENDOR_CXX}")
    list(APPEND parts "prefix=${VENDOR_INSTALL_DIR}")
    list(APPEND parts "src=${VENDOR_DEP_${dep}_CMAKE_SRC}")

    string(REPLACE ";" "\n" text "${parts}")
    string(SHA256 key "${text}")
    set(${out} "${key}" PARENT_SCOPE)
endfunction()

function(vendor_is_current dep key out reason)
    vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.key" stored)
    if(NOT stored STREQUAL key)
        if(stored STREQUAL "")
            set(${reason} "not built yet" PARENT_SCOPE)
        else()
            set(${reason} "manifest or toolchain changed" PARENT_SCOPE)
        endif()
        set(${out} FALSE PARENT_SCOPE)
        return()
    endif()

    vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.verify" verify)
    if(verify AND NOT EXISTS "${verify}")
        set(${reason} "install prefix is missing files" PARENT_SCOPE)
        set(${out} FALSE PARENT_SCOPE)
        return()
    endif()

    set(${reason} "" PARENT_SCOPE)
    set(${out} TRUE PARENT_SCOPE)
endfunction()

# ==============================================================================
# --list
# ==============================================================================

if(VENDOR_LIST)
    vendor_say("manifest : ${VENDOR_MANIFEST}")
    vendor_say("install  : ${VENDOR_INSTALL_DIR}")
    vendor_say("configs  : ${VENDOR_DEFAULT_CONFIGS}")
    vendor_say("platform : ${VENDOR_PLATFORM}")
    vendor_say("")

    foreach(dep IN LISTS ordered)
        vendor_build_key("${dep}" key)
        vendor_is_current("${dep}" "${key}" current reason)

        if(VENDOR_DEP_${dep}_PATH)
            set(source "in-tree ${VENDOR_DEP_${dep}_PATH}")
        else()
            set(source "${VENDOR_DEP_${dep}_TAG}")
        endif()

        if(current)
            set(state "up to date")
        else()
            set(state "STALE -- ${reason}")
        endif()

        string(LENGTH "${dep}" n)
        math(EXPR pad "16 - ${n}")
        set(spaces "")
        if(pad GREATER 0)
            string(REPEAT " " ${pad} spaces)
        endif()
        vendor_say("${dep}${spaces}${source}")
        vendor_say("                ${state}")

        vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.sha" sha)
        if(sha)
            string(SUBSTRING "${sha}" 0 12 short)
            vendor_say("                checkout ${short}")
        endif()
    endforeach()
    return()
endif()

# ==============================================================================
# Fetch
# ==============================================================================

# Initialises a dependency's submodules.
#
# `submodules = true` takes every one of them, which for the SDL satellites means
# gigabytes of codecs that are switched off anyway -- SDL_image alone carries
# libjxl, libavif, dav1d and aom. Listing the paths instead keeps a checkout to
# what the enabled features actually need, and doubles as documentation of each
# dependency's real subdependency set.
function(vendor_update_submodules dep src log)
    set(value "${VENDOR_DEP_${dep}_SUBMODULES}")

    if(value STREQUAL "" OR value STREQUAL "OFF")
        return()
    endif()

    set(paths "")
    if(NOT value STREQUAL "ON")
        set(paths "${value}")
        foreach(p IN LISTS paths)
            if(NOT EXISTS "${src}/${p}")
                vendor_die("${dep}: submodule path '${p}' is not in the checkout")
            endif()
        endforeach()
    endif()

    set(base submodule update --init --recursive)

    # Shallow first. Some upstreams pin a submodule commit that is not the tip of
    # any branch, and --depth 1 cannot reach it; a full clone of just that
    # submodule is the fallback.
    vendor_run(
        COMMAND "${VENDOR_GIT}" ${base} --depth 1 --force -- ${paths}
        WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)

    if(NOT rc EQUAL 0)
        vendor_say("${dep}: shallow submodule fetch failed, retrying in full")
        vendor_run(
            COMMAND "${VENDOR_GIT}" ${base} --force -- ${paths}
            WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)
        if(NOT rc EQUAL 0)
            vendor_die("${dep}: submodule update failed")
        endif()
    endif()

    vendor_prune_submodules("${dep}" "${src}" "${log}" "${paths}")
endfunction()

# Deinitialises submodules that are checked out but no longer listed.
#
# Dropping a path from `submodules` has to actually remove it, not just stop
# refreshing it. A stale checkout is invisible to the build -- the feature that
# used it is off, so nothing compiles it -- but it is very visible to
# vendor_collect_licenses(), which reads what is on disk precisely so that a
# notice ships only for code that shipped. Left alone, removing mpg123 from the
# manifest would keep crediting an LGPL library ROSE no longer contains, which
# is worse than a merely stale checkout: it is a false statement about what is
# in the binary. Pruning keeps "initialised" and "compiled" the same set.
function(vendor_prune_submodules dep src log keep)
    # `submodules = true` keeps everything by definition; nothing to prune.
    if(keep STREQUAL "")
        return()
    endif()

    vendor_submodule_paths("${src}" present TOP)

    set(stale "")
    foreach(p IN LISTS present)
        if(NOT p IN_LIST keep)
            list(APPEND stale "${p}")
        endif()
    endforeach()

    if(stale STREQUAL "")
        return()
    endif()

    string(REPLACE ";" ", " pretty "${stale}")
    vendor_say("${dep}: dropping unlisted submodule(s): ${pretty}")

    # --force because a build may have left objects inside the working tree;
    # these are vendored checkouts, so there is nothing of ours to lose.
    vendor_run(
        COMMAND "${VENDOR_GIT}" submodule deinit --force -- ${stale}
        WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)

    if(NOT rc EQUAL 0)
        vendor_die("${dep}: could not deinitialise ${pretty}")
    endif()
endfunction()

# Brings .vendor/src/<dep> to the pinned tag, doing nothing when it is already
# there. The recorded tag is what makes the no-op case free: no git process runs
# at all, so a clean re-run never touches the network.
function(vendor_fetch dep out_changed)
    set(${out_changed} FALSE PARENT_SCOPE)

    if(VENDOR_DEP_${dep}_PATH)
        return()   # in-tree source, nothing to fetch
    endif()

    set(src "${VENDOR_DEP_${dep}_SRC}")
    set(repo "${VENDOR_DEP_${dep}_REPO}")
    set(tag "${VENDOR_DEP_${dep}_TAG}")
    set(log "${VENDOR_LOG_DIR}/${dep}-fetch.log")

    vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.tag" have_tag)
    vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.repo" have_repo)

    # ------------------------------------------------------------ first clone
    # Submodules are deliberately not part of the clone: --recurse-submodules
    # takes all of them, and vendor_update_submodules() takes only the ones the
    # manifest asks for.
    if(NOT IS_DIRECTORY "${src}/.git")
        file(REMOVE_RECURSE "${src}")
        file(MAKE_DIRECTORY "${VENDOR_SRC_DIR}")
        vendor_say("${dep}: cloning ${tag}")
        vendor_run(
            COMMAND "${VENDOR_GIT}" clone --depth 1 --branch "${tag}"
                    "${repo}" "${src}"
            LOG "${log}" RESULT rc)
        if(NOT rc EQUAL 0)
            vendor_die("${dep}: git clone failed (${rc})")
        endif()
        vendor_update_submodules("${dep}" "${src}" "${log}")
    elseif(have_tag STREQUAL tag AND have_repo STREQUAL repo)
        # Already at the right tag. Submodules still get a pass, because adding
        # a path to `submodules` in the manifest does not change the tag -- the
        # checkout would otherwise keep an empty directory where the newly
        # requested source should be. Only reached for a dependency that is
        # being rebuilt anyway, so this is never on the no-op path.
        vendor_update_submodules("${dep}" "${src}" "${log}")
        return()
    else()
        # ------------------------------------------------------ move to a tag
        if(have_tag)
            vendor_say("${dep}: ${have_tag} -> ${tag}")
        else()
            vendor_say("${dep}: checking out ${tag}")
        endif()

        vendor_run(COMMAND "${VENDOR_GIT}" remote set-url origin "${repo}"
                   WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)

        vendor_run(
            COMMAND "${VENDOR_GIT}" fetch --depth 1 --force origin
                    "refs/tags/${tag}:refs/tags/${tag}"
            WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)
        if(NOT rc EQUAL 0)
            # Not a tag: fall back to fetching it as a branch or bare ref, which
            # keeps a manifest usable with a moving ref during development.
            vendor_run(
                COMMAND "${VENDOR_GIT}" fetch --depth 1 --force origin "${tag}"
                WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)
            if(NOT rc EQUAL 0)
                vendor_die("${dep}: cannot fetch '${tag}' from ${repo}")
            endif()
            set(tag "FETCH_HEAD")
        endif()

        vendor_run(COMMAND "${VENDOR_GIT}" checkout --detach --force "${tag}"
                   WORKING_DIRECTORY "${src}" LOG "${log}" RESULT rc)
        if(NOT rc EQUAL 0)
            vendor_die("${dep}: cannot check out '${tag}'")
        endif()

        vendor_update_submodules("${dep}" "${src}" "${log}")
    endif()

    # Record where we landed. The tag file is the fast path on the next run; the
    # sha is for --list.
    execute_process(
        COMMAND "${VENDOR_GIT}" rev-parse HEAD
        WORKING_DIRECTORY "${src}"
        OUTPUT_VARIABLE sha OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    file(MAKE_DIRECTORY "${VENDOR_STAMP_DIR}")
    file(WRITE "${VENDOR_STAMP_DIR}/${dep}.tag"  "${VENDOR_DEP_${dep}_TAG}")
    file(WRITE "${VENDOR_STAMP_DIR}/${dep}.repo" "${repo}")
    file(WRITE "${VENDOR_STAMP_DIR}/${dep}.sha"  "${sha}")

    set(${out_changed} TRUE PARENT_SCOPE)
endfunction()

# ==============================================================================
# Build
# ==============================================================================

function(vendor_build dep)
    set(src "${VENDOR_DEP_${dep}_CMAKE_SRC}")

    if(NOT EXISTS "${src}/CMakeLists.txt")
        vendor_die("${dep}: no CMakeLists.txt in ${src}")
    endif()

    # Start this dependency's notices from nothing. Two things write in here --
    # the upstream's own install rules and vendor_collect_licenses() -- and
    # neither removes anything, so a codec switched off in the manifest would
    # otherwise leave its license behind forever. Turning mpg123 off is exactly
    # that case: SDL_mixer stops installing LICENSE.mpg123.txt and the submodule
    # gets deinitialised, but both had already been copied here. A stale .lib is
    # merely wasted bytes; a stale license is a claim that code is in the binary
    # when it is not. Only ever reached on a rebuild, which is the only thing
    # that can change what a dependency contains.
    file(REMOVE_RECURSE "${VENDOR_INSTALL_DIR}/share/licenses/${dep}")

    # Discard the build tree when the configuration changed, because a stale
    # CMakeCache.txt would otherwise keep options the manifest no longer asks
    # for. See vendor_config_key().
    vendor_config_key("${dep}" config_key)
    vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.config" stored_config)

    if(VENDOR_FORCE OR NOT stored_config STREQUAL config_key)
        if(stored_config AND EXISTS "${VENDOR_BUILD_DIR}/${dep}")
            vendor_say("${dep}: configuration changed, discarding build tree")
        endif()
        file(REMOVE_RECURSE "${VENDOR_BUILD_DIR}/${dep}")
        file(REMOVE "${VENDOR_STAMP_DIR}/${dep}.config")
    endif()

    foreach(config IN LISTS VENDOR_DEP_${dep}_RESOLVED_CONFIGS)
        set(build "${VENDOR_BUILD_DIR}/${dep}/${config}")
        # One log per phase: a single file would be truncated by the next step,
        # losing exactly the configure output needed to explain a build failure.
        set(log_base "${VENDOR_LOG_DIR}/${dep}-${config}")

        set(cfg_args
            "-DCMAKE_BUILD_TYPE=${config}"
            "-DCMAKE_INSTALL_PREFIX=${VENDOR_INSTALL_DIR}"
            "-DCMAKE_PREFIX_PATH=${VENDOR_INSTALL_DIR}"
            "-DBUILD_SHARED_LIBS=ON"
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
            # Release and Debug share one prefix, so the debug artifacts need
            # distinct names. SDL already defaults to this; forcing it makes
            # every dependency agree.
            "-DCMAKE_DEBUG_POSTFIX=d"
        )

        if(VENDOR_PLATFORM STREQUAL "windows")
            # The dynamic CRT is not negotiable: ROSE_Core is a DLL and the
            # engine allocates across that boundary through header-inline
            # UniquePtr / BasicString::allocate, so every module must share one
            # heap. CMP0091 has to be NEW for this to be honoured at all, and
            # some dependencies still declare an older policy default.
            list(APPEND cfg_args
                "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW"
                "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
            )
        endif()

        if(VENDOR_GENERATOR)
            list(APPEND cfg_args "-G" "${VENDOR_GENERATOR}")
        endif()
        if(VENDOR_CC)
            list(APPEND cfg_args "-DCMAKE_C_COMPILER=${VENDOR_CC}")
        endif()
        if(VENDOR_CXX)
            list(APPEND cfg_args "-DCMAKE_CXX_COMPILER=${VENDOR_CXX}")
        endif()
        if(VENDOR_DEP_${dep}_SHIM_ABS)
            list(APPEND cfg_args "-DVENDOR_SOURCE_DIR=${VENDOR_DEP_${dep}_SRC}")
        endif()

        list(APPEND cfg_args ${VENDOR_DEP_${dep}_RESOLVED_ARGS})

        vendor_say("${dep}: configuring ${config}")
        vendor_run(
            COMMAND "${CMAKE_COMMAND}" -S "${src}" -B "${build}" ${cfg_args}
            LOG "${log_base}-configure.log" RESULT rc)
        if(NOT rc EQUAL 0)
            vendor_report_failure("${dep}" "${config}" "configure"
                                  "${log_base}-configure.log")
        endif()

        vendor_say("${dep}: building ${config}")
        vendor_run(
            COMMAND "${CMAKE_COMMAND}" --build "${build}"
                    --config "${config}" --parallel "${VENDOR_JOBS}"
            LOG "${log_base}-build.log" RESULT rc)
        if(NOT rc EQUAL 0)
            vendor_report_failure("${dep}" "${config}" "build"
                                  "${log_base}-build.log")
        endif()

        vendor_run(
            COMMAND "${CMAKE_COMMAND}" --install "${build}" --config "${config}"
            LOG "${log_base}-install.log" RESULT rc)
        if(NOT rc EQUAL 0)
            vendor_report_failure("${dep}" "${config}" "install"
                                  "${log_base}-install.log")
        endif()

        # Remember one installed file so a wiped prefix is detected next run.
        if(EXISTS "${build}/install_manifest.txt")
            file(READ "${build}/install_manifest.txt" manifest)
            vendor_split_lines("${manifest}" installed)
            list(REMOVE_ITEM installed "")
            list(LENGTH installed n)
            if(n GREATER 0)
                list(GET installed -1 witness)
                file(MAKE_DIRECTORY "${VENDOR_STAMP_DIR}")
                file(WRITE "${VENDOR_STAMP_DIR}/${dep}.verify" "${witness}")
            endif()
        endif()
    endforeach()

    file(MAKE_DIRECTORY "${VENDOR_STAMP_DIR}")
    file(WRITE "${VENDOR_STAMP_DIR}/${dep}.config" "${config_key}")
endfunction()

function(vendor_report_failure dep config phase log)
    message("")
    message("---------------------------------------------------------------")
    message("vendor: ${dep} failed to ${phase} (${config})")
    if(EXISTS "${log}")
        file(READ "${log}" text)
        vendor_split_lines("${text}" lines)

        # Pull out the diagnostics rather than tailing the file. Third-party
        # builds emit thousands of warnings, and a plain tail reliably shows the
        # last harmless -Wunsafe-buffer-usage instead of the actual error.
        # Matching the shape of a diagnostic, not the bare word: identifiers
        # like `total_error_0` appear constantly in codec sources, so "error"
        # has to be followed by a colon or a space and preceded by neither a
        # letter nor an underscore.
        set(errors "")
        set(trailing 0)
        foreach(line IN LISTS lines)
            if(line MATCHES "^FAILED:"
               OR line MATCHES "^ninja: (error|fatal)"
               OR line MATCHES "^error[: ]"
               OR line MATCHES "[^A-Za-z_]error[: ]"
               OR line MATCHES "CMake Error"
               OR line MATCHES "undefined (reference|symbol)"
               OR line MATCHES "LNK[0-9][0-9][0-9][0-9]")
                list(APPEND errors "${line}")
                # A CMake Error puts its explanation on the following lines, so
                # carry a few along; compiler diagnostics are self-contained on
                # one line and set this to zero.
                if(line MATCHES "CMake Error")
                    set(trailing 4)
                else()
                    set(trailing 0)
                endif()
            elseif(trailing GREATER 0 AND NOT line STREQUAL "")
                list(APPEND errors "${line}")
                math(EXPR trailing "${trailing} - 1")
            endif()
        endforeach()

        list(REMOVE_DUPLICATES errors)
        list(LENGTH errors n)

        if(n GREATER 0)
            message("")
            if(n GREATER 25)
                message("first 25 of ${n} diagnostics:")
                list(SUBLIST errors 0 25 errors)
            else()
                message("diagnostics:")
            endif()
            message("")
            foreach(line IN LISTS errors)
                message("  ${line}")
            endforeach()
        endif()

        message("")
        message("full log: ${log}")
    endif()
    message("---------------------------------------------------------------")
    message("Fix the cause and re-run; everything already built is skipped.")
    message(FATAL_ERROR "vendor: stopping at ${dep}.")
endfunction()

# ==============================================================================
# Licenses
# ==============================================================================
#
# Every dependency in the manifest is someone else's work under someone else's
# terms, and all of those terms require the notice to travel with the binaries.
# The checkouts under .vendor/src are build scratch and are not shipped, so the
# notices are copied into the install prefix instead, beside the libraries they
# cover:
#
#     <prefix>/share/licenses/<dep>/PACKAGE.txt     provenance: repo, tag, sha
#     <prefix>/share/licenses/<dep>/LICENSE...      the dependency's own
#     <prefix>/share/licenses/<dep>/<submodule>/    each vendored submodule's
#
# rose_deploy_licenses() in the root CMakeLists copies that tree next to every
# executable, the same job rose_deploy_dlls() does for the DLLs.
#
# This runs for skipped dependencies too, not just rebuilt ones: file(COPY) is a
# no-op when the destination is already current, and a prefix that was populated
# before this existed would otherwise never grow the notices.

# Files whose name marks them as license or attribution text. Prefix-matched,
# case-insensitively, which is what catches LICENSE.txt, COPYING.LGPL and
# LICENSE.MIT alike. AUTHORS is here because several of these licenses -- mpg123
# and harfbuzz among them -- point at it for the attribution list.
set(VENDOR_LICENSE_PATTERNS "LICEN[CS]E" "COPYING" "COPYRIGHT" "NOTICE" "AUTHORS")

# Copies the license-looking entries at the top level of <dir> into <dest>, and
# reports how many were found. Deliberately not recursive: every project in the
# manifest keeps its terms at the root of its tree, and a recursive sweep of, say,
# freetype would collect a few hundred per-file headers instead.
function(vendor_copy_license_files dir dest out_count)
    set(count 0)

    if(NOT IS_DIRECTORY "${dir}")
        set(${out_count} 0 PARENT_SCOPE)
        return()
    endif()

    file(GLOB entries RELATIVE "${dir}" "${dir}/*")
    foreach(entry IN LISTS entries)
        string(TOUPPER "${entry}" upper)

        set(matched FALSE)
        foreach(pattern IN LISTS VENDOR_LICENSE_PATTERNS)
            if(upper MATCHES "^${pattern}")
                set(matched TRUE)
                break()
            endif()
        endforeach()

        if(matched)
            # Handles nlohmann_json's LICENSES/ directory as well as plain
            # files; file(COPY) takes either.
            file(COPY "${dir}/${entry}" DESTINATION "${dest}")
            math(EXPR count "${count} + 1")
        endif()
    endforeach()

    set(${out_count} "${count}" PARENT_SCOPE)
endfunction()

# Every initialised submodule path of a checkout. Recursive unless a third
# argument of TOP asks for only the top-level ones.
#
# The manifest's `submodules` list is not used for this: it may be `true`, it
# says nothing about nested submodules, and it describes what was asked for
# rather than what is on disk. `git submodule status` prefixes an uninitialised
# submodule with '-', which is exactly the set to skip -- dependencies.toml
# leaves most of the SDL satellites' codecs uninitialised on purpose, and their
# licenses have no business in a build that never compiled them.
function(vendor_submodule_paths src out)
    set(paths "")

    set(depth --recursive)
    if("TOP" IN_LIST ARGN)
        set(depth "")
    endif()

    if(EXISTS "${src}/.git")
        execute_process(
            COMMAND "${VENDOR_GIT}" submodule status ${depth}
            WORKING_DIRECTORY "${src}"
            OUTPUT_VARIABLE status_text
            ERROR_QUIET
            RESULT_VARIABLE rc
        )
        if(rc EQUAL 0)
            vendor_split_lines("${status_text}" lines)
            foreach(line IN LISTS lines)
                # " <sha> <path> (<describe>)" -- present.  '+' is a checkout
                # that differs from the recorded sha, 'U' a merge conflict;
                # both still have files worth reading. '-' is not initialised.
                if(line MATCHES "^[ +U][0-9a-f]+ ([^ ]+)")
                    list(APPEND paths "${CMAKE_MATCH_1}")
                endif()
            endforeach()
        endif()
    endif()

    set(${out} "${paths}" PARENT_SCOPE)
endfunction()

# Removes bundle directories this pass no longer produces.
#
# vendor_build() clears a dependency's whole license directory, but only when it
# rebuilds, and `license_paths` is deliberately not part of the build key -- it
# describes what to attribute, not what to compile, and no one should pay a
# two-minute rebuild for editing a comment about copyright. So dropping a path
# from the manifest has to be cleaned up here instead.
#
# The previous run's PACKAGE.txt is the record of what was written: its
# `bundled:` line is this same list, and anything on it that is missing now is a
# directory this pass did not write and no longer stands behind.
function(vendor_drop_stale_bundles dest keep)
    vendor_read_file("${dest}/PACKAGE.txt" previous)

    if(NOT previous MATCHES "bundled:  ([^\n]+)")
        return()
    endif()

    string(REPLACE ", " ";" was "${CMAKE_MATCH_1}")

    foreach(leaf IN LISTS was)
        if(NOT leaf IN_LIST keep AND IS_DIRECTORY "${dest}/${leaf}")
            file(REMOVE_RECURSE "${dest}/${leaf}")
        endif()
    endforeach()
endfunction()

function(vendor_collect_licenses dep)
    set(src "${VENDOR_DEP_${dep}_SRC}")
    set(dest "${VENDOR_INSTALL_DIR}/share/licenses/${dep}")
    set(total 0)
    set(bundled "")

    if(NOT IS_DIRECTORY "${src}")
        return()
    endif()

    vendor_copy_license_files("${src}" "${dest}" found)
    math(EXPR total "${total} + ${found}")

    # A vendored submodule is compiled into the dependency's binaries, so its
    # notice ships with them. Flattened to the submodule's own name: the
    # external/ prefix every SDL satellite uses says nothing a reader needs.
    vendor_submodule_paths("${src}" submodules)
    foreach(rel IN LISTS submodules)
        get_filename_component(leaf "${rel}" NAME)
        vendor_copy_license_files("${src}/${rel}" "${dest}/${leaf}" found)
        if(found GREATER 0)
            list(APPEND bundled "${leaf}")
            math(EXPR total "${total} + ${found}")
        else()
            vendor_say("${dep}: warning: no license file in submodule ${rel}")
        endif()
    endforeach()

    # The escape hatch, for third-party code a dependency carries in its own
    # tree rather than as a submodule -- SDL's copy of hidapi, SDL_mixer's
    # dr_libs. The sweep above cannot find those, because it deliberately does
    # not recurse, and a recursive one would drown in per-file headers.
    #
    # A directory entry contributes the license files at its top level; a file
    # entry is taken as named. Either way the result lands in a subdirectory
    # named after the code it covers, since half of these files are called
    # LICENSE.txt and a flat copy would have them overwrite each other -- and
    # the dependency's own.
    foreach(rel IN LISTS VENDOR_DEP_${dep}_LICENSE_PATHS)
        if(NOT EXISTS "${src}/${rel}")
            vendor_die("${dep}: license_paths entry '${rel}' is not in ${src}")
        endif()

        if(IS_DIRECTORY "${src}/${rel}")
            get_filename_component(leaf "${rel}" NAME)
            vendor_copy_license_files("${src}/${rel}" "${dest}/${leaf}" found)
            if(found EQUAL 0)
                vendor_die("${dep}: license_paths entry '${rel}' holds no license file")
            endif()
        else()
            get_filename_component(parent "${rel}" DIRECTORY)
            get_filename_component(leaf "${parent}" NAME)
            file(COPY "${src}/${rel}" DESTINATION "${dest}/${leaf}")
            set(found 1)
        endif()

        list(APPEND bundled "${leaf}")
        math(EXPR total "${total} + ${found}")
    endforeach()

    # plutovg is a submodule of SDL_ttf and of plutosvg both, and --recursive
    # reports it once for each.
    list(REMOVE_DUPLICATES bundled)

    vendor_drop_stale_bundles("${dest}" "${bundled}")

    if(total EQUAL 0)
        vendor_say("${dep}: warning: no license file found in ${src}")
        vendor_say("${dep}: name one in `license_paths` in the manifest -- "
                   "nothing should ship unattributed")
        return()
    endif()

    # Provenance beside the text, so the notice says which version it covers.
    set(package "${dep}\n")
    if(VENDOR_DEP_${dep}_DESCRIPTION)
        string(APPEND package "${VENDOR_DEP_${dep}_DESCRIPTION}\n")
    endif()
    string(APPEND package "\n")
    if(VENDOR_DEP_${dep}_REPO)
        string(APPEND package "source:   ${VENDOR_DEP_${dep}_REPO}\n")
        string(APPEND package "tag:      ${VENDOR_DEP_${dep}_TAG}\n")
        vendor_read_file("${VENDOR_STAMP_DIR}/${dep}.sha" sha)
        if(sha)
            string(APPEND package "commit:   ${sha}\n")
        endif()
    else()
        string(APPEND package "source:   in-tree, ${VENDOR_DEP_${dep}_PATH}\n")
    endif()
    if(bundled)
        string(REPLACE ";" ", " bundled_text "${bundled}")
        string(APPEND package "bundled:  ${bundled_text}\n")
    endif()
    string(APPEND package
        "\nThe files beside this one are ${dep}'s own license and attribution "
        "text,\ncopied verbatim from the source above.\n")
    if(bundled)
        string(APPEND package
            "Each subdirectory holds the same for third-party code ${dep} carries,\n"
            "as a submodule or in its own tree.\n")
    endif()

    file(WRITE "${dest}/PACKAGE.txt" "${package}")
endfunction()

# ==============================================================================
# Main
# ==============================================================================

string(TIMESTAMP started "%s")

file(MAKE_DIRECTORY "${VENDOR_SRC_DIR}" "${VENDOR_BUILD_DIR}" "${VENDOR_STAMP_DIR}")

list(LENGTH ordered total)
vendor_say("vendoring ${total} dependencies into ${VENDOR_INSTALL_DIR}")
if(VENDOR_GENERATOR)
    vendor_say("generator: ${VENDOR_GENERATOR}, jobs: ${VENDOR_JOBS}")
endif()
vendor_say("")

set(built 0)
set(skipped 0)

foreach(dep IN LISTS ordered)
    vendor_build_key("${dep}" key)
    vendor_is_current("${dep}" "${key}" current reason)

    if(current AND NOT VENDOR_FORCE)
        vendor_say("${dep}: up to date")
        math(EXPR skipped "${skipped} + 1")
        continue()
    endif()

    if(VENDOR_FORCE)
        vendor_say("${dep}: rebuilding (--force)")
    else()
        vendor_say("${dep}: ${reason}")
    endif()

    vendor_fetch("${dep}" fetched)

    # The tree hash of an in-tree dependency is only knowable after any fetch,
    # so recompute the key before storing it.
    vendor_build_key("${dep}" key)

    vendor_build("${dep}")

    file(WRITE "${VENDOR_STAMP_DIR}/${dep}.key" "${key}")
    math(EXPR built "${built} + 1")
    vendor_say("")
endforeach()

# Attribution for everything that was just built, and for everything that was
# already up to date. A separate pass rather than a step inside the loop, so that
# there is one call site and no way for the skip path to miss it.
foreach(dep IN LISTS ordered)
    vendor_collect_licenses("${dep}")
endforeach()

file(WRITE "${VENDOR_INSTALL_DIR}/share/licenses/README.txt"
"Third-party licenses
====================

One directory per dependency, holding that project's own license and
attribution text copied verbatim from the source it was built from, plus a
PACKAGE.txt naming the repository and the exact commit. A nested directory is a
library that dependency vendors and compiles into its own binaries.

Collected by cmake/vendor.cmake from the manifest in dependencies.toml. See
THIRD_PARTY_NOTICES.md in the ROSE repository for the summary, and LICENSE
there for ROSE's own terms.
")

string(TIMESTAMP finished "%s")
math(EXPR elapsed "${finished} - ${started}")
math(EXPR minutes "${elapsed} / 60")
math(EXPR seconds "${elapsed} % 60")

vendor_say("")
vendor_say("done: ${built} built, ${skipped} up to date, ${minutes}m${seconds}s")
if(built GREATER 0)
    vendor_say("prefix: ${VENDOR_INSTALL_DIR}")
endif()
