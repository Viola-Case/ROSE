# ==============================================================================
# ROSE working-copy initialisation
# ==============================================================================
#
# One-time (and safely repeatable) setup for a fresh clone: the things a build
# does not do for you and git cannot carry, because they live in directories
# .gitignore excludes.
#
# Run it through the launchers rather than directly:
#
#     ./init.ps1  [flags] [task...]     (Windows)
#     ./init.sh   [flags] [task...]     (Linux, macOS)
#
# or, equivalently:
#
#     cmake -P cmake/init.cmake -- [flags] [task...]
#
# Every task is idempotent: a second run reports what is already in place and
# changes nothing. Adding a task means writing an init_task_<name> function and
# naming it in INIT_ALL_TASKS below; the dispatcher and the --list output follow
# from that.

cmake_minimum_required(VERSION 3.21)

get_filename_component(INIT_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(REPO_ROOT "${INIT_CMAKE_DIR}/.." ABSOLUTE)

# Tasks run in this order when none is named on the command line.
set(INIT_ALL_TASKS
    ide-schemas
)

set(INIT_HELP [[
ROSE working-copy initialisation

  ./init.ps1 [flags] [task...]     Windows
  ./init.sh  [flags] [task...]     Linux, macOS

With no task named, every task runs. Each is idempotent.

Tasks:
  ide-schemas            Point VS Code and JetBrains at schemas/scene.schema.json
                         so scene files get completion and validation. Both IDE
                         config directories are gitignored, which is why this
                         cannot simply be committed

Flags:
  --vscode               Restrict IDE tasks to .vscode
  --jetbrains            Restrict IDE tasks to .idea
                         (with neither, both are written)
  --list                 Show the tasks and exit
  --dry-run              Report what would change; write nothing
  --help                 This message
]])

# ==============================================================================
# Small helpers
# ==============================================================================

# A single named parameter rather than ARGN, for the reason vendor.cmake gives:
# "${ARGN}" would put a stray ';' between pieces of a call split across several
# string arguments, and list(JOIN) instead swallows a ';' that is genuinely part
# of the text.
function(init_say text)
    message(STATUS "${text}")
endfunction()

function(init_die text)
    message(FATAL_ERROR "init: ${text}")
endfunction()

# NOTICE rather than WARNING: these carry a snippet or a menu path for the
# reader to copy, and WARNING reflows its text, which mangles both.
function(init_warn text)
    message(NOTICE "init: warning: ${text}")
endfunction()

# Writes `content` to `path` unless --dry-run, reporting either way. `what` is
# the subject of the sentence, e.g. ".vscode/settings.json"; the two verbs are
# the same action said as "would <do> it" and as "<did> it", which English will
# not let us derive from one another.
function(init_write path content what verb_now verb_done)
    if(INIT_DRY_RUN)
        init_say("would ${verb_now} ${what}")
        return()
    endif()
    get_filename_component(dir "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")
    file(WRITE "${path}" "${content}")
    set_property(GLOBAL PROPERTY INIT_CHANGED TRUE)
    init_say("${verb_done} ${what}")
endfunction()

# ==============================================================================
# Task: ide-schemas
# ==============================================================================
#
# The scene schema is useless to an editor that has not been told which files it
# describes. Neither IDE infers that, and neither IDE's config directory is under
# version control here (.gitignore excludes .vscode/ and .idea/), so the mapping
# has to be re-established per working copy. That is this task.

# Where the schema lives, relative to the repo root. Both IDEs resolve their
# mapping relative to the project root, so one path serves both.
set(INIT_SCENE_SCHEMA "schemas/scene.schema.json")

# Which files the schema describes. Scene files are ordinary .json with no
# distinguishing extension, so this is a list of the places they actually live --
# extend it as the layout grows rather than widening it to every .json in the
# tree, which would flag vcpkg.json and CMakePresets.json as broken scenes.
set(INIT_SCENE_GLOBS
    "examples/*/assets/*.json"
    "assets/scenes/*.json"
)

set(INIT_SCHEMA_TITLE "ROSE scene")

# ------------------------------------------------------------------- VS Code
#
# .vscode/settings.json, key "json.schemas": an array of { fileMatch, url }.
# The file is merged into rather than overwritten, so unrelated settings and
# other people's schema mappings survive. The merge does round-trip through
# CMake's JSON writer, which reformats -- harmless -- and drops comments, which
# is not. A file carrying anything a rewrite would destroy is therefore left
# untouched and the entry printed as a snippet to paste.
function(init_vscode_schemas)
    set(settings "${REPO_ROOT}/.vscode/settings.json")

    # The entry we want present, built through the JSON API so nothing depends
    # on hand-escaping.
    set(matches "[]")
    set(index 0)
    foreach(glob IN LISTS INIT_SCENE_GLOBS)
        string(JSON matches SET "${matches}" ${index} "\"${glob}\"")
        math(EXPR index "${index} + 1")
    endforeach()

    set(entry "{}")
    string(JSON entry SET "${entry}" "fileMatch" "${matches}")
    string(JSON entry SET "${entry}" "url" "\"./${INIT_SCENE_SCHEMA}\"")

    if(NOT EXISTS "${settings}")
        set(doc "{}")
        string(JSON doc SET "${doc}" "json.schemas" "[${entry}]")
        init_write("${settings}" "${doc}\n" ".vscode/settings.json" "write" "wrote")
        return()
    endif()

    file(READ "${settings}" doc)

    # CMake's JSON reader accepts comments and trailing commas and its writer
    # then drops them, so parsing cleanly is no evidence that a rewrite would be
    # lossless. Look at the text instead -- with string literals blanked out
    # first, or a "https://..." setting reads as a comment.
    string(REGEX REPLACE "\"(\\\\.|[^\"\\\\])*\"" "\"\"" bare "${doc}")
    set(reason "")
    if("${bare}" MATCHES "//" OR "${bare}" MATCHES "/[*]")
        set(reason "holds comments, which are legal for VS Code but which a rewrite\n  from here would silently delete")
    elseif("${bare}" MATCHES ",[ \t\r\n]*[]}]")
        set(reason "holds a trailing comma, which VS Code allows but strict JSON does not")
    else()
        string(JSON type ERROR_VARIABLE parse_error TYPE "${doc}")
        if(parse_error)
            set(reason "does not parse")
        elseif(NOT type STREQUAL "OBJECT")
            set(reason "is not a JSON object")
        endif()
    endif()

    if(reason)
        init_warn(".vscode/settings.json ${reason},
  so it was left alone. Add this to its \"json.schemas\" array by hand:

${entry}
")
        return()
    endif()

    string(JSON schemas ERROR_VARIABLE no_key GET "${doc}" "json.schemas")
    if(no_key)
        set(schemas "[]")
    endif()

    string(JSON count LENGTH "${schemas}")
    set(slot ${count})           # append position, unless ours is already there
    set(current "")
    if(count GREATER 0)
        math(EXPR last "${count} - 1")
        foreach(i RANGE 0 ${last})
            string(JSON existing GET "${schemas}" ${i})
            string(JSON url ERROR_VARIABLE no_url GET "${existing}" "url")
            # Match on the schema path alone: the ./ prefix is optional to VS Code
            # and a hand-written entry may well omit it.
            if(NOT no_url AND url MATCHES "${INIT_SCENE_SCHEMA}$")
                set(slot ${i})
                set(current "${existing}")
                break()
            endif()
        endforeach()
    endif()

    # Compare parsed forms, not text: whitespace differs between what VS Code
    # writes and what CMake does, and a reformat is not a change.
    if(current)
        string(JSON current_matches ERROR_VARIABLE ignored GET "${current}" "fileMatch")
        string(JSON wanted_matches GET "${entry}" "fileMatch")
        if(current_matches STREQUAL wanted_matches)
            init_say(".vscode/settings.json already maps the scene schema")
            return()
        endif()
        set(verb_now "update the scene schema mapping in")
        set(verb_done "updated the scene schema mapping in")
    else()
        set(verb_now "add the scene schema mapping to")
        set(verb_done "added the scene schema mapping to")
    endif()

    string(JSON schemas SET "${schemas}" ${slot} "${entry}")
    string(JSON doc SET "${doc}" "json.schemas" "${schemas}")
    init_write("${settings}" "${doc}\n" ".vscode/settings.json" "${verb_now}" "${verb_done}")
endfunction()

# ----------------------------------------------------------------- JetBrains
#
# .idea/jsonSchemas.xml, component JsonSchemaMappingsProjectConfiguration: one
# <entry> per mapping, keyed by the schema's display name.
#
# This is XML the IDE owns, and there is no XML API here, so the file is edited
# by splicing rather than parsed: a whole document when there is none, an
# insertion before the closing </map> when there is. An existing entry for our
# schema is left exactly as found -- once the IDE has rewritten the block (it
# adds a schemaVersion it derives from the schema's own $schema keyword, among
# other things) second-guessing it from here would do more harm than good.
function(init_jetbrains_schemas)
    set(xml "${REPO_ROOT}/.idea/jsonSchemas.xml")

    set(patterns "")
    foreach(glob IN LISTS INIT_SCENE_GLOBS)
        string(APPEND patterns
            "                  <Item>\n"
            "                    <option name=\"path\" value=\"${glob}\" />\n"
            "                    <option name=\"mappingKind\" value=\"Pattern\" />\n"
            "                  </Item>\n")
    endforeach()

    set(entry
        "        <entry key=\"${INIT_SCHEMA_TITLE}\">\n"
        "          <value>\n"
        "            <SchemaInfo>\n"
        "              <option name=\"name\" value=\"${INIT_SCHEMA_TITLE}\" />\n"
        "              <option name=\"relativePathToSchema\" value=\"${INIT_SCENE_SCHEMA}\" />\n"
        "              <option name=\"applicationDefinedName\" value=\"false\" />\n"
        "              <option name=\"patterns\">\n"
        "                <list>\n"
        "${patterns}"
        "                </list>\n"
        "              </option>\n"
        "            </SchemaInfo>\n"
        "          </value>\n"
        "        </entry>\n")
    string(JOIN "" entry ${entry})

    if(NOT EXISTS "${xml}")
        set(doc
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<project version=\"4\">\n"
            "  <component name=\"JsonSchemaMappingsProjectConfiguration\">\n"
            "    <state>\n"
            "      <map>\n"
            "${entry}"
            "      </map>\n"
            "    </state>\n"
            "  </component>\n"
            "</project>\n")
        string(JOIN "" doc ${doc})
        init_write("${xml}" "${doc}" ".idea/jsonSchemas.xml" "write" "wrote")
        return()
    endif()

    file(READ "${xml}" doc)

    if(doc MATCHES "relativePathToSchema\" value=\"${INIT_SCENE_SCHEMA}\"")
        init_say(".idea/jsonSchemas.xml already maps the scene schema")
        return()
    endif()

    # Splice into the mappings map, in whichever of its two shapes the IDE left
    # it. Anything else is a file we do not recognise and will not guess at.
    string(FIND "${doc}" "</map>" close REVERSE)
    if(close GREATER_EQUAL 0)
        string(SUBSTRING "${doc}" 0 ${close} head)
        string(SUBSTRING "${doc}" ${close} -1 tail)
        # head stops mid-line, on the indentation of the </map> we split at.
        # Drop it, or the entry lands at that depth plus its own.
        string(REGEX REPLACE "[ \t]*$" "" head "${head}")
        set(doc "${head}${entry}      ${tail}")
    elseif(doc MATCHES "<map[ ]*/>")
        string(REGEX REPLACE "<map[ ]*/>" "<map>\n${entry}      </map>" doc "${doc}")
    else()
        init_warn(".idea/jsonSchemas.xml exists but has no <map> to add to, so it was left
  alone. Map ${INIT_SCENE_SCHEMA} by hand instead, under
  Settings > Languages & Frameworks > Schemas and DTDs > JSON Schema Mappings.
")
        return()
    endif()

    init_write("${xml}" "${doc}" ".idea/jsonSchemas.xml"
        "add the scene schema mapping to" "added the scene schema mapping to")
endfunction()

function(init_task_ide-schemas)
    if(NOT EXISTS "${REPO_ROOT}/${INIT_SCENE_SCHEMA}")
        init_die("${INIT_SCENE_SCHEMA} is missing; nothing to map an editor at")
    endif()
    if(INIT_WANT_VSCODE)
        init_vscode_schemas()
    endif()
    if(INIT_WANT_JETBRAINS)
        init_jetbrains_schemas()
    endif()
    # Only worth saying when something actually moved -- a run that found
    # everything already in place has nothing for the IDE to notice.
    get_property(changed GLOBAL PROPERTY INIT_CHANGED)
    if(changed AND NOT INIT_DRY_RUN)
        init_say("reopen the project for the IDE to pick the mapping up")
    endif()
endfunction()

# ==============================================================================
# Command line
# ==============================================================================

set(INIT_LIST       FALSE)
set(INIT_DRY_RUN    FALSE)
set(INIT_WANT_VSCODE    TRUE)
set(INIT_WANT_JETBRAINS TRUE)
set(INIT_SELECTED   "")

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

set(ide_restricted FALSE)
foreach(arg IN LISTS args)
    if(arg STREQUAL "--help" OR arg STREQUAL "-h")
        message("${INIT_HELP}")
        return()
    elseif(arg STREQUAL "--list")
        set(INIT_LIST TRUE)
    elseif(arg STREQUAL "--dry-run")
        set(INIT_DRY_RUN TRUE)
    elseif(arg STREQUAL "--vscode")
        if(NOT ide_restricted)
            set(INIT_WANT_VSCODE FALSE)
            set(INIT_WANT_JETBRAINS FALSE)
            set(ide_restricted TRUE)
        endif()
        set(INIT_WANT_VSCODE TRUE)
    elseif(arg STREQUAL "--jetbrains")
        if(NOT ide_restricted)
            set(INIT_WANT_VSCODE FALSE)
            set(INIT_WANT_JETBRAINS FALSE)
            set(ide_restricted TRUE)
        endif()
        set(INIT_WANT_JETBRAINS TRUE)
    elseif(arg MATCHES "^-")
        init_die("unknown flag '${arg}'. Try --help")
    else()
        if(NOT arg IN_LIST INIT_ALL_TASKS)
            string(JOIN ", " known ${INIT_ALL_TASKS})
            init_die("unknown task '${arg}'. Known tasks: ${known}")
        endif()
        list(APPEND INIT_SELECTED "${arg}")
    endif()
endforeach()

if(INIT_LIST)
    message("Tasks:")
    foreach(task IN LISTS INIT_ALL_TASKS)
        message("  ${task}")
    endforeach()
    return()
endif()

if(NOT INIT_SELECTED)
    set(INIT_SELECTED ${INIT_ALL_TASKS})
endif()
list(REMOVE_DUPLICATES INIT_SELECTED)

foreach(task IN LISTS INIT_SELECTED)
    cmake_language(CALL "init_task_${task}")
endforeach()
