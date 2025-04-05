# install_runtime_dlls.cmake
# Копирует все .dll файлы из указанной директории в ${CMAKE_BINARY_DIR}/bin

function(install_runtime_dlls_to_bin dll_dir)
    if (NOT IS_DIRECTORY "${dll_dir}")
        message(WARNING "install_runtime_dlls_to_bin: '${dll_dir}' is not a directory")
        return()
    endif()

    file(GLOB dlls "${dll_dir}/*.dll")
    if (NOT dlls)
        message(WARNING "No .dll files found in '${dll_dir}'")
        return()
    endif()

    set(output_dir "${CMAKE_BINARY_DIR}/bin")
    file(MAKE_DIRECTORY "${output_dir}")
    foreach(dll_file IN LISTS dlls)
        file(COPY "${dll_file}" DESTINATION "${output_dir}")
        get_filename_component(dll_name "${dll_file}" NAME)
        message(STATUS "Copied ${dll_name} → ${output_dir}")
    endforeach()
endfunction()