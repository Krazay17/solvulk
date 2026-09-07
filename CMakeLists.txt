cmake_minimum_required(VERSION 3.20)
project(solvulk C)
# --- Options & Global Settings ---
option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(HOT_RELOAD "Build for hot reload" OFF)

set(BUILD_APP2 OFF CACHE BOOL "" FORCE)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(EXTERN_DIR "${CMAKE_SOURCE_DIR}/extern")

# --- Include Extern Subdirectory ---
#add_subdirectory(extern)

# --- Dependencies ---
find_package(Vulkan REQUIRED)
#find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)

add_library(enet STATIC IMPORTED)
set_target_properties(enet PROPERTIES
    IMPORTED_LOCATION "${EXTERN_DIR}/build/lib/Debug/enet.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${EXTERN_DIR}/enet/include"
)
if(WIN32)
    # ENet relies on Winsock2 and Windows Multimedia timer APIs
    set_target_properties(enet PROPERTIES
        INTERFACE_LINK_LIBRARIES "ws2_32;winmm"
    )
endif()

add_library(webp_lib STATIC IMPORTED)
set_target_properties(webp_lib PROPERTIES
    IMPORTED_LOCATION "${EXTERN_DIR}/build/lib/Debug/webp_lib.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${EXTERN_DIR}/webp"
)
add_library(cglm_headers INTERFACE)
target_include_directories(cglm_headers INTERFACE "${EXTERN_DIR}/cglm/include")

# --- Engine Target (solvulk) ---
add_library(solvulk "")

# --- Core Sources ---
file(GLOB_RECURSE SOURCES CMAKE_CONFIGURE_DEPENDS "src/*.c")
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
    target_compile_options(solvulk PRIVATE /std:c17 /W2 /openmp:experimental)
    #set_property(TARGET solvulk PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(BUILD_SHARED_LIBS)
    target_compile_definitions(solvulk PUBLIC SOL_VULK_SHARED PRIVATE SOL_BUILD_DLL)
endif()


target_include_directories(solvulk 
PUBLIC 
    ${Vulkan_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/sol"
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/core"
    "${CMAKE_SOURCE_DIR}/src/game"
    "${CMAKE_SOURCE_DIR}/src/world"
    "${CMAKE_SOURCE_DIR}/src/world/systems"
    "${CMAKE_SOURCE_DIR}/include/sol"
    "${EXTERN_DIR}"
    "${EXTERN_DIR}/cglm/include"
    "${EXTERN_DIR}/enet/include"
    "${EXTERN_DIR}/webp/src"
)

target_link_libraries(solvulk
    PUBLIC 
        Vulkan::Vulkan
        #OpenMP::OpenMP_C
        enet
        webp_lib
        cglm_headers
        ${PLATFORM_LIBS}
)

set_target_properties(solvulk PROPERTIES C_STANDARD 17 OUTPUT_NAME "solvulk")

#--------------------------------------
# BUILD APP
#--------------------------------------
if(WIN32)
   add_executable(solapp 
       "${CMAKE_SOURCE_DIR}/src/platform/win32/win_main.c"
       "${CMAKE_SOURCE_DIR}/src/game/game.c"
       "${CMAKE_SOURCE_DIR}/assets/res.rc"
   )
   if(MSVC)
       target_link_options(solapp PRIVATE "/ENTRY:mainCRTStartup")
       #target_link_options(solapp PRIVATE "$<$<NOT:$<CONFIG:Debug>>:/SUBSYSTEM:WINDOWS>")
   endif()

else()
   add_executable(solapp
       "${CMAKE_SOURCE_DIR}/src/platform/linux/lin_main.c"
       "${CMAKE_SOURCE_DIR}/src/game/game.c"
   )
endif()

if(HOT_RELOAD)
   target_include_directories(solapp PRIVATE ${SOL_PUBLIC_INCLUDE})
else()
   target_link_libraries(solapp PRIVATE solvulk ${PLATFORM_LIBS})
endif()

set_target_properties(solapp PROPERTIES 
   OUTPUT_NAME "SolApp"
)

#--------------------------------------
# Build App2
#--------------------------------------
if(BUILD_APP2)
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
endif()
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
add_dependencies(solapp shaders)
if(BUILD_APP2)
add_dependencies(solapp2 shaders)
endif()
