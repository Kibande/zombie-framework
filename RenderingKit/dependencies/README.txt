# freetype
At the very end of freetype2/CMakeLists.txt, add the following line:

target_include_directories(freetype PUBLIC "${PROJECT_SOURCE_DIR}/include")

# SDL
On Windows, extract SDL2-devel-2.0.5-VC.zip here and it will be auto-detected.
