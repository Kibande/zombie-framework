cmake_minimum_required(VERSION 3.1)
project(zombie_framework)

set(WITH_BLEB ON CACHE BOOL "Enable Bleb support")
set(WITH_JPEG ON CACHE BOOL "Enable JPEG support (requires libjpeg)")
set(WITH_LODEPNG ON CACHE BOOL "Enable PNG support via LodePNG")
set(WITH_ZTYPE ON CACHE BOOL "Enable ztype (depends on freetype2)")

set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

add_subdirectory(dependencies/bleb)
add_subdirectory(dependencies/libcfx2)
add_subdirectory(dependencies/littl)

# Also add headers so that they're included in generated projects
file(GLOB sources
    ${PROJECT_SOURCE_DIR}/src/framework/*.cpp
    ${PROJECT_SOURCE_DIR}/src/framework/studio/*.cpp
    ${PROJECT_SOURCE_DIR}/src/framework/utility/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/dialogs/*.cpp
    ${PROJECT_SOURCE_DIR}/src/gameui/utility/*.cpp
    ${PROJECT_SOURCE_DIR}/src/zshared/*.c
    ${PROJECT_SOURCE_DIR}/src/zshared/*.cpp

    ${PROJECT_SOURCE_DIR}/dependencies/reflector/include/reflection/default_error_handler.cpp

    ${PROJECT_SOURCE_DIR}/src/framework/*.hpp
    ${PROJECT_SOURCE_DIR}/include/framework/*.hpp
    ${PROJECT_SOURCE_DIR}/include/framework/utility/*.hpp
    ${PROJECT_SOURCE_DIR}/include/gameui/*.hpp
    ${PROJECT_SOURCE_DIR}/include/ztype/*.hpp
)

list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/bmpcodec.cpp)

if (UNIX AND NOT APPLE)
    file(GLOB add_sources src/framework/linux/*)
    list(APPEND sources ${add_sources})
endif()

if (WITH_BLEB)
    file(GLOB add_sources src/framework/bleb/*.cpp)
    list(APPEND sources ${add_sources})
endif()

if (WITH_ZTYPE)
    file(GLOB add_sources src/ztype/*.cpp)
    list(APPEND sources ${add_sources})
endif()

if (WITH_JPEG)
    list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/jfifcodec.cpp)
endif()

if (WITH_LODEPNG)
    list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/lodepngcodec.cpp)
endif()

if (WIN32)
    file(GLOB add_sources src/framework/win32/*)
    list(APPEND sources ${add_sources})
endif()

add_library(${library} ${sources})

if (ZOMBIE_API_VERSION)
	target_compile_definitions(${PROJECT_NAME} PUBLIC -DZOMBIE_API_VERSION=${ZOMBIE_API_VERSION})
endif()

# Bleb
if (WITH_BLEB)
    add_dependencies(${library} bleb)
    target_compile_definitions(${library} PUBLIC -DZOMBIE_WITH_BLEB=1)
    target_link_libraries(${library} bleb)
endif()

# freetype2
if (WITH_ZTYPE AND NOT EMSCRIPTEN)
    find_package(Freetype)

    if (Freetype_FOUND)
        target_include_directories(${library} PRIVATE ${FREETYPE_INCLUDE_DIRS})
        target_link_libraries(${library} ${FREETYPE_LIBRARIES})
    else()
        add_subdirectory(dependencies/freetype2)
        add_dependencies(${library} freetype)

        target_include_directories(${library} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/freetype2/include)
        target_link_libraries(${library} freetype)
    endif()
endif()

# jpeg
if (WITH_JPEG)
    find_package(JPEG)

    if (JPEG_FOUND)
        target_include_directories(${library} PRIVATE ${JPEG_INCLUDE_DIR})
        target_link_libraries(${library} ${JPEG_LIBRARIES})
    else()
        add_subdirectory(dependencies/libjpeg ${CMAKE_CURRENT_BINARY_DIR}/build-libjpeg)
        add_dependencies(${library} jpeg)

        target_include_directories(${library} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libjpeg)
        target_include_directories(${library} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/build-libjpeg)
        target_link_libraries(${library} jpeg)
    endif()

    target_compile_definitions(${library} PUBLIC -DZOMBIE_WITH_JPEG=1)
endif()

# libcfx2
add_dependencies(${library} cfx2)
target_link_libraries(${library} cfx2)

# littl
add_dependencies(${library} littl)
target_link_libraries(${library} littl)

# lodepng
if (WITH_LODEPNG)
    target_compile_definitions(${library} PRIVATE -DZOMBIE_WITH_LODEPNG=1)
endif()

if (UNIX)
    target_link_libraries(${library} dl)
    target_link_libraries(${library} pthread)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    dependencies/glm
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
