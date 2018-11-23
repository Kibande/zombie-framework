cmake_minimum_required(VERSION 3.1)
project(framework2012)

set(WITH_ZTYPE ON CACHE BOOL "Enable ztype (depends on freetype2)")

set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

add_subdirectory(dependencies/libcfx2)
add_subdirectory(dependencies/littl)

# Also add headers so that they're included in generated projects
file(GLOB sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/contentkit/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/studio/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/utility/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameui/dialogs/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameui/utility/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render_opengl/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/zshared/*.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lodepng/lodepng.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/superscanf/superscanf.c

    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/contentkit/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameui/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render_opengl/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ztype/*.hpp
)

list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/render_opengl/video_slave.cpp)

if (NOT EMSCRIPTEN)
    list(APPEND sources ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glew-2.0.0/src/glew.c)
endif()

if (WITH_ZTYPE)
    file(GLOB add_sources src/ztype/*.c src/ztype/*.cpp)
    list(APPEND sources ${add_sources})
endif()

add_library(${library} ${sources})

target_compile_definitions(${library} PRIVATE -DGLEW_STATIC=1)

# freetype2
if (WITH_ZTYPE AND NOT EMSCRIPTEN)
    find_package(Freetype)

    if (Freetype_FOUND)
        target_include_directories(${library} PRIVATE ${FREETYPE_INCLUDE_DIRS})
        target_link_libraries(${library} ${FREETYPE_LIBRARIES})
    else()
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/freetype2)
        add_dependencies(${library} freetype)

        target_include_directories(${library} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/freetype2/include)
        target_link_libraries(${library} freetype)
    endif()
endif()

# jpeg
find_package(JPEG)

if (JPEG_FOUND)
    target_include_directories(${library} PRIVATE ${JPEG_INCLUDE_DIR})
    target_link_libraries(${library} ${JPEG_LIBRARIES})
else()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libjpeg ${CMAKE_CURRENT_BINARY_DIR}/build-libjpeg)
    add_dependencies(${library} jpeg)

    target_include_directories(${library} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libjpeg)
    target_include_directories(${library} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/build-libjpeg)
    target_link_libraries(${library} jpeg)
endif()

target_compile_definitions(${library} PUBLIC -DZOMBIE_WITH_JPEG=1)

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
    dependencies/glm
    dependencies/superscanf
)

if (NOT EMSCRIPTEN)
    # exposed through render.hpp, so must be public
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glew-2.0.0/include)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lodepng)

# OpenGL
if (WIN32)
    target_link_libraries(${library} opengl32)
else()
    target_link_libraries(${library} GL)
endif()

# SDL
if (NOT EMSCRIPTEN)
    if (MSVC)
        set(SDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/include)
        if (CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(SDL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/x86/SDL.lib)
            set(SDL_DLL ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/x86/SDL.dll PARENT_SCOPE)
        else()
            set(SDL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/x64/SDL.lib)
            set(SDL_DLL ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/x64/SDL.dll PARENT_SCOPE)
        endif()
    endif()

    if (WIN32 AND NOT MSVC)
        set(SDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/include)
        if (CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(SDL_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/lib/libSDL.dll.a)
            set(SDL_DLL ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/SDL-1.2.15/bin/SDL.dll)
        endif()
    endif()

    find_package(SDL REQUIRED)

    target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${SDL_INCLUDE_DIR}")
    target_link_libraries(${PROJECT_NAME} "${SDL_LIBRARY}")
endif()

# Export Assets
set(FRAMEWORK_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets PARENT_SCOPE)
