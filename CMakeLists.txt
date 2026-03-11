cmake_minimum_required(VERSION 3.20)
project(solvulk C) # Explicitly set C as the language

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 1. Locate Vulkan (CMake has a built-in script for this)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.c")

# 2. Define the executable
# Use WIN32 to tell the linker to look for WinMain instead of main
add_executable(solvulk WIN32 ${SOURCES})

# 3. Add Include Directories
target_include_directories(solvulk PRIVATE 
    "src"
    ${Vulkan_INCLUDE_DIRS}
)

# 4. Link Libraries
# We link Vulkan and the standard Windows user interface library
target_link_libraries(solvulk PRIVATE 
    Vulkan::Vulkan
)

# Optional: Set output directory
set_target_properties(solvulk PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)