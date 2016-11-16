#pragma once

#include <framework/modulehandler.hpp>

#include <RenderingKit/RenderingKit.hpp>

#include <reflection/magic.hpp>

#include "startupscreen.hpp"

namespace StudioKit
{
    using namespace zfw;

    struct TexturePreview_t
    {
        shared_ptr<IGraphics> g;

        uint64_t fileSize;
        Int2 originalSize;
        const char* formatName;
    };

    class ITexturePreviewCache
    {
        public:
            virtual ~ITexturePreviewCache() {}

            virtual bool Init(ISystem* sys, RenderingKit::IRenderingManager* rm,
                    const char* fileName, Short2 previewSize, Short2 numTiles) = 0;

            virtual int AddToQueue(const char* fileName) = 0;
            virtual void AllSubmitted() = 0;
            virtual void Flush() = 0;
            virtual size_t GetQueueLength() = 0;
            virtual int ProcessEntries(uint64_t timelimitUsec) = 0;

            virtual bool RetrieveEntry(const char* fileName, TexturePreview_t& entry_out) = 0;

            REFL_CLASS_NAME("ITexturePreviewCache", 1)
    };

    class IUpdateTexturePreviewCacheStartupTask : public IStartupTask
    {
        public:
            virtual bool Init(ISystem* sys, ITexturePreviewCache* previewCache) = 0;

            virtual void AddTextureDirectory(const char* dir) = 0;

            REFL_CLASS_NAME("IUpdateTexturePreviewCacheStartupTask", 1)
    };

#ifdef STUDIO_KIT_STATIC
    ITexturePreviewCache* CreateTexturePreviewCache();
    IUpdateTexturePreviewCacheStartupTask* CreateUpdateTexturePreviewCacheStartupTask();

    ZOMBIE_IFACTORYLOCAL(TexturePreviewCache)
    ZOMBIE_IFACTORYLOCAL(UpdateTexturePreviewCacheStartupTask)
#else
    ZOMBIE_IFACTORY(TexturePreviewCache, "StudioKit")
    ZOMBIE_IFACTORY(UpdateTexturePreviewCacheStartupTask, "StudioKit")
#endif
}
