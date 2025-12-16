cmake_minimum_required(VERSION 3.20)
project(DataFeedHub LANGUAGES C CXX VERSION 0.1.0)

if (MSVC)
    # Корректный __cplusplus для libmdbx
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>")

    # Запретить макросы min/max из <windows.h>, чтобы не ломать std::max и т.п.
    add_compile_definitions(NOMINMAX)
endif()

# ===== Root configuration =====
if(POLICY CMP0152)
    cmake_policy(SET CMP0152 NEW)
endif()

option(DFH_MSVC_AVOID_MSYS "Avoid picking MSYS/MinGW deps when building with MSVC" ON)

if(MSVC AND DFH_MSVC_AVOID_MSYS)
    set(_msys_paths "")
    foreach(p "C:/msys64" "C:/msys64/mingw64")
        if(EXISTS "${p}")
            list(APPEND _msys_paths "${p}")
        endif()
    endforeach()

    if(_msys_paths)
        set(CMAKE_IGNORE_PREFIX_PATH "${_msys_paths}" CACHE STRING "" FORCE)
        set(CMAKE_IGNORE_PATH        "${_msys_paths}" CACHE STRING "" FORCE)
        set(CMAKE_SYSTEM_IGNORE_PATH "${_msys_paths}" CACHE STRING "" FORCE)
    endif()
endif()

option(DFH_BUILD_TESTS "Build tests from the tests/ folder" ON)
option(DFH_BUILD_LEGACY_TESTS "Build existing regression tests (may require extra setup)" OFF)
option(DFH_SDK_BUNDLE_DEPS "Install vendored dependencies when preparing SDK artifacts" OFF)

set(DFH_DEPS_MODE "AUTO" CACHE STRING "AUTO|SYSTEM|BUNDLED")
set_property(CACHE DFH_DEPS_MODE PROPERTY STRINGS AUTO SYSTEM BUNDLED)
foreach(dep MDBX JSON ZLIB MINIZIP ZSTD)
    set(DFH_DEPS_${dep}_MODE "INHERIT" CACHE STRING "INHERIT|AUTO|SYSTEM|BUNDLED")
    set_property(CACHE DFH_DEPS_${dep}_MODE PROPERTY STRINGS INHERIT AUTO SYSTEM BUNDLED)
endforeach()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# CMAKE_C_EXTENSIONS OFF 
# * глобально влияет на все C-таргеты, включая зависимости (libmdbx, zlib-ng, minizip-ng и т.д.);
# * может переключить сборку на “строгий” режим стандарта и внезапно сломать платформенные куски/расширения компилятора;
# * основной проект - C++ interface-библиотека, своей C-компиляции практически нет -> пользы мало, риск есть.
# поэтому лучше не добавлять
# - set(CMAKE_C_EXTENSIONS OFF)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(cmake/deps/helpers.cmake)
include(cmake/deps/mdbx.cmake)
include(cmake/deps/nlohmann_json.cmake)
include(cmake/deps/zlib_ng.cmake)
include(cmake/deps/minizip_ng.cmake)
include(cmake/deps/zstd.cmake)
include(cmake/deps/simdcomp.cmake)
include(cmake/deps/vbyte.cmake)
include(cmake/deps/time_shield_cpp.cmake)
include(cmake/deps/gzip_hpp.cmake)
include(cmake/deps/fast_double_parser.cmake)
include(cmake/deps/fast_float.cmake)

# ===== Resolve dependencies =====
dfh_use_or_fetch_zlib_ng(ZLIB_TARGET)
dfh_use_or_fetch_minizip_ng(MINIZIP_TARGET "${ZLIB_TARGET}")
dfh_use_or_fetch_zstd(ZSTD_TARGET)
dfh_use_or_fetch_simdcomp(SIMDCOMP_TARGET)
dfh_use_or_fetch_vbyte(VBYTE_TARGET)
dfh_use_or_fetch_mdbx(MDBX_TARGET)
dfh_use_or_fetch_nlohmann_json(JSON_TARGET)
dfh_use_or_fetch_time_shield_cpp(TIME_SHIELD_TARGET)
dfh_use_or_fetch_gzip_hpp(GZIP_HPP_TARGET)
dfh_use_or_fetch_fast_double_parser(FAST_DOUBLE_TARGET)
dfh_use_or_fetch_fast_float(FAST_FLOAT_TARGET)

set(DFH_ALL_DEP_TARGETS
    ${ZLIB_TARGET}
    ${MINIZIP_TARGET}
    ${ZSTD_TARGET}
    ${SIMDCOMP_TARGET}
    ${VBYTE_TARGET}
    ${MDBX_TARGET}
    ${JSON_TARGET}
    ${TIME_SHIELD_TARGET}
    ${GZIP_HPP_TARGET}
    ${FAST_DOUBLE_TARGET}
    ${FAST_FLOAT_TARGET}
)
list(REMOVE_DUPLICATES DFH_ALL_DEP_TARGETS)

# ===== Library target =====
add_library(DataFeedHub INTERFACE)
target_include_directories(DataFeedHub INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(DataFeedHub INTERFACE cxx_std_17)

# ===== Tests =====
if(DFH_BUILD_TESTS)
    enable_testing()

    add_executable(test_dependencies tests/test_dependencies.cpp)
    target_link_libraries(test_dependencies PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
    target_compile_features(test_dependencies PRIVATE cxx_std_17)
    add_test(NAME test_dependencies COMMAND test_dependencies)

    add_executable(test_tick_codec_quote_roundtrip tests/test_tick_codec_quote_roundtrip.cpp)
    target_link_libraries(test_tick_codec_quote_roundtrip PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
    target_compile_features(test_tick_codec_quote_roundtrip PRIVATE cxx_std_17)
    add_test(NAME test_tick_codec_quote_roundtrip COMMAND test_tick_codec_quote_roundtrip)

    add_executable(test_tick_codec_market_roundtrip tests/test_tick_codec_market_roundtrip.cpp)
    target_link_libraries(test_tick_codec_market_roundtrip PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
    target_compile_features(test_tick_codec_market_roundtrip PRIVATE cxx_std_17)
    add_test(NAME test_tick_codec_market_roundtrip COMMAND test_tick_codec_market_roundtrip)

    add_executable(test_tick_codec_quotevol_roundtrip tests/test_tick_codec_quotevol_roundtrip.cpp)
    target_link_libraries(test_tick_codec_quotevol_roundtrip PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
    target_compile_features(test_tick_codec_quotevol_roundtrip PRIVATE cxx_std_17)
    add_test(NAME test_tick_codec_quotevol_roundtrip COMMAND test_tick_codec_quotevol_roundtrip)

    add_executable(test_tick_codec_quotel1_roundtrip tests/test_tick_codec_quotel1_roundtrip.cpp)
    target_link_libraries(test_tick_codec_quotel1_roundtrip PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
    target_compile_features(test_tick_codec_quotel1_roundtrip PRIVATE cxx_std_17)
    add_test(NAME test_tick_codec_quotel1_roundtrip COMMAND test_tick_codec_quotel1_roundtrip)

    add_executable(test_tick_codec_trade_roundtrip tests/test_tick_codec_trade_roundtrip.cpp)
    target_link_libraries(test_tick_codec_trade_roundtrip PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
    target_compile_features(test_tick_codec_trade_roundtrip PRIVATE cxx_std_17)
    add_test(NAME test_tick_codec_trade_roundtrip COMMAND test_tick_codec_trade_roundtrip)

    if(DFH_BUILD_LEGACY_TESTS)
        file(GLOB DFH_TEST_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/*.cpp")
        foreach(test_src IN LISTS DFH_TEST_SOURCES)
            get_filename_component(test_name "${test_src}" NAME_WE)
            if(test_name STREQUAL "test_dependencies")
                continue()
            endif()
            add_executable(${test_name} "${test_src}")
            target_link_libraries(${test_name} PRIVATE DataFeedHub ${DFH_ALL_DEP_TARGETS})
            target_compile_features(${test_name} PRIVATE cxx_std_17)
            add_test(NAME ${test_name} COMMAND ${test_name})
        endforeach()
    endif()
endif()
