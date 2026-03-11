cmake_minimum_required(VERSION 3.20)
project(solvulk C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Find Vulkan SDK
find_package(Vulkan REQUIRED)
file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(solvulk
    ${SOURCES}
)

target_include_directories(solvulk PRIVATE
    ${CMAKE_SOURCE_DIR}
    src
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(solvulk PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
)

target_link_libraries(solvulk PRIVATE
    ${Vulkan_LIBRARIES}
    user32
    gdi32
)