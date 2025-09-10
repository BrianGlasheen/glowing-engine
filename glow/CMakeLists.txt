cmake_minimum_required(VERSION 3.15)
project(CORE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(ENABLE_FMOD "enable fmod" OFF)
if(ENABLE_FMOD)
    add_compile_definitions(USE_FMOD)
endif()

set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(USE_AVX2 ON CACHE BOOL "" FORCE)
set(USE_AVX ON CACHE BOOL "" FORCE)
set(USE_SSE4_1 ON CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
set(USE_LZCNT ON CACHE BOOL "" FORCE)
set(USE_TZCNT ON CACHE BOOL "" FORCE)
set(USE_F16C ON CACHE BOOL "" FORCE)
set(USE_FMADD ON CACHE BOOL "" FORCE)
add_subdirectory(ext/JoltPhysics-5.3.0/Build)

# Add GLFW (modify path to your built GLFW folder)
add_subdirectory(ext/glfw-3.4)
add_subdirectory(ext/assimp)

# Update path for FMOD on Windows
set(FMOD_ROOT "${CMAKE_SOURCE_DIR}/ext/FMOD Studio API Windows")

# Include directories
include_directories(
    ext
    ext/glfw-3.4/include
    ext/assimp/include
    ext/JoltPhysics-5.3.0
    "${FMOD_ROOT}/api/core/inc"  # Updated FMOD include path
    src
    src/general
)

set(SOURCES
    ext/stb_image_impl.cpp
    "src/main.cpp"
    "src/glow.cpp"
    
    "src/asset/compute_shader.cpp"
    "src/asset/crosshair.cpp"
    #"src/asset/mesh.cpp"
    #"src/asset/model.cpp"
    "src/asset/material_manager.cpp"
    "src/asset/model_indirect.cpp"
    "src/asset/particle_manager.cpp"
    "src/asset/model_manager.cpp"
    "src/asset/shader.cpp"
    "src/asset/shader_manager.cpp"
    "src/asset/skybox.cpp"
    "src/asset/text.cpp"
    "src/asset/texture_manager.cpp"

    "src/core/audio.cpp"
    "src/core/editor.cpp"
    "src/core/entity.cpp"
    "src/core/light.cpp"
    "src/core/physics.cpp"
    "src/core/renderer.cpp"
    "src/core/renderer_debug.cpp"
    "src/core/scene.cpp"
    "src/core/ssbo.cpp"
    "src/core/terrain.cpp"
    "src/core/window.cpp"

    "src/player/player.cpp"

    "ext/glad/glad.c"
    "ext/dearimgui/imgui.cpp"
    "ext/dearimgui/imgui_draw.cpp"
    "ext/dearimgui/imgui_widgets.cpp"
    "ext/dearimgui/imgui_tables.cpp"
    "ext/dearimgui/imgui_impl_glfw.cpp"
    "ext/dearimgui/imgui_impl_opengl3.cpp"
    "ext/imguizmo/ImGuizmo.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Find FMOD library based on platform and architecture
if(ENABLE_FMOD)
    if(WIN32)
        # Check if we're building for 32-bit or 64-bit architecture
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(FMOD_LIB_DIR "${FMOD_ROOT}/api/core/lib/x64")
        else()
            set(FMOD_LIB_DIR "${FMOD_ROOT}/api/core/lib/x86")
        endif()
        
        # Find FMOD library (Windows uses .dll and needs .lib for linking)
        find_library(FMOD_LIBRARY 
            NAMES fmod_vc fmod64_vc  # Windows library naming
            PATHS ${FMOD_LIB_DIR}
            NO_DEFAULT_PATH
        )
    else()
        # Original Linux path
        find_library(FMOD_LIBRARY 
            NAMES fmod
            PATHS ${CMAKE_SOURCE_DIR}/ext/fmodstudioapi20307linux/api/core/lib/x86_64
            NO_DEFAULT_PATH
        )
    endif()
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    glfw ${GLFW_LIBRARIES}
    assimp
    $<$<BOOL:${ENABLE_FMOD}>:${FMOD_LIBRARY}>
    Jolt
)

# Copy FMOD DLL to output directory (for Windows)
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit
        file(GLOB FMOD_DLLS "${FMOD_ROOT}/api/core/lib/x64/*.dll")
    else()
        # 32-bit
        file(GLOB FMOD_DLLS "${FMOD_ROOT}/api/core/lib/x86/*.dll")
    endif()
    
    foreach(DLL ${FMOD_DLLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
endif()