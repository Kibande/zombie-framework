#pragma once

#if defined(ZOMBIE_OSX)
#define ClientEntryPoint extern "C" int SDL_main(int argc, char **argv)
#else
#define ClientEntryPoint extern "C" int main(int argc, char **argv)
#endif

#if defined(ZOMBIE_WINNT)
#define ZFW_LIBRARY_EXT ".zfw"
#elif defined(ZOMBIE_OSX)
#define ZFW_LIBRARY_EXT ".dylib"
#else
#define ZFW_LIBRARY_EXT ".so"
#endif

namespace zfw
{
    enum {
        APP_STANDALONE = 1,
        APP_HOSTED = 2
    };

    struct AppLibraryInfo
    {
        const char *name, *title, *desc, *homepage, *iconfile;
        int buildVersion;
    };

    class HostedApp {
        public:
            virtual void Init() = 0;
            virtual void Exit() = 0;
    };

    // DLL/so/dylib entry points
    typedef int (*AppLibrary_Init)(int argc, char **argv);
    typedef void* (AppLibrary_QueryInterface)(const char *interfaceName);
    typedef int (*AppLibrary_main)(int argc, char **argv);
}