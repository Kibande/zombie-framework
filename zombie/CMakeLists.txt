cmake_minimum_required(VERSION 3.1)
project(zombie)

set(CMAKE_CXX_STANDARD 14)

if (EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "-std=c++14 -s USE_FREETYPE=1 -s LEGACY_GL_EMULATION=1 -s DISABLE_EXCEPTION_CATCHING=0")
endif()

file(GLOB_RECURSE sources
    ${PROJECT_SOURCE_DIR}/src/*.cpp

    ${PROJECT_SOURCE_DIR}/src/*.hpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(../framework2012 ${CMAKE_BINARY_DIR}/build-framework)

add_executable(${PROJECT_NAME} ${sources})

add_dependencies(${PROJECT_NAME} framework2012)
target_link_libraries(${PROJECT_NAME} framework2012)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
)

if (SDL_DLL)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_DLL}" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${FRAMEWORK_ASSETS_DIR}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
