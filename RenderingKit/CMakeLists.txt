cmake_minimum_required(VERSION 3.1)
project(RenderingKit)

set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (NOT TARGET zombie_framework)
    add_subdirectory(../framework ${CMAKE_BINARY_DIR}/build-framework)
endif()

# Also add headers so that they're included in generated projects
file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/RenderingKit/*.cpp
    ${PROJECT_SOURCE_DIR}/src/RenderingKit/gameui/*.cpp

    ${PROJECT_SOURCE_DIR}/dependencies/glew-2.0.0/src/glew.c

    ${PROJECT_SOURCE_DIR}/include/RenderingKit/*.hpp
    ${PROJECT_SOURCE_DIR}/include/RenderingKit/gameui/*.hpp
    ${PROJECT_SOURCE_DIR}/include/RenderingKit/utility/*.hpp
)

add_library(${library} ${sources})
add_dependencies(${library} zombie_framework)
target_link_libraries(${library} zombie_framework)

target_compile_definitions(${library} PUBLIC -DRENDERING_KIT_STATIC=1)
target_compile_definitions(${library} PRIVATE -DGLEW_STATIC=1)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    dependencies/glew-2.0.0/include
)

# OpenGL
if (WIN32)
    target_link_libraries(${library} opengl32)
else()
    target_link_libraries(${library} GL)
endif()

# SDL2
if (MSVC)
    set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/include)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x64/SDL2.lib)
        set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x64/SDL2.dll PARENT_SCOPE)
    else()
        set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x86/SDL2.lib)
        set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/lib/x86/SDL2.dll PARENT_SCOPE)
    endif()
endif()

if (WIN32 AND NOT MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/include)
        set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/lib/libSDL2.dll.a)
        set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/bin/SDL2.dll PARENT_SCOPE)
    else()
        set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/include)
        set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/lib/libSDL2.dll.a)
        set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/bin/SDL2.dll PARENT_SCOPE)
    endif()
endif()

find_package(SDL2 REQUIRED)

target_include_directories(${library} SYSTEM PRIVATE "${SDL2_INCLUDE_DIR}")
target_link_libraries(${library} "${SDL2_LIBRARY}")

# Export Assets
set(RENDERING_KIT_ASSETS_DIR ${PROJECT_SOURCE_DIR}/assets PARENT_SCOPE)
