# CMake обёртка для библиотеки simdcomp

set(SIMDCOMP_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/avx512bitpacking.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/avxbitpacking.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/simdfor.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/simdcomputil.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/simdbitpacking.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/simdintegratedbitpacking.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/simdpackedsearch.c
    ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/src/simdpackedselect.c
)

add_library(simdcomp STATIC ${SIMDCOMP_SOURCES})
target_include_directories(simdcomp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../simdcomp/include)
target_compile_options(simdcomp PRIVATE -Wall -Wextra -Wshadow -O3 -march=native)
set_target_properties(simdcomp PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)