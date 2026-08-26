# ==============================================================================
# A reader for the restricted TOML subset used by dependencies.toml
# ==============================================================================
#
# Neither CMake, PowerShell nor sh ships a TOML parser, so this hand-parses a
# deliberately small subset:
#
#     # comment                     -- runs to end of line, honours quoting
#     [settings]                    -- one settings table
#     [[dependency]]                -- repeated; each starts a new record
#     key = "string"
#     key = true | false            -- becomes ON / OFF
#     key = ["a", "b"]              -- may span lines, trailing comma allowed
#
# Anything else is a hard error naming the file and line. The strictness is the
# point: a mistyped key that is silently ignored becomes a confusing build
# failure ten minutes later, while a parse error is obvious immediately.
#
# Two constraints the real TOML spec does not impose:
#
#   * `name` must be the first key in a [[dependency]] block. It is what every
#     other key is filed under, so accepting it late would mean buffering every
#     record; requiring it first keeps this parser a single forward pass.
#   * Semicolons are rejected outright. CMake has no string that survives one
#     unambiguously -- `;` is simultaneously a literal and the list separator --
#     and nothing in the manifest needs one.
#
# ------------------------------------------------------------------ interface
#
#   vendor_read_manifest(<path>)
#
# sets, in the caller's scope:
#
#   VENDOR_DEPS                     list of dependency names, declaration order
#   VENDOR_DEP_<name>_<KEY>         one variable per key, KEY upper-cased
#   VENDOR_SETTINGS_<KEY>           one variable per [settings] key
#
# <name> is used verbatim rather than upper-cased, so SDL3_image and SDL3_IMAGE
# could never collide.

# Keys accepted on a [[dependency]]. Extending the manifest means adding here
# first -- an unlisted key is a parse error.
set(VENDOR_DEP_KEYS
    name            # identifier; also the .vendor/src and stamp file name
    description     # free text, shown by --list
    repo            # git remote; mutually exclusive with `path`
    tag             # git tag (or any ref); required alongside `repo`
    path            # in-repo source directory; mutually exclusive with `repo`
    find            # the find_package() name this dependency provides
    shim            # source dir supplying a CMakeLists for a dep that has none
    subdir          # build this subdirectory of the checkout instead of its root
    needs           # dependency names that must be built first
    submodules      # true for every submodule, or a list of paths to init
    configs         # override the global config list for this dependency
    cmake_args      # passed to every configure of this dependency
    cmake_args_windows
    cmake_args_linux
    cmake_args_darwin
    platforms       # restrict to some of windows / linux / darwin
    license_paths   # extra files/dirs of license text the name sweep misses
)

set(VENDOR_SETTINGS_KEYS
    install_dir     # where every dependency is installed, relative to the repo
    configs         # default build configurations
    cmake_args      # prepended to every dependency's own cmake_args
    cmake_args_windows
    cmake_args_linux
    cmake_args_darwin
)

# ------------------------------------------------------------------ internals

# Strips a trailing `# comment`, leaving a `#` inside a quoted string alone.
function(_vendor_strip_comment line out)
    string(FIND "${line}" "#" hash)
    if(hash LESS 0)
        set(${out} "${line}" PARENT_SCOPE)
        return()
    endif()

    string(FIND "${line}" "\"" quote)
    if(quote LESS 0 OR hash LESS quote)
        # No quotes at all, or the # precedes them: plain truncation is correct.
        string(SUBSTRING "${line}" 0 ${hash} result)
        set(${out} "${result}" PARENT_SCOPE)
        return()
    endif()

    # Both present with the quote first: walk the line tracking quote state.
    string(LENGTH "${line}" len)
    set(pos 0)
    set(in_quote 0)
    set(acc "")
    while(pos LESS len)
        string(SUBSTRING "${line}" ${pos} 1 ch)
        if(ch STREQUAL "\"")
            if(in_quote)
                set(in_quote 0)
            else()
                set(in_quote 1)
            endif()
        elseif(ch STREQUAL "#" AND in_quote EQUAL 0)
            break()
        endif()
        set(acc "${acc}${ch}")
        math(EXPR pos "${pos} + 1")
    endwhile()
    set(${out} "${acc}" PARENT_SCOPE)
endfunction()

# Converts one TOML scalar to a CMake value. `ok` comes back FALSE for anything
# outside the subset, so the caller can report a line number.
function(_vendor_parse_scalar raw out ok)
    string(STRIP "${raw}" raw)

    if(raw MATCHES "^\"(.*)\"$")
        set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${ok} TRUE PARENT_SCOPE)
    elseif(raw STREQUAL "true")
        set(${out} "ON" PARENT_SCOPE)
        set(${ok} TRUE PARENT_SCOPE)
    elseif(raw STREQUAL "false")
        set(${out} "OFF" PARENT_SCOPE)
        set(${ok} TRUE PARENT_SCOPE)
    else()
        set(${out} "" PARENT_SCOPE)
        set(${ok} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Parses the inside of an array -- `"a", "b",` -- appending to the list in `out`.
function(_vendor_parse_array_body body out ok)
    set(items "${${out}}")
    set(${ok} TRUE PARENT_SCOPE)

    string(STRIP "${body}" body)
    if(body STREQUAL "")
        set(${out} "${items}" PARENT_SCOPE)
        return()
    endif()

    # Trailing commas are legal TOML and common in hand-edited lists.
    string(REGEX REPLACE ",[ \t]*$" "" body "${body}")

    # Split on commas sitting between quotes. Every element in this subset is a
    # quoted string, so `", "` is an unambiguous separator. A control character
    # stands in for it just long enough to become a list -- writing `;` directly
    # would be indistinguishable from the list separator we are building.
    string(ASCII 1 sep)
    string(REGEX REPLACE "\"[ \t]*,[ \t]*\"" "\"${sep}\"" body "${body}")
    string(REPLACE "${sep}" ";" body "${body}")

    foreach(item IN LISTS body)
        _vendor_parse_scalar("${item}" value item_ok)
        if(NOT item_ok)
            set(${ok} FALSE PARENT_SCOPE)
            return()
        endif()
        list(APPEND items "${value}")
    endforeach()

    set(${out} "${items}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------ interface

function(vendor_read_manifest path)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "vendor: manifest not found: ${path}")
    endif()

    file(READ "${path}" content)

    # A UTF-8 BOM would otherwise become part of the first token. Several files
    # in this repo carry one, so assume the manifest might too. CMake has no \x
    # escape, hence string(ASCII) for the three bytes.
    string(ASCII 239 187 191 bom)
    if(content MATCHES "^${bom}")
        string(SUBSTRING "${content}" 3 -1 content)
    endif()

    # Make the newline the list separator so foreach() can walk lines.
    #
    # Square brackets have to be hidden first. CMake's list splitting treats
    # [ ... ] as a nesting group and refuses to split inside it, so a multi-line
    # array in this file would otherwise arrive as one giant "line":
    #
    #   set(x "p;a[b;c]d;q")  ->  list(LENGTH x) is 3, not 5
    #
    # Swapping the brackets for control characters across the split, then back
    # per line, sidesteps that entirely. Parentheses do not have the same
    # behaviour and need no such treatment.
    string(ASCII 2 lbracket)
    string(ASCII 3 rbracket)
    string(REPLACE "[" "${lbracket}" content "${content}")
    string(REPLACE "]" "${rbracket}" content "${content}")

    # Protect literal semicolons so they cannot masquerade as separators.
    string(REPLACE ";" "\\;" content "${content}")
    string(REPLACE "\r" "" content "${content}")
    string(REPLACE "\n" ";" content "${content}")

    set(deps "")
    set(scope "")           # "" before any table, "settings", or a dependency name
    set(expect_name FALSE)  # set right after [[dependency]]
    set(array_key "")       # non-empty while an array is still open
    set(array_items "")
    set(lineno 0)

    foreach(line IN LISTS content)
        math(EXPR lineno "${lineno} + 1")

        string(REPLACE "${lbracket}" "[" line "${line}")
        string(REPLACE "${rbracket}" "]" line "${line}")

        _vendor_strip_comment("${line}" line)
        string(STRIP "${line}" line)

        if(line STREQUAL "")
            continue()
        endif()

        # Checked after comment stripping, so prose may use semicolons freely.
        if(line MATCHES ";")
            message(FATAL_ERROR
                "${path}:${lineno}: semicolons are not supported in manifest "
                "values (CMake cannot represent one unambiguously).")
        endif()

        # ------------------------------------------------ continuing an array
        if(NOT array_key STREQUAL "")
            set(body "${line}")
            set(closed FALSE)
            if(body MATCHES "^(.*)\\]$")
                set(body "${CMAKE_MATCH_1}")
                set(closed TRUE)
            endif()

            _vendor_parse_array_body("${body}" array_items body_ok)
            if(NOT body_ok)
                message(FATAL_ERROR
                    "${path}:${lineno}: cannot parse array element: ${line}")
            endif()

            if(closed)
                string(TOUPPER "${array_key}" KEY)
                if(scope STREQUAL "settings")
                    set(VENDOR_SETTINGS_${KEY} "${array_items}")
                else()
                    set(VENDOR_DEP_${scope}_${KEY} "${array_items}")
                endif()
                set(array_key "")
                set(array_items "")
            endif()
            continue()
        endif()

        # ------------------------------------------------------------ tables
        if(line STREQUAL "[[dependency]]")
            set(scope "")
            set(expect_name TRUE)
            continue()
        endif()

        if(line STREQUAL "[settings]")
            set(scope "settings")
            set(expect_name FALSE)
            continue()
        endif()

        if(line MATCHES "^\\[")
            message(FATAL_ERROR
                "${path}:${lineno}: unknown table '${line}'. "
                "Only [settings] and [[dependency]] are supported.")
        endif()

        # ------------------------------------------------------- key = value
        if(NOT line MATCHES "^([A-Za-z_][A-Za-z0-9_]*)[ \t]*=[ \t]*(.*)$")
            message(FATAL_ERROR "${path}:${lineno}: cannot parse: ${line}")
        endif()
        set(key "${CMAKE_MATCH_1}")
        set(raw "${CMAKE_MATCH_2}")

        # ------------------------------------------------- validate the key
        if(expect_name)
            if(NOT key STREQUAL "name")
                message(FATAL_ERROR
                    "${path}:${lineno}: expected 'name' as the first key of the "
                    "[[dependency]] block, got '${key}'.")
            endif()
            _vendor_parse_scalar("${raw}" dep_name name_ok)
            if(NOT name_ok OR dep_name STREQUAL "")
                message(FATAL_ERROR
                    "${path}:${lineno}: 'name' must be a non-empty quoted string.")
            endif()
            if(NOT dep_name MATCHES "^[A-Za-z0-9_.+-]+$")
                message(FATAL_ERROR
                    "${path}:${lineno}: '${dep_name}' is not usable as a directory "
                    "name. Use letters, digits, and _ . + - only.")
            endif()
            if(dep_name IN_LIST deps)
                message(FATAL_ERROR
                    "${path}:${lineno}: duplicate dependency '${dep_name}'.")
            endif()
            list(APPEND deps "${dep_name}")
            set(VENDOR_DEP_${dep_name}_NAME "${dep_name}")
            set(scope "${dep_name}")
            set(expect_name FALSE)
            continue()
        endif()

        if(scope STREQUAL "")
            message(FATAL_ERROR
                "${path}:${lineno}: '${key}' appears before any "
                "[settings] or [[dependency]] table.")
        endif()

        if(scope STREQUAL "settings")
            if(NOT key IN_LIST VENDOR_SETTINGS_KEYS)
                string(REPLACE ";" ", " known "${VENDOR_SETTINGS_KEYS}")
                message(FATAL_ERROR
                    "${path}:${lineno}: unknown [settings] key '${key}'.\n"
                    "Known keys: ${known}")
            endif()
        else()
            if(NOT key IN_LIST VENDOR_DEP_KEYS)
                string(REPLACE ";" ", " known "${VENDOR_DEP_KEYS}")
                message(FATAL_ERROR
                    "${path}:${lineno}: unknown [[dependency]] key '${key}' "
                    "on '${scope}'.\nKnown keys: ${known}")
            endif()
        endif()

        # ----------------------------------------------------- store the value
        string(TOUPPER "${key}" KEY)

        if(raw MATCHES "^\\[(.*)\\]$")
            set(array_items "")
            _vendor_parse_array_body("${CMAKE_MATCH_1}" array_items body_ok)
            if(NOT body_ok)
                message(FATAL_ERROR "${path}:${lineno}: cannot parse array: ${raw}")
            endif()
            set(value "${array_items}")
            set(array_items "")
        elseif(raw MATCHES "^\\[(.*)$")
            # Opens here, closes on a later line; nothing to store yet.
            set(array_key "${key}")
            set(array_items "")
            _vendor_parse_array_body("${CMAKE_MATCH_1}" array_items body_ok)
            if(NOT body_ok)
                message(FATAL_ERROR "${path}:${lineno}: cannot parse array: ${raw}")
            endif()
            continue()
        else()
            _vendor_parse_scalar("${raw}" value scalar_ok)
            if(NOT scalar_ok)
                message(FATAL_ERROR
                    "${path}:${lineno}: cannot parse value: ${raw}\n"
                    "Expected a quoted string, true, or false.")
            endif()
        endif()

        if(scope STREQUAL "settings")
            set(VENDOR_SETTINGS_${KEY} "${value}")
        else()
            set(VENDOR_DEP_${scope}_${KEY} "${value}")
        endif()
    endforeach()

    if(NOT array_key STREQUAL "")
        message(FATAL_ERROR "${path}: unterminated array for key '${array_key}'.")
    endif()
    if(expect_name)
        message(FATAL_ERROR "${path}: trailing [[dependency]] with no 'name'.")
    endif()

    # --------------------------------------------------------------- export
    set(VENDOR_DEPS "${deps}" PARENT_SCOPE)
    foreach(dep IN LISTS deps)
        foreach(k IN LISTS VENDOR_DEP_KEYS)
            string(TOUPPER "${k}" K)
            set(VENDOR_DEP_${dep}_${K} "${VENDOR_DEP_${dep}_${K}}" PARENT_SCOPE)
        endforeach()
    endforeach()
    foreach(k IN LISTS VENDOR_SETTINGS_KEYS)
        string(TOUPPER "${k}" K)
        set(VENDOR_SETTINGS_${K} "${VENDOR_SETTINGS_${K}}" PARENT_SCOPE)
    endforeach()
endfunction()
