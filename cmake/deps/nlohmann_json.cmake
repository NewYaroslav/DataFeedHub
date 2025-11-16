# ===== deps/nlohmann_json.cmake =====
# Purpose: Provide nlohmann_json target from system package, submodule, or FetchContent.
# Inputs:  DFH_DEPS_MODE, DFH_DEPS_JSON_MODE
# Outputs: out_target receives nlohmann_json::nlohmann_json

include_guard(GLOBAL)

function(dfh_use_or_fetch_nlohmann_json out_target)
    if(TARGET nlohmann_json::nlohmann_json)
        set(${out_target} nlohmann_json::nlohmann_json PARENT_SCOPE)
        return()
    endif()

    dfh_get_effective_mode(DFH_DEPS_JSON_MODE mode)

    if(NOT mode STREQUAL "BUNDLED")
        find_package(nlohmann_json CONFIG QUIET)
        if(TARGET nlohmann_json::nlohmann_json)
            set(${out_target} nlohmann_json::nlohmann_json PARENT_SCOPE)
            return()
        endif()
    endif()

    if(mode STREQUAL "SYSTEM")
        message(FATAL_ERROR "nlohmann_json not found in SYSTEM mode")
    endif()

    set(_JSON_SRC "${PROJECT_SOURCE_DIR}/libs/json")
    if(EXISTS "${_JSON_SRC}/CMakeLists.txt")
        add_subdirectory("${_JSON_SRC}" "${CMAKE_BINARY_DIR}/_deps/nlohmann-json-build" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.11.3
        )
        FetchContent_MakeAvailable(nlohmann_json)
    endif()

    if(TARGET nlohmann_json::nlohmann_json)
        set(${out_target} nlohmann_json::nlohmann_json PARENT_SCOPE)
    elseif(TARGET nlohmann_json)
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
        set(${out_target} nlohmann_json::nlohmann_json PARENT_SCOPE)
    else()
        message(FATAL_ERROR "nlohmann_json target not found after configuration")
    endif()
endfunction()
