cmake_minimum_required(VERSION 3.1)
project(ContainerDeferredShading)

set(BUILD_SHARED_LIBS OFF)
set(DIST_DIR ${RUNTIME_OUTPUT_DIRECTORY})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "-std=c++14 -s USE_FREETYPE=1 -s USE_SDL=2 -s TOTAL_MEMORY=30000000")
endif()

set(sources
        src/components.cpp
        src/components.hpp
        src/ContainerMapView.hpp
        src/main.cpp
        src/MapSelectionScene.cpp
        src/MapSelectionScene.hpp
        src/MapViewScene.cpp
        src/RenderSystem.cpp
        src/RenderSystem.hpp
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
