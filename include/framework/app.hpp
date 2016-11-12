#pragma once

#include <framework/base.hpp>

#if defined(ZOMBIE_OSX)
#define ClientEntryPoint extern "C" int SDL_main(int argc, char **argv)
#else
#define ClientEntryPoint extern "C" int main(int argc, char **argv)
#endif
