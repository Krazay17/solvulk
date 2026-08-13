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
set(ENET_TEST OFF CACHE BOOL "" FORCE)
set(CGLM_SHARED OFF CACHE BOOL "" FORCE)
set(CGLM_STATIC ON CACHE BOOL "" FORCE)
# --- Dependencies ---
find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)
add_subdirectory(extern/cglm)
add_subdirectory(extern/enet)
# --- WebP Lib ---
file(GLOB_RECURSE WEBP_SOURCES
    "extern/webp/src/dec/*.c"
    "extern/webp/src/dsp/*.c"
    "extern/webp/src/utils/*.c"
)
add_library(webp_lib STATIC ${WEBP_SOURCES})
target_include_directories(webp_lib PRIVATE "${CMAKE_SOURCE_DIR}/extern/webp")
# --- Engine Target (solvulk) ---
add_library(solvulk "")
# --- Core Sources ---
file(GLOB_RECURSE SOURCES "src/*.c")
list(FILTER SOURCES EXCLUDE REGEX "src/platform/.*")
target_sources(solvulk PRIVATE ${SOURCES})
# Platform Logic
if(WIN32)
    file(GLOB PLATFORM_SOURCES "src/platform/win32/*.c")
    target_sources(solvulk PRIVATE ${PLATFORM_SOURCES} )
    target_compile_definitions(solvulk PUBLIC VK_USE_PLATFORM_WIN32_KHR)
    set(PLATFORM_LIBS user32 gdi32 dwmapi)
else()
    list(FILTER SOURCES EXCLUDE REGEX "src/platform/win32/*.c")
    file(GLOB PLATFORM_SOURCES "src/platform/linux/*.c")
    target_sources(solvulk PRIVATE ${PLATFORM_SOURCES})
    target_compile_definitions(solvulk PUBLIC VK_USE_PLATFORM_XLIB_KHR)
endif()
# Compiler Specifics
if(MSVC)
    target_compile_options(solvulk PRIVATE /std:c11 /W2)
    #set_property(TARGET solvulk PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
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
        "${CMAKE_SOURCE_DIR}/extern"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/core"
        "${CMAKE_SOURCE_DIR}/src/world"
        "${CMAKE_SOURCE_DIR}/include/sol"
        ${Vulkan_INCLUDE_DIRS}
)

if(SOL_DEV_MODE)
    # This pushes the internal headers to anything linking against the engine
    target_compile_definitions(solvulk PUBLIC SOL_DEV_MODE)
    target_include_directories(solvulk 
    PUBLIC 
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/core"
        "${CMAKE_SOURCE_DIR}/src/world"
        "${CMAKE_SOURCE_DIR}/include/sol"
    )
endif()

target_link_libraries(solvulk
    PUBLIC 
        Vulkan::Vulkan
        OpenMP::OpenMP_C
        enet::enet_static
    PRIVATE
        webp_lib
        ${PLATFORM_LIBS}
)

set_target_properties(solvulk PROPERTIES C_STANDARD 11 OUTPUT_NAME "solvulk")

#--------------------------------------
# BUILD APP
#--------------------------------------
#if(WIN32)
#    add_executable(solapp 
#        "${CMAKE_SOURCE_DIR}/app/win_main.c"
#        "${CMAKE_SOURCE_DIR}/app/game.c"
#        "${CMAKE_SOURCE_DIR}/assets/res.rc"
#    )
#    if(MSVC)
#        target_link_options(solapp PRIVATE "/ENTRY:mainCRTStartup")
#        #target_link_options(solapp PRIVATE "$<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>")
#    endif()
#
#else()
#    add_executable(solapp
#        "${CMAKE_SOURCE_DIR}/app/lin_main.c"
#        #"${CMAKE_SOURCE_DIR}/src/game/game.c"
#    )
#endif()
#
#if(HOT_RELOAD)
#    target_include_directories(solapp PRIVATE ${SOL_PUBLIC_INCLUDE})
#else()
#    target_link_libraries(solapp PRIVATE solvulk ${PLATFORM_LIBS})
#endif()
#
#set_target_properties(solapp PROPERTIES 
#    OUTPUT_NAME "SolApp"
#)

#--------------------------------------
# Build App2
#--------------------------------------
if(WIN32)
    add_executable(solapp2
        "${CMAKE_SOURCE_DIR}/app/win_main.c"
        "${CMAKE_SOURCE_DIR}/app/game2.c"
        "${CMAKE_SOURCE_DIR}/assets/res.rc"
    )
    if(MSVC)
        target_link_options(solapp2 PRIVATE "/ENTRY:mainCRTStartup")
    endif()
endif()

target_link_libraries(solapp2 PRIVATE solvulk ${PLATFORM_LIBS})
set_target_properties(solapp2 PROPERTIES
    OUTPUT_NAME "SolApp2"
)
#--------------------------------------
# compile shaders
#--------------------------------------
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/src/core/render/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SHADERS
    ${SHADER_DIR}/model.vert
    ${SHADER_DIR}/model_skinned.vert
    ${SHADER_DIR}/model.frag
    ${SHADER_DIR}/rect.vert
    ${SHADER_DIR}/rect.frag
    ${SHADER_DIR}/recti.vert
    ${SHADER_DIR}/recti.frag
    ${SHADER_DIR}/text.vert
    ${SHADER_DIR}/text.frag
    ${SHADER_DIR}/line.vert
    ${SHADER_DIR}/line.frag
    ${SHADER_DIR}/sphere.vert
    ${SHADER_DIR}/sphere.frag
    ${SHADER_DIR}/skybox.vert
    ${SHADER_DIR}/skybox.frag
    ${SHADER_DIR}/fireball.frag
    
    ${SHADER_DIR}/ribbon.vert
    
    ${SHADER_DIR}/font2d.vert
    ${SHADER_DIR}/font.frag

    ${SHADER_DIR}/quad.vert
    ${SHADER_DIR}/healthbar.frag
    ${SHADER_DIR}/sprite.frag
    ${SHADER_DIR}/text3d.frag
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
#add_dependencies(solapp shaders)
add_dependencies(solapp2 shaders)

