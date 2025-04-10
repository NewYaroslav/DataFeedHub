# move_static_output.cmake
# Переносит выходную статическую библиотеку в ${CMAKE_BINARY_DIR}/lib

macro(move_static_output target_name)
    if (TARGET ${target_name})
        message(STATUS "Moving archive output for target: ${target_name}")
        set_target_properties(${target_name} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        )
    else()
        message(WARNING "Target '${target_name}' does not exist. Cannot move output.")
    endif()
endmacro()