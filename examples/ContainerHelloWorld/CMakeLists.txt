cmake_minimum_required(VERSION 3.1)
project(ContainerHelloWorld)
set(CMAKE_CXX_STANDARD 14)
set(DIST_DIR ${CMAKE_BINARY_DIR})

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

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" ${DIST_DIR})
