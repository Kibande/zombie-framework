cmake_minimum_required(VERSION 3.1)
project(Container3DScene)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 14)
set(DIST_DIR ${RUNTIME_OUTPUT_DIRECTORY})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "-std=c++14 -s USE_FREETYPE=1 -s USE_SDL=2")
endif()

file(GLOB_RECURSE sources
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp
)

add_subdirectory(../../Container ${CMAKE_BINARY_DIR}/build-Container)

add_executable(${PROJECT_NAME} ${sources})

add_dependencies(${PROJECT_NAME} Container)
target_link_libraries(${PROJECT_NAME} Container)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
