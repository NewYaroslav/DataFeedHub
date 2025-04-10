# zlib-ng
set(BUILD_TESTING OFF CACHE BOOL "Disable testing" FORCE)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "Disable test binaries" FORCE)
set(ZLIBNG_BUILD_TESTS OFF CACHE BOOL "Disable zlib-ng test build" FORCE)
set(WITH_GTEST OFF CACHE BOOL "Disable building gtest" FORCE)
set(ZLIBNG_BUILD_SHARED OFF CACHE BOOL "Disable building shared zlib-ng" FORCE)
set(ZLIBNG_BUILD_STATIC ON CACHE BOOL "Enable building static zlib-ng" FORCE)
set(ZLIB_COMPAT ON CACHE BOOL "Turn ON zlib compatibility" FORCE)