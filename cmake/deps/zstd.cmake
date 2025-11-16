# ===== deps/zstd.cmake =====
# Purpose: Provide ZSTD::ZSTD target from system package or vendored zstd.

include_guard(GLOBAL)

function(dfh_use_or_fetch_zstd out_target)
    if(TARGET ZSTD::ZSTD)
        set(${out_target} ZSTD::ZSTD PARENT_SCOPE)
        return()
    endif()

    dfh_get_effective_mode(DFH_DEPS_ZSTD_MODE mode)

    if(NOT mode STREQUAL "BUNDLED")
        find_package(zstd CONFIG QUIET)
        if(TARGET zstd::libzstd_shared)
            add_library(ZSTD::ZSTD ALIAS zstd::libzstd_shared)
            set(${out_target} ZSTD::ZSTD PARENT_SCOPE)
            return()
        elseif(TARGET zstd::libzstd_static)
            add_library(ZSTD::ZSTD ALIAS zstd::libzstd_static)
            set(${out_target} ZSTD::ZSTD PARENT_SCOPE)
            return()
        endif()
    endif()

    if(mode STREQUAL "SYSTEM")
        message(FATAL_ERROR "zstd not found in SYSTEM mode")
    endif()

    set(_ZSTD_SRC "${PROJECT_SOURCE_DIR}/libs/zstd")
    if(NOT EXISTS "${_ZSTD_SRC}/build/cmake/CMakeLists.txt")
        message(FATAL_ERROR "zstd submodule is missing (${_ZSTD_SRC})")
    endif()

    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)

    add_subdirectory("${_ZSTD_SRC}/build/cmake" "${CMAKE_BINARY_DIR}/_deps/zstd-build" EXCLUDE_FROM_ALL)

    if(TARGET libzstd_static)
        add_library(ZSTD::ZSTD ALIAS libzstd_static)
    elseif(TARGET libzstd_shared)
        add_library(ZSTD::ZSTD ALIAS libzstd_shared)
    else()
        message(FATAL_ERROR "zstd configuration did not produce libzstd target")
    endif()

    set(${out_target} ZSTD::ZSTD PARENT_SCOPE)
endfunction()
