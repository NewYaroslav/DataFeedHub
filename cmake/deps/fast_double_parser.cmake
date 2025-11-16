# ===== deps/fast_double_parser.cmake =====
# Purpose: Provide dfh::fast_double_parser header-only target.

include_guard(GLOBAL)

function(dfh_use_or_fetch_fast_double_parser out_target)
    if(TARGET dfh::fast_double_parser)
        set(${out_target} dfh::fast_double_parser PARENT_SCOPE)
        return()
    endif()

    set(_FDP_INCLUDE "${PROJECT_SOURCE_DIR}/libs/fast_double_parser/include")
    if(NOT EXISTS "${_FDP_INCLUDE}/fast_double_parser.h")
        message(FATAL_ERROR "fast_double_parser headers are missing (${_FDP_INCLUDE})")
    endif()

    add_library(dfh_fast_double_parser INTERFACE)
    target_include_directories(dfh_fast_double_parser INTERFACE "${_FDP_INCLUDE}")

    add_library(dfh::fast_double_parser ALIAS dfh_fast_double_parser)
    set(${out_target} dfh::fast_double_parser PARENT_SCOPE)
endfunction()
