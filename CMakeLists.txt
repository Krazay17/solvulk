cmake_minimum_required(VERSION 3.20)
project(solvulk C)

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(HOT_RELOAD "Build for hot reload" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(SOL_PUBLIC_INCLUDE "${CMAKE_SOURCE_DIR}/include")
set(SOL_PRIVATE_INCLUDE  
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/include"
)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)
find_package(cglm REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.c")
list(FILTER SOURCES EXCLUDE REGEX "src/platform/.*")

if(WIN32)
    file(GLOB PLATFORM_SOURCES "src/platform/win32/*.c")
    set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resources.rc")
    set(PLATFORM_DEFS VK_USE_PLATFORM_WIN32_KHR)
    list(APPEND PLATFORM_LIBS user32 gdi32)
else()
    file(GLOB PLATFORM_SOURCES "src/platform/linux/*.c")
    set(RESOURCE_FILE "")
    set(PLATFORM_DEFS VK_USE_PLATFORM_XLIB_KHR)
    list(APPEND PLATFORM_LIBS)
endif()

add_library(solvulk ${SOURCES} ${RESOURCE_FILE} ${PLATFORM_SOURCES})

if(BUILD_SHARED_LIBS)
    # Tell the header we are in "Shared Mode"
    target_compile_definitions(solvulk PUBLIC SOL_VULK_SHARED)
    
    # Tell the header that THIS specific target is the one EXPORTING
    target_compile_definitions(solvulk PRIVATE SOL_BUILD_DLL)
endif()

target_include_directories(solvulk
    PUBLIC
        ${SOL_PUBLIC_INCLUDE}
    PRIVATE
        ${SOL_PRIVATE_INCLUDE}
        ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(solvulk PUBLIC ${PLATFORM_DEFS})

target_link_libraries(solvulk
    PUBLIC 
        Vulkan::Vulkan
        cglm::cglm
    PRIVATE
        ${PLATFORM_LIBS}
)

set_target_properties(solvulk PROPERTIES C_STANDARD 11 OUTPUT_NAME "solvulk")

#--------------------------------------
# BUILD APP
#--------------------------------------
set(SOLAPP_INCLUDE "${CMAKE_SOURCE_DIR}/app")
if(WIN32)
    add_executable(solapp #WIN32 
        "${CMAKE_SOURCE_DIR}/app/main.c"
        "${CMAKE_SOURCE_DIR}/app/game.c"
        ${RESOURCE_FILE}
    )
else()
    add_executable(solapp "${CMAKE_SOURCE_DIR}/app/linmain.c")
endif()

target_include_directories(solapp PRIVATE ${SOLAPP_INCLUDE})

if(HOT_RELOAD)
    target_include_directories(solapp PRIVATE ${SOL_PUBLIC_INCLUDE})
else()
    target_link_libraries(solapp PRIVATE solvulk)
endif()

set_target_properties(solapp PROPERTIES OUTPUT_NAME "SolApp")
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
if(WIN32)
set(GLSLC_PATH "$ENV{VULKAN_SDK}/Bin/glslc.exe")
else()
set(GLSLC_PATH "glslc")
endif()

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLC_PATH} ${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})

add_dependencies(solvulk shaders)
add_dependencies(solapp shaders)

#---- Server App ----
add_executable(solvulk_server server/main.c)

target_include_directories(solvulk_server PRIVATE
    "server"
    "vendored/enet/include"
)

if(WIN32)
    target_link_libraries(solvulk_server PRIVATE ws2_32 winmm)
endif()