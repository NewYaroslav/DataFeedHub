# install_static_libs.cmake
# Копирует все .a, .lib и .def файлы из указанной папки в ${CMAKE_BINARY_DIR}/lib

function(install_static_libs_to_lib lib_dir)
    if (NOT IS_DIRECTORY "${lib_dir}")
        message(WARNING "install_static_libs_to_lib: '${lib_dir}' is not a directory")
        return()
    endif()

    file(GLOB libs 
        "${lib_dir}/*.a"
        "${lib_dir}/*.lib"
        "${lib_dir}/*.def"
    )

    if (NOT libs)
        message(WARNING "No .a, .lib, or .def files found in '${lib_dir}'")
        return()
    endif()

    set(output_dir "${CMAKE_BINARY_DIR}/lib")
    file(MAKE_DIRECTORY "${output_dir}")

    foreach(lib_file IN LISTS libs)
        file(COPY "${lib_file}" DESTINATION "${output_dir}")
        get_filename_component(lib_name "${lib_file}" NAME)
        message(STATUS "Copied ${lib_name} → ${output_dir}")
    endforeach()
endfunction()

function(install_lib_files_to_lib)
    set(output_dir "${CMAKE_BINARY_DIR}/lib")
    file(MAKE_DIRECTORY "${output_dir}")

    foreach(lib_file IN LISTS ARGN)
        if (EXISTS "${lib_file}")
            file(COPY "${lib_file}" DESTINATION "${output_dir}")
            get_filename_component(lib_name "${lib_file}" NAME)
            message(STATUS "Copied ${lib_name} → ${output_dir}")
        else()
            message(WARNING "install_lib_files_to_lib: '${lib_file}' does not exist")
        endif()
    endforeach()
endfunction()