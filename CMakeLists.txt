cmake_minimum_required(VERSION 3.1)
project(zombie_framework)

set(WITH_JPEG ON CACHE BOOL "Enable JPEG support (requires jpeglib)")
set(WITH_LODEPNG ON CACHE BOOL "Enable PNG support via LodePNG")

set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

add_subdirectory(dependencies/libcfx2)

file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/framework/*.cpp
    ${PROJECT_SOURCE_DIR}/src/framework/utility/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/*.cpp
    ${PROJECT_SOURCE_DIR}/src/zshared/*.c
    ${PROJECT_SOURCE_DIR}/src/zshared/*.cpp
)

list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/bmpcodec.cpp)

if (UNIX AND NOT APPLE)
    file(GLOB add_sources src/framework/linux/*)
    list(APPEND sources ${add_sources})
endif()

if (WITH_LODEPNG)
    add_definitions(-DZOMBIE_WITH_LODEPNG=1)
    list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/lodepngcodec.cpp)
endif()

if (WIN32)
    file(GLOB add_sources src/framework/win32/*)
    list(APPEND sources ${add_sources})
endif()

add_library(${library} ${sources})
add_dependencies(${library} cfx2)
target_link_libraries(${library} cfx2)

if (UNIX)
    target_link_libraries(${library} dl)
    target_link_libraries(${library} pthread)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
        include
        dependencies/glm
        dependencies/littl
        dependencies/reflector/include
)

if (WITH_LODEPNG)
    target_include_directories(${PROJECT_NAME} PRIVATE dependencies/lodepng)
endif()

# TODO: use CMake's facilities for this
add_custom_target(sdk)
add_custom_command(TARGET sdk POST_BUILD
    MAIN_DEPENDENCY ${library}

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist

    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/include
            ${CMAKE_BINARY_DIR}/dist/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/dependencies/glm
            ${CMAKE_BINARY_DIR}/dist/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/dependencies/littl
            ${CMAKE_BINARY_DIR}/dist/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/dependencies/reflector/include
            ${CMAKE_BINARY_DIR}/dist/include

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/lib
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_NAME:${library}> ${CMAKE_BINARY_DIR}/dist/lib
)
