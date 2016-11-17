cmake_minimum_required(VERSION 3.1)
project(StudioKit)

set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

if (NOT TARGET zombie_framework)
    add_subdirectory(../framework ${CMAKE_BINARY_DIR}/build-framework)
endif()

if (STUDIO_KIT_WITH_RENDERING_KIT AND NOT TARGET RenderingKit)
    add_subdirectory(../RenderingKit ${CMAKE_BINARY_DIR}/build-RenderingKit)
endif()

# Also add headers so that they're included in generated projects
file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/StudioKit/blenderimporter.cpp
	${PROJECT_SOURCE_DIR}/src/StudioKit/mapwriter.cpp
    ${PROJECT_SOURCE_DIR}/src/StudioKit/worldgeom.cpp
    ${PROJECT_SOURCE_DIR}/src/StudioKit/*.hpp

    ${PROJECT_SOURCE_DIR}/include/StudioKit/*.hpp
)

if (STUDIO_KIT_WITH_RENDERING_KIT)
	list(APPEND sources ${PROJECT_SOURCE_DIR}/src/StudioKit/startupscreen.cpp)
	list(APPEND sources ${PROJECT_SOURCE_DIR}/src/StudioKit/texturecache.cpp)
endif()

add_library(${library} ${sources})

add_dependencies(${library} zombie_framework)
target_link_libraries(${library} zombie_framework)

if (STUDIO_KIT_WITH_RENDERING_KIT)
	add_dependencies(${library} RenderingKit)
	target_link_libraries(${library} RenderingKit)
endif()

target_compile_definitions(${library} PUBLIC -DSTUDIO_KIT_STATIC=1)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

# Export Assets
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
