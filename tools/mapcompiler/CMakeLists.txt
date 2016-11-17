cmake_minimum_required(VERSION 3.1)
project(mapcompiler)

set(CMAKE_CXX_STANDARD 14)
set(ZOMBIE_API_VERSION 201701)

file(GLOB_RECURSE sources
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp

	${PROJECT_SOURCE_DIR}/include/*.hpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(../../framework ${CMAKE_BINARY_DIR}/build-framework)
add_subdirectory(../../StudioKit ${CMAKE_BINARY_DIR}/build-StudioKit)

add_executable(${PROJECT_NAME} ${sources})

add_dependencies(${PROJECT_NAME} zombie_framework)
target_link_libraries(${PROJECT_NAME} zombie_framework)

add_dependencies(${PROJECT_NAME} StudioKit)
target_link_libraries(${PROJECT_NAME} StudioKit)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
)
