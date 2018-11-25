#pragma once

#ifndef ZOMBIE_API_VERSION
#define ZOMBIE_API_VERSION 1000
#endif

#include <littl/Base.hpp>

#include <reflection/base.hpp>

#include <memory>
#include <typeindex>

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
#elif defined(_3DS)
#define ZOMBIE_CTR
#define ZOMBIE_PLATFORM "ctr"
#else
// General UNIX-like system (Linux, BSD)
#define ZOMBIE_UNIX
#define ZOMBIE_PLATFORM "linux"
#endif

#ifndef ZOMBIE_BUILDNAME
#define ZOMBIE_BUILDNAME "HEAD"
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define ZOMBIE_DEBUG
#endif

#ifdef ZOMBIE_DEBUG
#define ZOMBIE_BUILDTYPENAME "DEBUG"
#else
#define ZOMBIE_BUILDTYPENAME "RELEASE"
#endif

// MessageQueue helper
#define DECL_MESSAGE(struct_,type_) struct struct_ : public zfw::MessageStruct<type_>

static_assert(sizeof(int) == sizeof(int32_t), "this is not a joke.");

namespace li
{
    class InputStream;
    class OutputStream;
    class IOStream;
}

namespace zfw
{
    using li::InputStream;
    using li::OutputStream;
    using li::IOStream;

    using std::dynamic_pointer_cast;
    using std::move;
    using std::shared_ptr;
    using std::static_pointer_cast;
    using std::unique_ptr;

    using reflection::UUID_t;

    // -1   is "self" (not valid for over-the-air messages)
    // 0    is the host (if applicable)
    // 1+   are connected clients
    typedef int32_t ClientId;

    typedef std::type_index TypeID;

    // enums
    enum class PixmapFormat_t;

    // structs
    struct ErrorBuffer_t;
    struct FSStat_t;
    struct MessageHeader;
    struct Pixmap_t;
    struct PixmapInfo_t;
    struct Vkey_t;

    // core classes
    //class IAspectVisitor;
    class IBroadcastHandler;
    struct IComponentType;
    class IConfig;
    class IDecoder;
    class IEncoder;
    class IEngine;
    //class IEntity2;
    class IEntityHandler;
    class IEntityVisitor;
    class IEntityWorld2;
    class IEssentials;
    class IFileSystem;
    class IFSObject;
    class IFSUnion;
    class IGeneric;
    class IGraphics2;
    class IGraphics;
    class ILuaScriptContext;
    class IMediaCodecHandler;
    class IModuleHandler;
    class IPixmapDecoder;
    class IPixmapEncoder;
    class IResource;
    class IResource2;
    class IResourceManager;
    class IResourceManager2;
    class IResourceProvider;
    class IResourceProvider2;
    class IScriptAPI;
    class IScriptEnv;
    class IScriptHandler;
    class ISystem;
    class IVarSystem;
    class IVideoHandler;

    // helper classes
    class EntityWorld;
    class MessageQueue;
    class Profiler;
    class Session;
    class ShaderPreprocessor;
    class Timer;

    // core app interfaces
    class ICollisionHandler;
    class IEntity;
    class IScene;

    // built-in components
    struct Model3D;
    struct Position;

    template <int type>
    struct MessageStruct
    {
        static const int msgType = type;
    };

    template <class C> TypeID typeID()
    {
        return typeid(C);
    }

    template <typename ComponentStruct>
    IComponentType& GetComponentType();

    // h-values for mouse, keyboard events:
    //  h <= -2:    no widget was affected by this event (yet)
    static const int h_new = -2;
    //  h = -1:     a widget was indirectly affected by this event
    //  example: closing a dropdown menu by clicking outside of it
    static const int h_indirect = -1;
    //  h = 0:      a widget was directly affected by this event
    //  example: pressing a button; generally clicking/moving mouse within any widget
    static const int h_direct = 0;
    //  h = 1:      stop propagating this event immediately
    static const int h_stop = 1;

    static const float f_pi = 3.14159265358979323846f;
    static const float f_halfpi = 1.57079632679f;
}
