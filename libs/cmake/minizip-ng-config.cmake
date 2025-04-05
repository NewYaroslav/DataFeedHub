# minizip-ng-config.cmake
# Конфигурация для сборки minizip-ng с встроенной zlib-ng и базовой функциональностью

# minizip-ng-config.cmake
set(MZ_INSTALL OFF CACHE BOOL "Disable installation" FORCE)
set(MZ_FETCH_LIBS OFF CACHE BOOL "Do not fetch external libs" FORCE)
set(MZ_FORCE_ZLIB ON CACHE BOOL "Use externally provided zlib" FORCE)

# Enable only zlib and PKWARE crypto
set(MZ_ZLIB ON CACHE BOOL "Enable zlib support" FORCE)
set(MZ_PKCRYPT ON CACHE BOOL "Enable PKWARE encryption" FORCE)

# Disable everything else
set(MZ_BZIP2 OFF CACHE BOOL "" FORCE)
set(MZ_LZMA OFF CACHE BOOL "" FORCE)
set(MZ_ZSTD OFF CACHE BOOL "" FORCE)
set(MZ_WZAES OFF CACHE BOOL "" FORCE)
set(MZ_OPENSSL OFF CACHE BOOL "" FORCE)