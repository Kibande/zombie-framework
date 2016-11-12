# freetype
At the very end of freetype2/CMakeLists.txt, add the following line:

target_include_directories(freetype PUBLIC "${PROJECT_SOURCE_DIR}/include")

# libjpeg
At the very end of libjpeg/CMakeLists.txt, add the following lines:

target_include_directories(jpeg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(jpeg PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
