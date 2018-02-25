cmake_minimum_required(VERSION 3.1)
project(ntile)
set(CMAKE_CXX_STANDARD 14)
set(DIST_DIR ${CMAKE_BINARY_DIR})
set(ZOMBIE_API_VERSION 201701)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/ntile/*.cpp
    ${PROJECT_SOURCE_DIR}/src/ntile/entities/*.cpp
    ${PROJECT_SOURCE_DIR}/src/ntile/n3d_gl/n3d_gl_blockmodel.cpp
    ${PROJECT_SOURCE_DIR}/src/ntile/n3d_gl/n3d_gl_blockmodel.hpp
    ${PROJECT_SOURCE_DIR}/src/ntile/n3d_gl/*.cpp
    ${PROJECT_SOURCE_DIR}/src/ntile/studio/*.cpp

    ${PROJECT_SOURCE_DIR}/dependencies/glew-2.0.0/src/glew.c

    ${PROJECT_SOURCE_DIR}/src/ntile/*.hpp
    ${PROJECT_SOURCE_DIR}/src/ntile/entities/*.hpp
    ${PROJECT_SOURCE_DIR}/src/ntile/n3d_gl/*.hpp
    ${PROJECT_SOURCE_DIR}/src/ntile/studio/*.hpp
)

add_subdirectory(../framework ${CMAKE_BINARY_DIR}/build-framework)

add_executable(${PROJECT_NAME} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "nanotile")
target_compile_definitions(${PROJECT_NAME} PRIVATE -DGLEW_STATIC=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DZOMBIE_STUDIO=1)

add_dependencies(${PROJECT_NAME} zombie_framework)
target_link_libraries(${PROJECT_NAME} zombie_framework)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    dependencies/glew-2.0.0/include
)

# Lua
find_package(Lua)

if (LUA_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE SYSTEM ${LUA_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
else()
    add_subdirectory(dependencies/lua ${CMAKE_BINARY_DIR}/build-lua)
    target_include_directories(${PROJECT_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/dependencies/lua/src
            ${CMAKE_BINARY_DIR}/build-lua
            )
    target_link_libraries(${PROJECT_NAME} liblua_static)
endif()

# OpenGL
if (WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
else()
    target_link_libraries(${PROJECT_NAME} GL)
endif()

# SDL2
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
        set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/include)
        set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/lib/libSDL2.dll.a)
        set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/x86_64-w64-mingw32/bin/SDL2.dll)
    else()
        set(SDL2_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/include)
        set(SDL2_LIBRARY ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/lib/libSDL2.dll.a)
        set(SDL2_DLL ${PROJECT_SOURCE_DIR}/dependencies/SDL2-2.0.5/i686-w64-mingw32/bin/SDL2.dll)
    endif()
endif()

find_package(SDL2 REQUIRED)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${SDL2_INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME} "${SDL2_LIBRARY}")

if (SDL2_DLL)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_DLL}" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()

# Copy assets
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" ${DIST_DIR})
