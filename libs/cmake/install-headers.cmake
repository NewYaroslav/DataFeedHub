# install_headers.cmake
#
# Утилита для копирования заголовков в build-libs/include/<target_name>/

function(install_headers_to_include target_name include_dir)
    file(GLOB_RECURSE headers
        "${include_dir}/*.h"
        "${include_dir}/*.hpp"
		"${include_dir}/*.ipp"
        "${include_dir}/*.inl"
    )

    if (headers)
        set(output_dir "${CMAKE_BINARY_DIR}/include/${target_name}")
        file(MAKE_DIRECTORY "${output_dir}")
        foreach(header_file IN LISTS headers)
            get_filename_component(rel_path "${header_file}" DIRECTORY)
            file(RELATIVE_PATH rel_path "${include_dir}" "${rel_path}")
            file(MAKE_DIRECTORY "${output_dir}/${rel_path}")
            file(COPY "${header_file}" DESTINATION "${output_dir}/${rel_path}")
        endforeach()

        message(STATUS "Installed headers for ${target_name} → ${output_dir}")
    else()
        message(WARNING "No headers found in '${include_dir}' for '${target_name}'")
    endif()
endfunction()