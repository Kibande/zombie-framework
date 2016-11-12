#pragma once

#include <framework/base.hpp>

namespace zfw
{
    IConfig*            p_CreateConfig(ErrorBuffer_t* eb);

    IEntityHandler*     p_CreateEntityHandler(ErrorBuffer_t* eb, ISystem* sys);
    shared_ptr<IFileSystem> p_CreateStdFileSystem(ErrorBuffer_t* eb, const char* absolutePathPrefix, int access);
    IFSUnion*           p_CreateFSUnion(ErrorBuffer_t* eb);
    IMediaCodecHandler* p_CreateMediaCodecHandler();
    IModuleHandler*     p_CreateModuleHandler(ErrorBuffer_t* eb);
    IResourceManager*   p_CreateResourceManager(ErrorBuffer_t* eb, ISystem* sys, const char* name);
    IResourceManager2*  p_CreateResourceManager2(ISystem* sys);
    ShaderPreprocessor* p_CreateShaderPreprocessor(ISystem* sys);
    Timer*              p_CreateTimer(ErrorBuffer_t* eb);
    IVarSystem*         p_CreateVarSystem(ISystem* sys);

    IPixmapEncoder*     p_CreateBmpEncoder(ISystem* sys);

#ifdef ZOMBIE_WITH_BLEB
    shared_ptr<IFileSystem> p_CreateBlebFileSystem(ISystem* sys, const char* path, int access);
#endif

#ifdef ZOMBIE_WITH_JPEG
    IPixmapDecoder*     p_CreateJfifDecoder();
#endif

#ifdef ZOMBIE_WITH_LODEPNG
    IPixmapDecoder*     p_CreateLodePngDecoder(ISystem* sys);
    IPixmapEncoder*     p_CreateLodePngEncoder(ISystem* sys);
#endif
}
