# При генерации Code::Blocks проекта с MinGW можно использовать так:
# cmake -G "CodeBlocks - MinGW Makefiles" -S . -B build-cb
cmake_minimum_required(VERSION 3.18)
project(DataFeedHub LANGUAGES CXX)

# Пути к инклудам и библиотекам
set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/build-libs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Подключаем файлы статичных библиотек
set(PROJECT_LIBRARY_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/build-libs/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/build-libs/bin
)

# Подключаем dll файлы
set(PROJECT_DLL_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/build-libs/bin
)

file(GLOB DLL_FILES "${PROJECT_DLL_DIR}/*.dll")

# Устанавливаем макросы для сборки
set(PROJECT_DEFINES
    SQLITE_THREADSAFE=1
)

# Подключаем необходимые библиотеки
set(PROJECT_LIBS
    mdbx
    zstd
    vbyte
    simdcomp
    minizip
	z
    ntdll
    bcrypt
)

# Заголовочные файлы из include/
file(GLOB_RECURSE PROJECT_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    include/*.hpp
)

# Создаём виртуальную цель для заголовков, чтобы отображались в IDE
add_custom_target(project_headers SOURCES ${PROJECT_HEADERS})

# Находим все .cpp файлы в tests/
file(GLOB TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.cpp)

# Для каждого создаём отдельную цель
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)

    add_executable(${test_name} ${test_src})

    target_include_directories(${test_name} PRIVATE ${PROJECT_INCLUDE_DIRS})
    target_link_directories(${test_name} PRIVATE ${PROJECT_LIBRARY_DIRS})
    target_compile_definitions(${test_name} PRIVATE ${PROJECT_DEFINES})
    target_link_libraries(${test_name} PRIVATE ${PROJECT_LIBS})
	
	# Добавляем заголовочные файлы в IDE-представление цели
	set_source_files_properties(${PROJECT_HEADERS} PROPERTIES HEADER_FILE_ONLY ON)
    target_sources(${test_name} PRIVATE ${PROJECT_HEADERS})
	
	foreach(dll ${DLL_FILES})
        add_custom_command(TARGET ${test_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${dll}"
            "$<TARGET_FILE_DIR:${test_name}>"
        )
    endforeach()
endforeach()
