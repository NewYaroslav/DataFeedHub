# ===== deps/gzip_hpp.cmake =====
# Purpose: Provide dfh::gzip_hpp header-only target.

include_guard(GLOBAL)

function(dfh_use_or_fetch_gzip_hpp out_target)
    if(TARGET dfh::gzip_hpp)
        set(${out_target} dfh::gzip_hpp PARENT_SCOPE)
        return()
    endif()

    set(_GZIP_INCLUDE "${PROJECT_SOURCE_DIR}/libs/gzip-hpp/include")
    if(NOT EXISTS "${_GZIP_INCLUDE}/gzip/compress.hpp")
        message(FATAL_ERROR "gzip-hpp headers are missing (${_GZIP_INCLUDE})")
    endif()

    add_library(dfh_gzip_hpp INTERFACE)
    target_include_directories(dfh_gzip_hpp INTERFACE "${_GZIP_INCLUDE}")
    target_compile_definitions(dfh_gzip_hpp INTERFACE ZLIB_CONST)

    add_library(dfh::gzip_hpp ALIAS dfh_gzip_hpp)
    set(${out_target} dfh::gzip_hpp PARENT_SCOPE)
endfunction()
