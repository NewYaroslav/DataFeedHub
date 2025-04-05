# sqlite-wrapper.cmake

set(SQLITE_VERSION 3460000)
set(SQLITE_DIR ${CMAKE_BINARY_DIR}/sqlite)
set(SQLITE_URL "https://www.sqlite.org/2024/sqlite-amalgamation-${SQLITE_VERSION}.zip")
set(SQLITE_ZIP "${CMAKE_BINARY_DIR}/sqlite.zip")

if (NOT EXISTS "${SQLITE_DIR}/sqlite3.c")
    message(STATUS "Downloading SQLite ${SQLITE_VERSION}...")
    
	file(DOWNLOAD
        ${SQLITE_URL}
        ${SQLITE_ZIP}
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
        INACTIVITY_TIMEOUT 30
    )

    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
    if (NOT DOWNLOAD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download SQLite. Code: ${DOWNLOAD_RESULT}")
    endif()

    message(STATUS "Unpacking SQLite...")

    file(MAKE_DIRECTORY ${SQLITE_DIR})

	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar xf ${SQLITE_ZIP}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		RESULT_VARIABLE UNZIP_RESULT
	)

    if (NOT UNZIP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to unpack SQLite zip (unsupported format?). Try unpacking manually.")
    endif()

    # Найдём распакованную папку
    file(GLOB SQLITE_UNPACKED_DIRS "${CMAKE_BINARY_DIR}/sqlite-amalgamation-*")
	list(LENGTH SQLITE_UNPACKED_DIRS SQLITE_DIR_COUNT)

	if(NOT SQLITE_DIR_COUNT EQUAL 1)
		message(FATAL_ERROR "Expected exactly one sqlite-amalgamation-* directory, found ${SQLITE_DIR_COUNT}")
	endif()

	list(GET SQLITE_UNPACKED_DIRS 0 SQLITE_UNPACKED_DIR)

	list(LENGTH SQLITE_UNPACKED_DIRS DIR_COUNT)
    if (DIR_COUNT EQUAL 0)
        message(FATAL_ERROR "Failed to find unpacked SQLite directory.")
    elseif (DIR_COUNT GREATER 1)
        message(WARNING "Multiple unpacked directories found. Using the first one.")
    endif()
	
	list(GET SQLITE_UNPACKED_DIRS 0 SQLITE_UNPACKED_DIR)

    if (NOT EXISTS "${SQLITE_DIR}/sqlite3.c")
		file(REMOVE_RECURSE ${SQLITE_DIR})
        file(RENAME "${SQLITE_UNPACKED_DIR}" "${SQLITE_DIR}")
    endif()
endif()

add_library(sqlite3 STATIC
    ${SQLITE_DIR}/sqlite3.c
)

target_include_directories(sqlite3 PUBLIC
    ${SQLITE_DIR}
)

target_compile_definitions(sqlite3 PUBLIC
    SQLITE_THREADSAFE=1
    SQLITE_OMIT_LOAD_EXTENSION
)

set_target_properties(sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
