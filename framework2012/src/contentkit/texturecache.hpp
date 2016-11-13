
#include <framework/framework.hpp>
#include <framework/rendering.hpp>
#include <framework/startupscreen.hpp>

namespace contentkit
{
    using namespace zfw;

    class TexturePreviewCache
    {
        public:
            struct Entry
            {
                Float2 uv[2];
                Short2 size;

                uint64_t fileSize;
                Int2 originalSize;
                const char* format;
            };

        public:
            static TexturePreviewCache* Create(const char* fileName, CShort2& previewSize, CShort2& numTiles);
            virtual ~TexturePreviewCache() {}

            virtual void Init() = 0;

            virtual int AddToQueue(const char* fileName) = 0;
            virtual void AllSubmitted() = 0;
            virtual void Flush() = 0;
            virtual size_t GetQueueLength() = 0;
            virtual int ProcessEntries(uint64_t timelimitUsec) = 0;

            virtual zr::ITexture* RetrieveEntry(const char* fileName, Entry& info) = 0;
    };

    class UpdateTexturePreviewCacheStartupTask : public IStartupTask
    {
        public:
            static UpdateTexturePreviewCacheStartupTask* Create(TexturePreviewCache* previewCache);

            virtual void AddTextureDirectory(const char* dir) = 0;
    };
}