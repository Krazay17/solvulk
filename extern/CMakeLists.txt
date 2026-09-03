cmake_minimum_required(VERSION 3.20)
project(solvulk_externs C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)

set(ENET_TEST OFF CACHE BOOL "" FORCE)
set(CGLM_SHARED OFF CACHE BOOL "" FORCE)
set(CGLM_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(cglm)
add_subdirectory(enet)

file(GLOB_RECURSE WEBP_SOURCES
    "webp/src/dec/*.c"
    "webp/src/dsp/*.c"
    "webp/src/utils/*.c"
)
list(FILTER WEBP_SOURCES EXCLUDE REGEX ".*enc.*")

add_library(webp_lib STATIC ${WEBP_SOURCES})
target_include_directories(webp_lib PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/webp/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/webp"
)