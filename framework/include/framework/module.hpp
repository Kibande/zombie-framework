#pragma once

#include <framework/base.hpp>

#if defined(ZOMBIE_WINNT)
#define ZOMBIE_LIBRARY_SUFFIX ".dll"
#define zombie_shared __declspec(dllexport)
#define zombie_cdecl __cdecl
#elif defined(ZOMBIE_OSX)
#define ZOMBIE_LIBRARY_SUFFIX ".dylib"
#define zombie_shared __attribute__ ((visibility ("default")))
#define zombie_cdecl
#else
#define ZOMBIE_LIBRARY_SUFFIX ".so"
#define zombie_shared __attribute__ ((visibility ("default")))
#define zombie_cdecl
#endif

namespace zfw
{
    enum { MODULE_NAME_MAX_LENGTH = 127 };

    typedef void* (zombie_cdecl *zfw_CreateInterface_t)(const char* interfaceName);
}
