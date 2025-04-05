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

set(PROJECT_LIBRARY_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/build-libs/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/build-libs/bin
)

set(PROJECT_DEFINES
    SQLITE_THREADSAFE=1
)

set(PROJECT_LIBS
    mdbx
    zlibstatic
    zstd
    vbyte
    simdcomp
    minizip
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

# Найдём все .cpp файлы в tests/
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
    target_sources(${test_name} PRIVATE ${PROJECT_HEADERS})
endforeach()
