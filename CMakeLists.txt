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

target_link_options(solvulk PRIVATE
    #"-mwindows"
)

# compile shaders
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SHADERS
    ${SHADER_DIR}/triangle.vert
    ${SHADER_DIR}/triangle.frag
)

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND "$ENV{VULKAN_SDK}/Bin/glslc.exe" ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(solvulk shaders)