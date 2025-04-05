# Wrapper для библиотеки vbyte (Makefile → CMake)

set(VBYTE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../libvbyte/vbyte.cc
    ${CMAKE_CURRENT_LIST_DIR}/../libvbyte/varintdecode.c
)

add_library(vbyte STATIC ${VBYTE_SOURCES})
target_include_directories(vbyte PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../libvbyte)
target_compile_options(vbyte PRIVATE -O3 -Wall -Wextra -pedantic -mavx -Wno-shift-count-overflow)
set_target_properties(vbyte PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)