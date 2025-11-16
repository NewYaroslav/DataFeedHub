# ===== deps/vbyte.cmake =====
# Purpose: Provide dfh::vbyte static library built from libvbyte sources.

include_guard(GLOBAL)

function(dfh_use_or_fetch_vbyte out_target)
    if(TARGET dfh::vbyte)
        set(${out_target} dfh::vbyte PARENT_SCOPE)
        return()
    endif()

    set(_VBYTE_SRC "${PROJECT_SOURCE_DIR}/libs/libvbyte")
    if(NOT EXISTS "${_VBYTE_SRC}/vbyte.cc")
        message(FATAL_ERROR "libvbyte sources are missing (${_VBYTE_SRC})")
    endif()

    add_library(dfh_vbyte STATIC
        "${_VBYTE_SRC}/vbyte.cc"
        "${_VBYTE_SRC}/varintdecode.c"
    )
    target_include_directories(dfh_vbyte PUBLIC "${_VBYTE_SRC}")
    if(MSVC)
        target_compile_options(dfh_vbyte PRIVATE /permissive- /W4 /arch:AVX)
    else()
        target_compile_options(dfh_vbyte PRIVATE -Wall -Wextra -Wpedantic -mssse3 -msse4.1)
    endif()
    set_target_properties(dfh_vbyte PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
    )

    add_library(dfh::vbyte ALIAS dfh_vbyte)
    set(${out_target} dfh::vbyte PARENT_SCOPE)
endfunction()
