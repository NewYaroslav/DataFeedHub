# ===== deps/fast_float.cmake =====
# Purpose: Provide dfh::fast_float header-only target.

include_guard(GLOBAL)

function(dfh_use_or_fetch_fast_float out_target)
    if(TARGET dfh::fast_float)
        set(${out_target} dfh::fast_float PARENT_SCOPE)
        return()
    endif()

    set(_FAST_FLOAT_INCLUDE "${PROJECT_SOURCE_DIR}/libs/fast_float/include")
    if(NOT EXISTS "${_FAST_FLOAT_INCLUDE}/fast_float/fast_float.h")
        message(FATAL_ERROR "fast_float headers are missing (${_FAST_FLOAT_INCLUDE})")
    endif()

    add_library(dfh_fast_float INTERFACE)
    target_include_directories(dfh_fast_float INTERFACE "${_FAST_FLOAT_INCLUDE}")

    add_library(dfh::fast_float ALIAS dfh_fast_float)
    set(${out_target} dfh::fast_float PARENT_SCOPE)
endfunction()
