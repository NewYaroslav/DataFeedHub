# zstd-wrapper.cmake — обёртка для сборки только статической библиотеки zstd

# Отключаем всё лишнее (CLI, программы, тесты, fuzz и т.п.)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)

# Подключаем только cmake-файлы библиотеки
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../zstd/build/cmake EXCLUDE_FROM_ALL)

# Принудительно собираем библиотеку
if (TARGET libzstd_static)
    add_custom_target(force_build_zstd_static ALL DEPENDS libzstd_static)
endif()