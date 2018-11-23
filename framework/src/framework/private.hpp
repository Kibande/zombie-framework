#pragma once

#include <framework/base.hpp>

namespace zfw
{
    IConfig*            p_CreateConfig(ErrorBuffer_t* eb);

    unique_ptr<IBroadcastHandler> p_CreateBroadcastHandler();
    IEntityHandler*     p_CreateEntityHandler(ErrorBuffer_t* eb, IEngine* sys);
    unique_ptr<IEntityWorld2>       p_CreateEntityWorld2(IBroadcastHandler* broadcast);
    shared_ptr<IFileSystem> p_CreateStdFileSystem(ErrorBuffer_t* eb, const char* absolutePathPrefix, int access);
    IFSUnion*           p_CreateFSUnion(ErrorBuffer_t* eb);
    IMediaCodecHandler* p_CreateMediaCodecHandler();
    IModuleHandler*     p_CreateModuleHandler(ErrorBuffer_t* eb);
    IResourceManager*   p_CreateResourceManager(ErrorBuffer_t* eb, IEngine* sys, const char* name);
    IResourceManager2*  p_CreateResourceManager2(IEngine* sys);
    ShaderPreprocessor* p_CreateShaderPreprocessor(IEngine* sys);
    Timer*              p_CreateTimer(ErrorBuffer_t* eb);
    IVarSystem*         p_CreateVarSystem(IEngine* sys);

    IPixmapEncoder*     p_CreateBmpEncoder(IEngine* sys);

#ifdef ZOMBIE_WITH_BLEB
    shared_ptr<IFileSystem> p_CreateBlebFileSystem(IEngine* sys, const char* path, int access);
#endif

#ifdef ZOMBIE_WITH_JPEG
    IPixmapDecoder*     p_CreateJfifDecoder();
#endif

#ifdef ZOMBIE_WITH_LODEPNG
    IPixmapDecoder*     p_CreateLodePngDecoder(IEngine* sys);
    IPixmapEncoder*     p_CreateLodePngEncoder(IEngine* sys);
#endif
}
