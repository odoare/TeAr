# ──────────────────────────────────────────────────────────────────────────
# FxmeDeps.cmake — resolves the shared FX-Mechanics repos (JUCE, FxmeTools,
# FxmeFX, ...) without git submodules.
#
# Each dependency is either
#   * a sibling clone next to this project (~/src/<Name>) when
#     FXME_USE_SIBLINGS is ON (the default) and that directory exists, or
#   * the commit pinned by the project, downloaded by FetchContent.
#
# Release builds (CI or local) configure with -DFXME_USE_SIBLINGS=OFF so they
# come from the pins only. Every dependency prints where it came from, and a
# sibling clone also prints its commit and whether it is dirty.
#
# Dependencies are only *populated*, never added: the caller decides what to
# do with <NAME>_DIR (add_subdirectory(${JUCE_DIR}), include a .cmake from
# ${FXMETOOLS_DIR}, compile sources from ${FXMEFX_DIR}, ...).
#
# FetchContent dedups by name and the first declaration wins, so the
# top-level project must call fxme_dependency() for every repo it uses,
# before anything nested gets a chance to.
#
# This file is identical in every FX-Mechanics project: copy it, don't edit
# it in one place only.
# ──────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)
include(FetchContent)

option(FXME_USE_SIBLINGS "Use ../<Repo> clones instead of the pinned commits" ON)

# Where sibling clones live: the parent of the repo holding this file
# (<repo>/cmake/FxmeDeps.cmake → <repo>/..), whichever directory cmake -S
# pointed at — a plugin configured on its own included.
get_filename_component(_fxme_default_siblings "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(FXME_SIBLINGS_DIR "${_fxme_default_siblings}" CACHE PATH
    "Directory holding the sibling clones (JUCE, FxmeTools, ...)")

# fxme_dependency(<Name> <git-url> <tag-or-sha> [SIBLING <dir-name>] [SHALLOW])
#
# Sets <NAME>_DIR (upper case) in the caller's scope.
#   SIBLING  directory name under FXME_SIBLINGS_DIR (default: <Name>)
#   SHALLOW  shallow clone; only valid when <tag-or-sha> is a tag or branch
#
# Prefer full SHAs or tags: a branch name is not a pin.
# -DFETCHCONTENT_SOURCE_DIR_<NAME>=<path> still overrides a single repo.
function(fxme_dependency name url tag)
    cmake_parse_arguments(arg "SHALLOW" "SIBLING" "" ${ARGN})
    if(NOT arg_SIBLING)
        set(arg_SIBLING ${name})
    endif()
    string(TOUPPER "${name}" upper)
    string(TOLOWER "${name}" lower)

    # FetchContent stores FETCHCONTENT_SOURCE_DIR_<NAME> in the cache, the
    # sibling path included. Remember which value we injected, so that turning
    # FXME_USE_SIBLINGS off in an existing build dir really goes back to the
    # pin; a path the user passed with -D is left alone. Test the value, not
    # whether the variable is defined: it is (empty) after the first configure.
    set(sibling "${FXME_SIBLINGS_DIR}/${arg_SIBLING}")
    set(use_sibling FALSE)
    if(FXME_USE_SIBLINGS AND EXISTS "${sibling}")
        set(use_sibling TRUE)
    endif()
    if(DEFINED _FXME_INJECTED_${upper}
       AND "${FETCHCONTENT_SOURCE_DIR_${upper}}" STREQUAL "${_FXME_INJECTED_${upper}}")
        unset(FETCHCONTENT_SOURCE_DIR_${upper} CACHE)
        unset(FETCHCONTENT_SOURCE_DIR_${upper})
    endif()
    unset(_FXME_INJECTED_${upper} CACHE)
    if(use_sibling AND NOT FETCHCONTENT_SOURCE_DIR_${upper})
        set(FETCHCONTENT_SOURCE_DIR_${upper} "${sibling}")
        set(_FXME_INJECTED_${upper} "${sibling}" CACHE INTERNAL "")
    endif()
    if(FETCHCONTENT_SOURCE_DIR_${upper})
        set(local TRUE)
    else()
        set(local FALSE)
    endif()

    set(shallow "")
    if(arg_SHALLOW)
        set(shallow GIT_SHALLOW TRUE)
    endif()

    # SOURCE_SUBDIR names a directory without a CMakeLists.txt, so
    # FetchContent_MakeAvailable() populates the sources and adds nothing.
    # Submodules (WDL inside FxmeTools) are cloned recursively by default.
    FetchContent_Declare(${name}
        GIT_REPOSITORY ${url}
        GIT_TAG        ${tag}
        ${shallow}
        SOURCE_SUBDIR  fxme-populate-only)
    FetchContent_MakeAvailable(${name})

    set(dir "${${lower}_SOURCE_DIR}")
    if(local)
        _fxme_describe_clone("${dir}" "${tag}" state)
        message(STATUS "${name}: local clone ${dir} (${state})")
    else()
        message(STATUS "${name}: pinned ${tag}")
    endif()
    set(${upper}_DIR "${dir}" PARENT_SCOPE)
endfunction()

# Commit, dirty flag and whether it matches the pin, for the status line.
function(_fxme_describe_clone dir tag out)
    find_package(Git QUIET)
    if(NOT GIT_FOUND OR NOT EXISTS "${dir}/.git")
        set(${out} "not a git clone" PARENT_SCOPE)
        return()
    endif()
    execute_process(COMMAND ${GIT_EXECUTABLE} -C "${dir}" rev-parse HEAD
        OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} -C "${dir}" rev-parse --verify --quiet "${tag}^{commit}"
        OUTPUT_VARIABLE pinned OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} -C "${dir}" status --porcelain --untracked-files=no
        OUTPUT_VARIABLE changes ERROR_QUIET)

    string(SUBSTRING "${head}" 0 9 state)
    if(NOT changes STREQUAL "")
        string(APPEND state ", dirty")
    endif()
    if(pinned STREQUAL head)
        string(APPEND state ", = pin")
    else()
        string(APPEND state ", differs from pin ${tag}")
    endif()
    set(${out} "${state}" PARENT_SCOPE)
endfunction()
