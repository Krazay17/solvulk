cmake_minimum_required(VERSION 3.20)
project(solvulk C)
# --- Options & Global Settings ---
option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(HOT_RELOAD "Build for hot reload" OFF)
option(SOL_DEV_MODE "Build dev mode" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# --- Dependencies ---
find_package(Vulkan REQUIRED)
find_package(cglm REQUIRED)
find_package(OpenMP REQUIRED)
# --- Engine Target (solvulk) ---
add_library(solvulk "")
# --- Core Sources ---
file(GLOB_RECURSE SOURCES "src/*.c")
list(FILTER SOURCES EXCLUDE REGEX "src/platform/.*")
target_sources(solvulk PRIVATE ${SOURCES})
# Platform Logic
if(WIN32)
    file(GLOB PLATFORM_SOURCES "src/platform/win32/*.c")
    target_sources(solvulk PRIVATE ${PLATFORM_SOURCES})
    target_compile_definitions(solvulk PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    set(PLATFORM_LIBS user32 gdi32 dwmapi)
    target_link_libraries(solvulk PRIVATE ${PLATFORM_LIBS})
else()
    file(GLOB PLATFORM_SOURCES "src/platform/linux/*.c")
    target_sources(solvulk PRIVATE ${PLATFORM_SOURCES})
    target_compile_definitions(solvulk PUBLIC VK_USE_PLATFORM_XLIB_KHR)
endif()
# Compiler Specifics
if(MSVC)
    target_compile_options(solvulk PRIVATE /std:c11)
    set_property(TARGET solvulk PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(BUILD_SHARED_LIBS)
    target_compile_definitions(solvulk PUBLIC SOL_VULK_SHARED PRIVATE SOL_BUILD_DLL)
endif()

if(SOL_DEV_MODE)
    target_compile_definitions(solvulk PUBLIC SOL_DEV_MODE)
    target_include_directories(solvulk PUBLIC "${CMAKE_SOURCE_DIR}/src/_internal")
endif()

target_include_directories(solvulk
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/_internal"
        ${Vulkan_INCLUDE_DIRS}
)

if(SOL_DEV_MODE)
    # This pushes the internal headers to anything linking against the engine
    target_compile_definitions(solvulk PUBLIC SOL_DEV_MODE)
    target_include_directories(solvulk 
    PUBLIC 
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/_internal"
    )
endif()

target_link_libraries(solvulk
    PUBLIC 
        Vulkan::Vulkan
        cglm::cglm
        OpenMP::OpenMP_C
    PRIVATE
        ${PLATFORM_LIBS}
)

set_target_properties(solvulk PROPERTIES C_STANDARD 11 OUTPUT_NAME "solvulk")

#--------------------------------------
# BUILD APP
#--------------------------------------
if(WIN32)
    add_executable(solapp
        "${CMAKE_SOURCE_DIR}/app/main.c"
        "${CMAKE_SOURCE_DIR}/app/game.c"
        "${CMAKE_SOURCE_DIR}/resources.rc"
    )
    #target_link_options(solapp PRIVATE "$<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>")
else()
    add_executable(solapp "${CMAKE_SOURCE_DIR}/app/linmain.c")
endif()

target_include_directories(solapp PRIVATE "${CMAKE_SOURCE_DIR}/app")

if(HOT_RELOAD)
    target_include_directories(solapp PRIVATE ${SOL_PUBLIC_INCLUDE})
else()
    target_link_libraries(solapp PRIVATE solvulk ${PLATFORM_LIBS})
endif()

set_target_properties(solapp PROPERTIES 
    OUTPUT_NAME "SolApp"
    LINK_FLAGS "/ENTRY:mainCRTStartup"
)
#--------------------------------------
# compile shaders
#--------------------------------------
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SHADERS
    ${SHADER_DIR}/model.vert
    ${SHADER_DIR}/model.frag
    ${SHADER_DIR}/rect.vert
    ${SHADER_DIR}/rect.frag
    ${SHADER_DIR}/text.vert
    ${SHADER_DIR}/text.frag
    ${SHADER_DIR}/line.vert
    ${SHADER_DIR}/line.frag
    ${SHADER_DIR}/sphere.vert
    ${SHADER_DIR}/sphere.frag
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
add_executable(solvulk_server src_server/main.c)

target_include_directories(solvulk_server 
PRIVATE
    "src_server"
    "vendored/enet/include"
)

if(WIN32)
    target_link_libraries(solvulk_server PRIVATE ws2_32 winmm)
endif()

set_target_properties(solvulk_server PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
