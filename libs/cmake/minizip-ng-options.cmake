# minizip-ng-options.cmake
# Конфигурация для сборки minizip-ng с встроенной zlib-ng и базовой функциональностью

set(MZ_INSTALL OFF CACHE BOOL "Disable installation" FORCE)
set(MZ_FETCH_LIBS OFF CACHE BOOL "Do not fetch external libs" FORCE)
set(MZ_FORCE_ZLIB ON CACHE BOOL "Use externally provided zlib" FORCE)

# Enable zlib and basic crypto
set(MZ_ZLIB ON CACHE BOOL "Enable zlib support" FORCE)
set(MZ_ZIP64_SUPPORT ON CACHE BOOL "Enable ZIP64 support" FORCE)

# Disable all others
# set(MZ_COMPAT OFF CACHE BOOL "" FORCE)
set(MZ_PKCRYPT OFF CACHE BOOL "Enable PKWARE encryption" FORCE)
set(MZ_BZIP2 OFF CACHE BOOL "" FORCE)
set(MZ_LZMA OFF CACHE BOOL "" FORCE)
set(MZ_ZSTD OFF CACHE BOOL "" FORCE)
set(MZ_WZAES OFF CACHE BOOL "" FORCE)
set(MZ_OPENSSL OFF CACHE BOOL "" FORCE)
set(MZ_ICONV OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(MZ_BUILD_FUZZ_TESTS OFF CACHE BOOL "" FORCE)