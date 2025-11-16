# ===== deps/minizip_ng.cmake =====
# Purpose: Provide dfh::minizip target backed by minizip-ng or system package.

include_guard(GLOBAL)

function(dfh_use_or_fetch_minizip_ng out_target zlib_target)
    if(TARGET dfh::minizip)
        set(${out_target} dfh::minizip PARENT_SCOPE)
        return()
    endif()

    if(NOT TARGET ${zlib_target})
        message(FATAL_ERROR "minizip-ng requires a valid zlib target; got '${zlib_target}'")
    endif()

    dfh_get_effective_mode(DFH_DEPS_MINIZIP_MODE mode)

    if(NOT mode STREQUAL "BUNDLED")
        find_package(minizip CONFIG QUIET)
        if(TARGET minizip::minizip)
            add_library(dfh::minizip ALIAS minizip::minizip)
            set(${out_target} dfh::minizip PARENT_SCOPE)
            return()
        elseif(TARGET minizip)
            add_library(dfh::minizip ALIAS minizip)
            set(${out_target} dfh::minizip PARENT_SCOPE)
            return()
        endif()
    endif()

    if(mode STREQUAL "SYSTEM")
        message(FATAL_ERROR "minizip not found in SYSTEM mode")
    endif()

    set(_MINIZIP_SRC "${PROJECT_SOURCE_DIR}/libs/minizip-ng")
    if(NOT EXISTS "${_MINIZIP_SRC}/CMakeLists.txt")
        message(FATAL_ERROR "minizip-ng submodule is missing (${_MINIZIP_SRC})")
    endif()

    set(MZ_INSTALL OFF CACHE BOOL "" FORCE)
    set(MZ_FETCH_LIBS OFF CACHE BOOL "" FORCE)
    set(MZ_FORCE_ZLIB ON CACHE BOOL "" FORCE)
    set(MZ_ZLIB ON CACHE BOOL "" FORCE)
    set(MZ_ZIP64_SUPPORT ON CACHE BOOL "" FORCE)
    set(MZ_PKCRYPT OFF CACHE BOOL "" FORCE)
    set(MZ_BZIP2 OFF CACHE BOOL "" FORCE)
    set(MZ_LZMA OFF CACHE BOOL "" FORCE)
    set(MZ_ZSTD OFF CACHE BOOL "" FORCE)
    set(MZ_WZAES OFF CACHE BOOL "" FORCE)
    set(MZ_OPENSSL OFF CACHE BOOL "" FORCE)
    set(MZ_ICONV OFF CACHE BOOL "" FORCE)
    set(MZ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(MZ_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(MZ_BUILD_FUZZ_TESTS OFF CACHE BOOL "" FORCE)

    add_subdirectory("${_MINIZIP_SRC}" "${CMAKE_BINARY_DIR}/_deps/minizip-ng-build" EXCLUDE_FROM_ALL)

    if(TARGET minizip)
        target_link_libraries(minizip PRIVATE ${zlib_target})
        add_library(dfh::minizip ALIAS minizip)
    else()
        message(FATAL_ERROR "minizip-ng configuration did not produce minizip target")
    endif()

    set(${out_target} dfh::minizip PARENT_SCOPE)
endfunction()
