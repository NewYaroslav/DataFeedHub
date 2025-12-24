# Проверка, что в include/DataFeedHub отсутствуют обратные относительные include (../ и ..\).

file(GLOB_RECURSE DFH_HEADER_FILES
    "${CMAKE_SOURCE_DIR}/include/DataFeedHub/*.h"
    "${CMAKE_SOURCE_DIR}/include/DataFeedHub/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/DataFeedHub/*.inl"
)

set(DFH_BAD_FILES "")

foreach(header_file IN LISTS DFH_HEADER_FILES)
    file(READ "${header_file}" header_content)

    string(FIND "${header_content}" "../" pos_forward)
    string(FIND "${header_content}" "..\\" pos_backward)

    if(pos_forward GREATER -1 OR pos_backward GREATER -1)
        list(APPEND DFH_BAD_FILES "${header_file}")
    endif()
endforeach()

if(DFH_BAD_FILES)
    string(REPLACE ";" "\n" DFH_BAD_FILES_LIST "${DFH_BAD_FILES}")
    message(FATAL_ERROR "Найдены обратные относительные пути в include. Файлы:\n${DFH_BAD_FILES_LIST}")
endif()
