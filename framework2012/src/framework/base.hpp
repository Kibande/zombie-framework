#pragma once

#include <littl/BaseIO.hpp>
#include <littl/String.hpp>

#define f_pi 3.14159265358979323846f

#if defined(EMSCRIPTEN)
#define ZOMBIE_EMSCRIPTEN
#define ZOMBIE_PLATFORM "emscripten"
#elif defined(_WIN32) || defined (_WIN64)
// Win32, Win64
#define ZOMBIE_WINNT
#define ZOMBIE_PLATFORM "winnt"
#elif defined(__APPLE__)
// iOS, iOS Simulator (not supported); OS X
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define ZOMBIE_IOS
#define ZOMBIE_PLATFORM "iphone"
#elif TARGET_IPHONE_SIMULATOR
#define ZOMBIE_IOS
#define ZOMBIE_IOSSIM
#define ZOMBIE_PLATFORM "iphone"
#elif TARGET_OS_MAC
#define ZOMBIE_OSX
#define ZOMBIE_PLATFORM "osx"
#else
#error
#endif /* TARGET_OS_IPHONE */
#else
// General UNIX-like system (Linux, BSD)
#define ZOMBIE_UNIX
#define ZOMBIE_PLATFORM "linux"
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define ZOMBIE_DEBUG
#endif

#define DECL_MESSAGE(struct_,type_) struct struct_ : public zfw::MessageStruct<type_>

namespace zfw
{
    using namespace li;

    typedef uint8_t declname;
    typedef const void* Name;

    // -1   is "self" (not valid for over-the-air messages)
    // 0    is always the host (???)
    // 1+   are connected clients
    typedef int32_t ClientId;

    struct Vkey_t;

    class IEntity;
    class MessageQueue;
    class Session;

    template <int type> struct MessageStruct
    {
        static const int msgType = type;
    };
}