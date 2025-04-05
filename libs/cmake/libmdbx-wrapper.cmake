# libmdbx-wrapper.cmake — обёртка для сборки только статической библиотеки libmdbx

# Выключаем всё лишнее
set(MDBX_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(MDBX_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MDBX_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MDBX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(MDBX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MDBX_USE_VALGRIND OFF CACHE BOOL "" FORCE)

# Подключаем проект
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../libmdbx EXCLUDE_FROM_ALL)

# Гарантируем сборку
if (TARGET mdbx-static)
    add_custom_target(force_build_libmdbx ALL DEPENDS mdbx-static)
endif()