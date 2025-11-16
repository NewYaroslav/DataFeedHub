# ===== deps/simdcomp.cmake =====
# Purpose: Provide dfh::simdcomp static library built from the vendored sources.

include_guard(GLOBAL)

function(dfh_use_or_fetch_simdcomp out_target)
    if(TARGET dfh::simdcomp)
        set(${out_target} dfh::simdcomp PARENT_SCOPE)
        return()
    endif()

    set(_SIMDCOMP_SRC "${PROJECT_SOURCE_DIR}/libs/simdcomp")
    if(NOT EXISTS "${_SIMDCOMP_SRC}/src/simdfor.c")
        message(FATAL_ERROR "simdcomp sources are missing (${_SIMDCOMP_SRC})")
    endif()

    set(_SIMDCOMP_SOURCES
        "${_SIMDCOMP_SRC}/src/avx512bitpacking.c"
        "${_SIMDCOMP_SRC}/src/avxbitpacking.c"
        "${_SIMDCOMP_SRC}/src/simdfor.c"
        "${_SIMDCOMP_SRC}/src/simdcomputil.c"
        "${_SIMDCOMP_SRC}/src/simdbitpacking.c"
        "${_SIMDCOMP_SRC}/src/simdintegratedbitpacking.c"
        "${_SIMDCOMP_SRC}/src/simdpackedsearch.c"
        "${_SIMDCOMP_SRC}/src/simdpackedselect.c"
    )

    add_library(dfh_simdcomp STATIC ${_SIMDCOMP_SOURCES})
    target_include_directories(dfh_simdcomp PUBLIC "${_SIMDCOMP_SRC}/include")
    if(MSVC)
        target_compile_options(dfh_simdcomp PRIVATE /permissive- /W4)
    else()
        target_compile_options(dfh_simdcomp PRIVATE -Wall -Wextra -Wshadow)
    endif()
    set_target_properties(dfh_simdcomp PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
    )

    add_library(dfh::simdcomp ALIAS dfh_simdcomp)
    set(${out_target} dfh::simdcomp PARENT_SCOPE)
endfunction()
