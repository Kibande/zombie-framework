cmake_minimum_required(VERSION 3.1)
project(RenderingKit)

set(CMAKE_CXX_STANDARD 14)

set(RENDERING_KIT_USE_OPENGL_ES_SUBSET OFF CACHE BOOL "Use OpenGL ES-compatible subset")

if (RENDERING_KIT_USE_OPENGL_ES_SUBSET OR EMSCRIPTEN)
    set(USING_OPENGL_ES ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (NOT TARGET zombie_framework)
    add_subdirectory(../framework ${CMAKE_BINARY_DIR}/build-framework)
endif()

# Also add headers so that they're included in generated projects
file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/RenderingKit/*.cpp
    ${PROJECT_SOURCE_DIR}/src/RenderingKit/*.hpp
    ${PROJECT_SOURCE_DIR}/src/RenderingKit/gameui/*.cpp

    ${PROJECT_SOURCE_DIR}/include/RenderingKit/*.hpp
    ${PROJECT_SOURCE_DIR}/include/RenderingKit/gameui/*.hpp
    ${PROJECT_SOURCE_DIR}/include/RenderingKit/utility/*.hpp
)

if (USING_OPENGL_ES)
    list(REMOVE_ITEM sources ${PROJECT_SOURCE_DIR}/src/RenderingKit/DeferredShadingManager.cpp)
endif()

list(APPEND sources
        include/RenderingKit/utility/ShaderGlobal.hpp
        include/RenderingKit/utility/VertexFormat.hpp

        src/RenderingKit/GlobalCache.cpp
        src/RenderingKit/wm/WindowManager_SDL2.cpp
        )

if (NOT EMSCRIPTEN)
    list(APPEND sources ${PROJECT_SOURCE_DIR}/dependencies/glew-2.0.0/src/glew.c)
endif()

add_library(${PROJECT_NAME} STATIC ${sources})
add_dependencies(${PROJECT_NAME} zombie_framework)
target_link_libraries(${PROJECT_NAME} zombie_framework)

target_compile_definitions(${PROJECT_NAME} PUBLIC RENDERING_KIT_STATIC=1)
target_compile_definitions(${PROJECT_NAME} PUBLIC STX_NO_STD_VARIANT=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC=1)

if (USING_OPENGL_ES)
    target_compile_definitions(${PROJECT_NAME} PUBLIC RENDERING_KIT_USING_OPENGL_ES)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

if (NOT EMSCRIPTEN)
    target_include_directories(${PROJECT_NAME} PRIVATE dependencies/glew-2.0.0/include)
endif()

# OpenGL
if (WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
else()
    target_link_libraries(${PROJECT_NAME} GL)
endif()

# SDL2
if (NOT EMSCRIPTEN)
    if (MSVC)
        set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/include)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x64/SDL2.lib)
            set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x64/SDL2.dll)
        else()
            set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x86/SDL2.lib)
            set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x86/SDL2.dll)
        endif()
    endif()

    if (WIN32 AND NOT MSVC)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/include/SDL2)
            set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/lib/libSDL2.dll.a)
            set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/bin/SDL2.dll)
        else()
            set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/include/SDL2)
            set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/lib/libSDL2.dll.a)
            set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/bin/SDL2.dll)
        endif()
    endif()

    if (SDL2_DLL)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_DLL}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
    endif()

    find_package(SDL2 REQUIRED)

    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${SDL2_INCLUDE_DIR}")
    target_link_libraries(${PROJECT_NAME} "${SDL2_LIBRARY}")
endif()

if((CMAKE_CXX_COMPILER_ID MATCHES Clang) OR (CMAKE_CXX_COMPILER_ID STREQUAL GNU))
    target_compile_options(${PROJECT_NAME} PRIVATE "-Werror=narrowing" "-Werror=switch")
endif()

# Export Assets
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
