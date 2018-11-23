cmake_minimum_required(VERSION 3.1)
project(zombie_framework)

set(WITH_BLEB ON CACHE BOOL "Enable Bleb support")
set(WITH_JPEG ON CACHE BOOL "Enable JPEG support (requires libjpeg)")
set(WITH_LODEPNG ON CACHE BOOL "Enable PNG support via LodePNG")
set(WITH_ZTYPE ON CACHE BOOL "Enable ztype (depends on freetype2)")
set(ZOMBIE_WITH_LUA OFF CACHE BOOL "Enable experimental Lua support")

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 14)
set(library ${PROJECT_NAME})

add_subdirectory(dependencies/bleb)
add_subdirectory(dependencies/libcfx2)
add_subdirectory(dependencies/littl)

set(sources
        dependencies/reflector/include/reflection/default_error_handler.cpp

        include/framework/abstractentity.hpp
        include/framework/app.hpp
        include/framework/components/drawable.hpp
        include/framework/components/position.hpp
        include/framework/componenttype.hpp
        include/framework/base.hpp
        include/framework/bleb/studiobleb.hpp
        include/framework/broadcasthandler.hpp
        include/framework/colorconstants.hpp
        include/framework/datamodel.hpp
        include/framework/engine.hpp
        include/framework/entity.hpp
        include/framework/entity2.hpp
        include/framework/entityhandler.hpp
        include/framework/entityworld.hpp
        include/framework/entityworld2.hpp
        include/framework/errorbuffer.hpp
        include/framework/errorcheck.hpp
        include/framework/errorcodes.hpp
        include/framework/event.hpp
        include/framework/filesystem.hpp
        include/framework/framework.hpp
        include/framework/graphics.hpp
        include/framework/interpolator.hpp
        include/framework/lua/luascript.hpp
        include/framework/mediacodechandler.hpp
        include/framework/messagequeue.hpp
        include/framework/module.hpp
        include/framework/modulehandler.hpp
        include/framework/nativedialogs.hpp
        include/framework/pixmap.hpp
        include/framework/pointentity.hpp
        include/framework/profiler.hpp
        include/framework/resource.hpp
        include/framework/resourcemanager.hpp
        include/framework/resourcemanager2.hpp
        include/framework/scene.hpp
        include/framework/scripting.hpp
        include/framework/shader_preprocessor.hpp
        include/framework/studio/recentfiles.hpp
        include/framework/system.hpp
        include/framework/timer.hpp
        include/framework/utility/algebrahelpers.hpp
        include/framework/utility/errorbuffer.hpp
        include/framework/utility/essentials.hpp
        include/framework/utility/params.hpp
        include/framework/utility/pixmap.hpp
        include/framework/utility/util.hpp
        include/framework/varsystem.hpp
        include/framework/videocapture.hpp
        include/framework/videohandler.hpp

        include/gameui/dialogs.hpp
        include/gameui/gameui.hpp
        include/gameui/gameui_base.hpp
        include/gameui/uithemer.hpp

        include/zshared/crc16_ccitt.hpp
        include/zshared/mediafile2.hpp

        include/ztype/ztype.hpp

        src/framework/components/drawable.cpp
        src/framework/components/position.cpp
        src/framework/asprintf.cpp
        src/framework/broadcasthandler.cpp
        src/framework/codecs/bmpcodec.cpp
        src/framework/codecs/jfifcodec.cpp
        src/framework/codecs/lodepngcodec.cpp
        src/framework/datamodel.cpp
        src/framework/engine.cpp
        src/framework/entityhandler.cpp
        src/framework/entityworld.cpp
        src/framework/entityworld2.cpp
        src/framework/filesystem_std.cpp
        src/framework/fsunion.cpp
        src/framework/mediacodechandler.cpp
        src/framework/messagequeue.cpp
        src/framework/modulehandler.cpp
        src/framework/private.hpp
        src/framework/profiler.cpp
        src/framework/resourcemanager.cpp
        src/framework/resourcemanager2.cpp
        src/framework/scene.cpp
        src/framework/shader_preprocessor.cpp
        src/framework/studio/recentfiles.cpp
        src/framework/timer.cpp
        src/framework/utility/algebrahelpers.cpp
        src/framework/utility/essentials.cpp
        src/framework/utility/params.cpp
        src/framework/utility/util.cpp
        src/framework/utility/vkey.cpp
        src/framework/varsystem.cpp
        src/framework/vkeyreflection.cpp

        src/gameui/button.cpp
        src/gameui/checkbox.cpp
        src/gameui/combobox.cpp
        src/gameui/dialogs/messagedlg.cpp
        src/gameui/graphics.cpp
        src/gameui/listctrl.cpp
        src/gameui/menubar.cpp
        src/gameui/panel.cpp
        src/gameui/popup.cpp
        src/gameui/scrollbar.cpp
        src/gameui/slider.cpp
        src/gameui/staticimage.cpp
        src/gameui/statictext.cpp
        src/gameui/tablelayout.cpp
        src/gameui/textbox.cpp
        src/gameui/treebox.cpp
        src/gameui/uicontainer.cpp
        src/gameui/uiloader.cpp
        src/gameui/utility/uithemerhelper.cpp
        src/gameui/widget.cpp
        src/gameui/widgetcontainer.cpp
        src/gameui/window.cpp

        src/zshared/crc16_ccitt.cpp
        src/zshared/md5.c
        src/zshared/md5.h
        src/zshared/mediafile2.cpp
        )

if (UNIX AND NOT APPLE)
    list(APPEND sources
            src/framework/linux/nativedialogs.cpp
            )
endif()

if (WITH_BLEB)
    list(APPEND sources
            src/framework/bleb/filesystem_bleb.cpp
            src/framework/bleb/studiobleb.cpp
            )
endif()

if (ZOMBIE_WITH_LUA)
    list(APPEND sources
            src/framework/lua/luascript.cpp
            src/framework/lua/private.hpp
            )
endif()

if (WITH_ZTYPE)
    list(APPEND sources
            src/ztype/facefile.cpp
            src/ztype/facelistfile.cpp
            src/ztype/fontcache.cpp
            src/ztype/rasterizer.cpp
            )
endif()

if (WITH_JPEG)
    list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/jfifcodec.cpp)
endif()

if (WITH_LODEPNG)
    list(APPEND sources ${PROJECT_SOURCE_DIR}/src/framework/codecs/lodepngcodec.cpp)
endif()

if (WIN32)
    list(APPEND sources
            src/framework/win32/nativedialogs.cpp
            )
endif()

add_library(${library} ${sources})

target_compile_definitions(${library} PUBLIC $<$<CONFIG:DEBUG>:DEBUG>)
target_compile_features(${library} PUBLIC cxx_std_14)

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

# Lua
if (ZOMBIE_WITH_LUA)
    find_package(Lua)

    # Lua's include directories need to be public, because we expose lua_State* for template-based binding
    if (LUA_FOUND)
        target_include_directories(${PROJECT_NAME} PUBLIC SYSTEM ${LUA_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
    else()
        add_subdirectory(dependencies/lua ${CMAKE_CURRENT_BINARY_DIR}/build-lua)
        target_include_directories(${PROJECT_NAME} PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lua/src
                ${CMAKE_CURRENT_BINARY_DIR}/build-lua
                )
        target_link_libraries(${PROJECT_NAME} liblua)
    endif()

    target_compile_definitions(${library} PUBLIC -DZOMBIE_WITH_LUA=1)
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
