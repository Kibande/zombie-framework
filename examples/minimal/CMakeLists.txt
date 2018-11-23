cmake_minimum_required(VERSION 3.1)
project(minimal)

set(CMAKE_CXX_STANDARD 14)
set(ZOMBIE_API_VERSION 201901)

add_subdirectory(../../framework ${CMAKE_BINARY_DIR}/build-framework)

add_executable(${PROJECT_NAME} main.cpp)
add_dependencies(${PROJECT_NAME} zombie_framework)
target_link_libraries(${PROJECT_NAME} zombie_framework)
