cmake_minimum_required(VERSION 3.11)
project(rpmalloc
  VERSION 1.4.5
  DESCRIPTION "A library for efficient similarity search and clustering of dense vectors."
  HOMEPAGE_URL "Public domain cross platform lock free thread caching 16-byte aligned memory allocator implemented in C"
  LANGUAGES C CXX)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build Tests" ON)
option(CONFIGURABLE_HEAPS "enable configuring heap sizes" OFF)
option(FIRST_CLASS_HEAPS "Enable first class heaps" OFF)
option(ENABLE_THREAD_CACHE "Enable thread local cache" ON)
option(ENABLE_GLOBAL_CACHE "Enable global cache" ON)
option(ENABLE_VALIDATE_ARGS "Enable validating args" OFF)
option(ENABLE_STATISTICS "Enable allocator statistics" OFF)
option(ENABLE_ASSERTS "Enable asserts" OFF)
option(ENABLE_OVERRIDE "Enable overriding standard library malloc/free and new/delete" OFF)
option(ENABLE_RELOAD "Enable preloading" OFF)
option(DISABLE_UNMAP "Disable unmapping memory pages (also enables unlimited cache)" OFF)
option(ENABLE_UNLIMITED_CACHE "Enable unlimited cache" OFF)
option(ENABLE_ADAPTIVE_THREAD_CACHE "Enable unlimited cache" OFF)

if (CONFIGURABLE_HEAPS)
  add_compile_definitions(RPMALLOC_CONFIGURABLE=1)
endif()

if (FIRST_CLASS_HEAPS)
  add_compile_definitions(RPMALLOC_FIRST_CLASS_HEAPS=1)
endif()

if (ENABLE_THREAD_CACHE)
  add_compile_definitions(ENABLE_THREAD_CACHE=1)
endif()

if (ENABLE_GLOBAL_CACHE)
  add_compile_definitions(ENABLE_GLOBAL_CACHE=1)
endif()

add_subdirectory(rpmalloc)

if (BUILD_TESTS)
  add_subdirectory(test)
endif()
