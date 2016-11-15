cmake_minimum_required(VERSION 3.1)
project(Container)
set(CMAKE_CXX_STANDARD 14)
set(DIST_DIR ${CMAKE_BINARY_DIR})
set(ZOMBIE_API_VERSION 201601)

file(GLOB_RECURSE sources
    ${PROJECT_SOURCE_DIR}/src/Container/*.cpp
	${PROJECT_SOURCE_DIR}/src/Container/*.hpp

    ${PROJECT_SOURCE_DIR}/include/Container/*.hpp
)

add_subdirectory(../framework ${CMAKE_BINARY_DIR}/build-framework)
add_subdirectory(../RenderingKit ${CMAKE_BINARY_DIR}/build-RenderingKit)

add_library(${PROJECT_NAME} ${sources})

add_dependencies(${PROJECT_NAME} zombie_framework)
target_link_libraries(${PROJECT_NAME} zombie_framework)

add_dependencies(${PROJECT_NAME} RenderingKit)
target_link_libraries(${PROJECT_NAME} RenderingKit)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
)

# Export Assets
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
