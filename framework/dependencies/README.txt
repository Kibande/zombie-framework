# libjpeg
At the very end of libjpeg/CMakeLists.txt, add the following lines:

target_include_directories(jpeg PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(jpeg PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
