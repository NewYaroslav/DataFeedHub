# ===== deps/time_shield_cpp.cmake =====
# Purpose: Provide dfh::time_shield_cpp header-only target.

include_guard(GLOBAL)

function(dfh_use_or_fetch_time_shield_cpp out_target)
    if(TARGET dfh::time_shield_cpp)
        set(${out_target} dfh::time_shield_cpp PARENT_SCOPE)
        return()
    endif()

    set(_TSHIELD_INCLUDE "${PROJECT_SOURCE_DIR}/libs/time-shield-cpp/include")
    if(NOT EXISTS "${_TSHIELD_INCLUDE}/time_shield_cpp/time_shield.hpp")
        message(FATAL_ERROR "time-shield-cpp headers are missing (${_TSHIELD_INCLUDE})")
    endif()

    add_library(dfh_time_shield_cpp INTERFACE)
    target_include_directories(dfh_time_shield_cpp INTERFACE
        "${_TSHIELD_INCLUDE}"
        "${_TSHIELD_INCLUDE}/time_shield_cpp"
    )

    add_library(dfh::time_shield_cpp ALIAS dfh_time_shield_cpp)
    set(${out_target} dfh::time_shield_cpp PARENT_SCOPE)
endfunction()
