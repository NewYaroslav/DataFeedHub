# ===== deps/zlib_ng.cmake =====
# Purpose: Provide a ZLIB::ZLIB target backed by zlib-ng or system zlib.

include_guard(GLOBAL)

function(dfh_use_or_fetch_zlib_ng out_target)
    if(TARGET ZLIB::ZLIB)
        set(${out_target} ZLIB::ZLIB PARENT_SCOPE)
        return()
    endif()

    dfh_get_effective_mode(DFH_DEPS_ZLIB_MODE mode)

    if(NOT mode STREQUAL "BUNDLED")
        find_package(ZLIB QUIET)
        if(TARGET ZLIB::ZLIB)
            set(${out_target} ZLIB::ZLIB PARENT_SCOPE)
            return()
        endif()
    endif()

    if(mode STREQUAL "SYSTEM")
        message(FATAL_ERROR "zlib not found in SYSTEM mode")
    endif()

    set(_ZLIB_SRC "${PROJECT_SOURCE_DIR}/libs/zlib-ng")
    if(NOT EXISTS "${_ZLIB_SRC}/CMakeLists.txt")
        message(FATAL_ERROR "zlib-ng submodule is missing (${_ZLIB_SRC})")
    endif()

    set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(ZLIBNG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WITH_GTEST OFF CACHE BOOL "" FORCE)
    set(ZLIBNG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZLIBNG_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)

    add_subdirectory("${_ZLIB_SRC}" "${CMAKE_BINARY_DIR}/_deps/zlib-ng-build" EXCLUDE_FROM_ALL)

    if(TARGET zlibstatic)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    elseif(TARGET zlib)
        add_library(ZLIB::ZLIB ALIAS zlib)
    else()
        message(FATAL_ERROR "zlib-ng configuration did not produce zlibstatic target")
    endif()

    set(${out_target} ZLIB::ZLIB PARENT_SCOPE)
endfunction()
