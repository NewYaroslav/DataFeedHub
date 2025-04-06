cmake_minimum_required(VERSION 3.18)
project(LibsProject LANGUAGES C CXX)

set(LIBS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(${LIBS_CMAKE_DIR}/move_static_output.cmake)
include(${LIBS_CMAKE_DIR}/install_headers.cmake)
include(${LIBS_CMAKE_DIR}/install_static_libs.cmake)
include(${LIBS_CMAKE_DIR}/install_runtime_dlls.cmake)

# zlib
# add_subdirectory(zlib) # соберётся как zlibstatic
# move_static_output(zlibstatic)
# install_headers_to_include(zlib ${CMAKE_CURRENT_LIST_DIR}/zlib)

# zlib-ng
include(${LIBS_CMAKE_DIR}/zlib-ng-config.cmake)
add_subdirectory(zlib-ng)
move_static_output(zlibstatic)
install_headers_to_include("." ${CMAKE_BINARY_DIR}/zlib-ng)

# Удалить старые значения переменных, если вдруг что-то "нашлось"
unset(ZLIB_FOUND CACHE)
unset(ZLIB_INCLUDE_DIRS CACHE)
unset(ZLIB_LIBRARIES CACHE)
unset(ZLIB_VERSION_STRING CACHE)
unset(ZLIB_INCLUDE_DIR CACHE)
unset(ZLIB_LIBRARY CACHE)

# Сообщаем minizip-ng, что нашли zlib
set(ZLIB_FOUND TRUE CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng CACHE INTERNAL "")
set(ZLIB_LIBRARIES zlibstatic CACHE INTERNAL "")
set(ZLIB_VERSION_STRING "2.1.6" CACHE INTERNAL "")

# Альтернативные имена для совместимости
set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIRS} CACHE INTERNAL "")
set(ZLIB_LIBRARY ${ZLIB_LIBRARIES} CACHE INTERNAL "")

# minizip-ng
include(${LIBS_CMAKE_DIR}/minizip-ng-config.cmake)
add_subdirectory(minizip-ng)
target_link_libraries(minizip PRIVATE zlibstatic)
move_static_output(minizip)
install_headers_to_include("." ${CMAKE_CURRENT_LIST_DIR}/minizip-ng)

# simdcomp
include(${LIBS_CMAKE_DIR}/simdcomp-wrapper.cmake)
install_headers_to_include("." ${CMAKE_CURRENT_LIST_DIR}/simdcomp/include)

# vbyte
include(${LIBS_CMAKE_DIR}/vbyte-wrapper.cmake)
install_headers_to_include("." ${CMAKE_CURRENT_LIST_DIR}/libvbyte)

# zstd
include(${LIBS_CMAKE_DIR}/zstd-wrapper.cmake)
move_static_output(libzstd_static)
install_headers_to_include("." ${CMAKE_CURRENT_LIST_DIR}/zstd/lib)

# mdbx
include(${LIBS_CMAKE_DIR}/libmdbx-wrapper.cmake)
move_static_output(mdbx-static)
install_headers_to_include("." ${CMAKE_CURRENT_LIST_DIR}/libmdbx)

# sqlite
include(${LIBS_CMAKE_DIR}/sqlite-wrapper.cmake)
move_static_output(sqlite3)
install_headers_to_include("." ${CMAKE_CURRENT_LIST_DIR}/sqlite)

# openssl
# install_headers_to_include(openssl ${CMAKE_CURRENT_LIST_DIR}/openssl-win64-v3.4.0/include/openssl)
# install_static_libs_to_lib(${CMAKE_CURRENT_LIST_DIR}/openssl-win64-v3.4.0/lib/VC/x64/MT)
# install_runtime_dlls_to_bin (${CMAKE_CURRENT_LIST_DIR}/openssl-win64-v3.4.0/bin)

# curl
# install_headers_to_include(curl ${CMAKE_CURRENT_LIST_DIR}/curl-8.11.0_1-win64-mingw/include/curl)
# install_static_libs_to_lib(${CMAKE_CURRENT_LIST_DIR}/curl-8.11.0_1-win64-mingw/lib)
# install_runtime_dlls_to_bin (${CMAKE_CURRENT_LIST_DIR}/curl-8.11.0_1-win64-mingw/bin)

# header-only
install_headers_to_include(time_shield_cpp ${CMAKE_CURRENT_LIST_DIR}/time-shield-cpp/include/time_shield_cpp)
# install_headers_to_include(log-it          ${CMAKE_CURRENT_LIST_DIR}/log-it-cpp/include/log-it)
# install_headers_to_include("."             ${CMAKE_CURRENT_LIST_DIR}/asio/asio/include)
install_headers_to_include(gzip            ${CMAKE_CURRENT_LIST_DIR}/gzip-hpp/include/gzip)
install_headers_to_include(nlohmann        ${CMAKE_CURRENT_LIST_DIR}/json/include/nlohmann)
# install_headers_to_include(Simple-WebSocket-Server ${CMAKE_CURRENT_LIST_DIR}/Simple-WebSocket-Server)
# install_headers_to_include(kurlyk          ${CMAKE_CURRENT_LIST_DIR}/kurlyk/include)
install_headers_to_include("."             ${CMAKE_CURRENT_LIST_DIR}/fast_double_parser/include)
install_headers_to_include(fast_float      ${CMAKE_CURRENT_LIST_DIR}/fast_float/include)

add_custom_target(all_libs_built ALL
	COMMENT "All libraries and headers have been prepared."
)
