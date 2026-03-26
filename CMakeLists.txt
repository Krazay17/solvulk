cmake_minimum_required(VERSION 3.20)
project(solvulk C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Find Vulkan SDK
find_package(Vulkan REQUIRED)
find_package(cglm REQUIRED)
file(GLOB_RECURSE SOURCES "src/*.c")
list(FILTER SOURCES EXCLUDE REGEX "src/app/.*")
set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resources.rc")
set(SOL_PUBLIC_INCLUDE "${CMAKE_SOURCE_DIR}/include")
set(SOL_PRIVATE_INCLUDE  
    "${CMAKE_SOURCE_DIR}/src/core"
    "${CMAKE_SOURCE_DIR}/src/include"
    "${CMAKE_SOURCE_DIR}/src/systems"
    "${CMAKE_SOURCE_DIR}/src/singles"
    "${CMAKE_SOURCE_DIR}/src/utils"
    "${CMAKE_SOURCE_DIR}/src/world"
)

#add_executable(solvulk #WIN32
#    ${SOURCES}
#    ${RESOURCE_FILE}
#)
add_library(solvulk SHARED 
    ${SOURCES}
    ${RESOURCE_FILE}
)


target_include_directories(solvulk
    PUBLIC
        ${SOL_PUBLIC_INCLUDE}
    PRIVATE
        ${SOL_PRIVATE_INCLUDE}
        ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(solvulk PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
    SOL_BUILD_DLL
)

target_link_libraries(solvulk PRIVATE
    Vulkan::Vulkan
    cglm::cglm
    user32
    gdi32
)

set_target_properties(solvulk PROPERTIES
    #WIN32_EXECUTABLE true
    C_STANDARD 11
    OUTPUT_NAME "Solvulk"
)

#--------------------------------------
# Test Exe link to .dll
#--------------------------------------
add_executable(test1
    "${CMAKE_SOURCE_DIR}/src/app/main.c"
)

target_link_libraries(test1 PRIVATE
    solvulk
    cglm::cglm
)
target_include_directories(test1 PRIVATE 
    ${SOL_PUBLIC_INCLUDE}
)
#--------------------------------------
# compile shaders
#--------------------------------------
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SHADERS
    ${SHADER_DIR}/basic3d.vert
    ${SHADER_DIR}/basic3d.frag
    ${SHADER_DIR}/batched_rect.vert
    ${SHADER_DIR}/basicrect.vert
    ${SHADER_DIR}/basicrect.frag
    ${SHADER_DIR}/basictext.vert
    ${SHADER_DIR}/basictext.frag
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