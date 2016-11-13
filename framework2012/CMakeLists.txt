cmake_minimum_required(VERSION 3.1)
project(framework2012)

set(WITH_ZTYPE ON CACHE BOOL "Enable ztype (depends on freetype2)")

set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

add_subdirectory(dependencies/freetype2)
add_subdirectory(dependencies/libcfx2)
add_subdirectory(dependencies/littl)
add_subdirectory(dependencies/libjpeg)

# Also add headers so that they're included in generated projects
file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/contentkit/*.cpp
    ${PROJECT_SOURCE_DIR}/src/framework/*.cpp
    ${PROJECT_SOURCE_DIR}/src/framework/studio/*.cpp
    ${PROJECT_SOURCE_DIR}/src/framework/utility/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/dialogs/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/utility/*.cpp
    ${PROJECT_SOURCE_DIR}/src/render_opengl/*.cpp
    ${PROJECT_SOURCE_DIR}/src/zshared/*.cpp

    ${PROJECT_SOURCE_DIR}/dependencies/glew-2.0.0/src/glew.c
    ${PROJECT_SOURCE_DIR}/dependencies/lodepng/lodepng.cpp
    ${PROJECT_SOURCE_DIR}/dependencies/superscanf/superscanf.c

    ${PROJECT_SOURCE_DIR}/src/*.h
    ${PROJECT_SOURCE_DIR}/src/contentkit/*.hpp
    ${PROJECT_SOURCE_DIR}/src/framework/*.hpp
    ${PROJECT_SOURCE_DIR}/src/gameui/*.hpp
    ${PROJECT_SOURCE_DIR}/src/render_opengl/*.hpp
    ${PROJECT_SOURCE_DIR}/src/ztype/*.hpp
)

list(REMOVE_ITEM sources ${PROJECT_SOURCE_DIR}/src/render_opengl/video_slave.cpp)

if (WITH_ZTYPE)
    file(GLOB add_sources src/ztype/*.c src/ztype/*.cpp)
    list(APPEND sources ${add_sources})
endif()

add_library(${library} ${sources})

target_compile_definitions(${library} PRIVATE -DGLEW_STATIC=1)

# freetype2
if (WITH_ZTYPE)
    add_dependencies(${library} freetype)
    target_link_libraries(${library} freetype)
endif()

# jpeg
add_dependencies(${library} jpeg)
target_compile_definitions(${library} PUBLIC -DZOMBIE_WITH_JPEG=1)
target_link_libraries(${library} jpeg)

# libcfx2
add_dependencies(${library} cfx2)
target_link_libraries(${library} cfx2)

# littl
add_dependencies(${library} littl)
target_link_libraries(${library} littl)

# lodepng
target_compile_definitions(${library} PRIVATE -DZOMBIE_WITH_LODEPNG=1)

if (UNIX)
    target_link_libraries(${library} dl)
    target_link_libraries(${library} pthread)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    src
    dependencies/glew-2.0.0/include
    dependencies/glm
    dependencies/superscanf
)

target_include_directories(${PROJECT_NAME} PRIVATE dependencies/lodepng)

# OpenGL
if (WIN32)
    target_link_libraries(${library} opengl32)
else()
    target_link_libraries(${library} GL)
endif()

# SDL
if (MSVC)
    set(SDL_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL-1.2.15/include)
    set(SDL_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/x86/SDL.lib)
    set(SDL_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/x86/SDL.dll PARENT_SCOPE)
endif()

find_package(SDL REQUIRED)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${SDL_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} "${SDL_LIBRARY}")

# Export Assets
set(FRAMEWORK_ASSETS_DIR ${PROJECT_SOURCE_DIR}/assets PARENT_SCOPE)
