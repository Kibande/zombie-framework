
#include <StudioKit/texturecache.hpp>

#include <RenderingKit/RenderingKitUtility.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/filesystem.hpp>
#include <framework/utility/pixmap.hpp>

#include <littl/File.hpp>
#include <littl/Stack.hpp>
#include <littl/Thread.hpp>

#include <ctype.h>

// FIXME: Use FileSystem for texpreview.rgba

/*

    texpreview.cache File Format (unaligned)
    ----------------------------

    Header:                         (16 bytes)
        uint32_t version;           (set to 100)
        uint32_t num_entries;
        uint16_t previewSize[2];
        uint16_t numTiles[2];

    Entries:
        uint16_t fileNameLength     (incl. final NUL)
        char fileName[]             (UTF-8, NUL-terminated)
        uint64_t fileSize
        uint32_t size[2]
        uint8_t originalFormat      (enumerated; see below)

    originalFormat:
        0 = unk
        1 = JPEG
        2 = PNG

    texpreview.rgba File Format
    ---------------------------

    no header
    width = numTiles[0] * previewSize[0]
    height = numTiles[1] * previewSize[1]
    format = RGBA 8-bit uint
*/

namespace StudioKit
{
    using namespace li;
    using namespace RenderingKit;

    struct TexPreviewCacheHeader
    {
        uint32_t version;
        uint32_t num_entries;
        uint16_t previewSize[2];
        uint16_t numTiles[2];
    };

    struct TexPreviewCacheEntry
    {
        uint64_t offset;

        String fileName;
        uint32_t fileNameHash;

        uint64_t fileSize;
        uint32_t size[2];
        uint8_t originalFormat;
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class TexturePreviewCache : public ITexturePreviewCache
    {
        public:
            TexturePreviewCache();
            ~TexturePreviewCache();

            virtual bool Init(ISystem* sys, RenderingKit::IRenderingManager* rm,
                    const char* fileName, Short2 previewSize, Short2 numTiles) override;

            virtual int AddToQueue(const char* fileName) override;
            virtual void AllSubmitted() override { status = ALL_SUBMITTED; }
            virtual void Flush() override;
            virtual size_t GetQueueLength() override { return queue.getLength(); }
            virtual int ProcessEntries(uint64_t timelimitUsec) override;

            virtual bool RetrieveEntry(const char* fileName, TexturePreview_t& info) override;

        private:
            enum Status { SUBMITTING, ALL_SUBMITTED, TEXTURE_DOWNLOADED };

            struct QueueEntry
            {
                TexPreviewCacheEntry fileEntry;
                Short2 pos, size;

                unique_ptr<Pixmap_t> pm;

                QueueEntry(const QueueEntry& other) = delete;
                QueueEntry& operator =(const QueueEntry& other) = delete;

                QueueEntry() {}

                QueueEntry(QueueEntry&& other) : fileEntry(move(other.fileEntry)), pos(other.pos), size(other.size),
                        pm(move(other.pm)) {}

                QueueEntry& operator =(QueueEntry&& other)
                {
                    fileEntry = move(other.fileEntry);
                    pos = other.pos;
                    size = other.size;
                    pm = move(other.pm);
                    return *this;
                }
            };

            ISystem* sys;
            ErrorBuffer_t* eb;
            IRenderingManager* rm;

            Mutex queueMutex;

            unique_ptr<IOStream> io;
            TexPreviewCacheHeader header;
            List<TexPreviewCacheEntry> entries;

            List<QueueEntry> queue;
            Status status;
            int modifications;

            TexturedPainter2D<> tp;
            Pixmap_t renderTexPm;
            shared_ptr<ITexture> renderTex;
            shared_ptr<IRenderBuffer> renderBuffer;

            int AddEntryToQueue(size_t index);
            bool Load(const Short2& previewSize, const Short2& numTiles);
            static const char* GetFormatAsString(int originalFormat);
    };

    class UpdateTexturePreviewCacheStartupTask : public IUpdateTexturePreviewCacheStartupTask
    {
        public:
            UpdateTexturePreviewCacheStartupTask();

            virtual bool Init(ISystem* sys, ITexturePreviewCache* previewCache) override;

            virtual int Analyze() override;
            virtual void Init(IStartupTaskProgressListener* progressListener) override { listener = progressListener; }
            virtual void OnMainThreadDraw() override;
            virtual void Start() override;

            virtual void AddTextureDirectory(const char* dir) override { textureDirs.add( dir ); }

        private:
            ISystem* sys;

            ITexturePreviewCache* previewCache;

            List<String> textureDirs;

            IStartupTaskProgressListener* listener;
            size_t done, count;

            void ScanDir(const String& path);
    };

    // ====================================================================== //
    //  class TexturePreviewCache
    // ====================================================================== //

    ITexturePreviewCache* CreateTexturePreviewCache()
    {
        return new TexturePreviewCache();
    }

    TexturePreviewCache::TexturePreviewCache()
    {
        modifications = 0;
    }

    TexturePreviewCache::~TexturePreviewCache()
    {
        //Flush();
    }

    bool TexturePreviewCache::Init(ISystem* sys, RenderingKit::IRenderingManager* rm,
            const char* fileName, Short2 previewSize, Short2 numTiles)
    {
        SetEssentials(sys->GetEssentials());

        this->sys = sys;
        this->rm = rm;

        eb = GetErrorBuffer();
        auto fs = sys->GetFileSystem();

        // FIXME: Normalize file name
        IOStream* p_io = nullptr;

        if (!fs->OpenFileStream(fileName, kFileMayCreate, nullptr, nullptr, &p_io))
            return ErrorBuffer::SetError2(eb, eb->errorCode, 1,
                    "desc", sprintf_4095("Failed to open '%s' for writing", fileName)
                    ), false;

        io.reset(p_io);

        if (!Load(previewSize, numTiles))
        {
            header.version = 100;
            header.num_entries = 0;
            header.previewSize[0] = previewSize.x;
            header.previewSize[1] = previewSize.y;
            header.numTiles[0] = numTiles.x;
            header.numTiles[1] = numTiles.y;

            io->rewind();
            io->write(&header, sizeof(header));
        }

        // Create RenderTex
        Pixmap::Initialize(&renderTexPm, Int2(header.previewSize[0] * header.numTiles[0],
                header.previewSize[1] * header.numTiles[1]), PixmapFormat_t::RGBA8);

        if (!File::loadIntoBuffer("texpreview.rgba", Pixmap::GetPixelDataForWriting(&renderTexPm),
                Pixmap::GetBytesTotal(renderTexPm.info)))
        {
            entries.clear();
            modifications = 1;
        }
        else
            modifications = 0;

        status = SUBMITTING;

        if (!tp.Init(rm))
            return false;

        PixmapWrapper wrapper(&renderTexPm, false);

        renderTex = rm->CreateTexture("TexturePreviewCache::renderTex");
        renderTex->SetContentsFromPixmap(&wrapper);

        renderBuffer = rm->CreateRenderBufferWithTexture("TexturePreviewCache/renderBuffer", renderTex, 0);
        return true;
    }

    bool TexturePreviewCache::Load(const Short2& previewSize, const Short2& numTiles)
    {
        if (io->read(&header, sizeof(header)) != sizeof(header))
            return false;

        if (header.version != 100
                || header.previewSize[0] < previewSize.x || header.previewSize[1] < previewSize.y
                || header.numTiles[0] < numTiles.x || header.numTiles[1] < numTiles.y
                )
            return false;

        entries.resize(header.num_entries);
        printf("(TexturePreviewCache %u entries)\n", header.num_entries);

        for (unsigned int i = 0; i < header.num_entries; i++)
        {
            TexPreviewCacheEntry entry;
            entry.offset = io->getPos();

            uint16_t fileNameLength;

            if (!io->readLE(&fileNameLength))
                break;

            entry.fileName = io->readString();
            entry.fileNameHash = entry.fileName.getHash();

            // not sure how reliable this is
            if (io->read(&entry.fileSize, 17) != 17)
                break;

            printf("(TexturePreviewCache '%s')\n", entry.fileName.getBuffer());
            entries.add(move(entry));
        }

        return true;
    }

    int TexturePreviewCache::AddEntryToQueue(size_t index)
    {
        TexPreviewCacheEntry& entry = entries[index];

        unique_ptr<Pixmap_t> pm(new Pixmap_t);

        if (!Pixmap::LoadFromFile(sys, pm.get(), entry.fileName)) {
            sys->PrintError(eb, kLogWarning);
            return 0;
        }

        QueueEntry queueEntry;
        queueEntry.fileEntry = entry;
        queueEntry.pm = move(pm);
        queueEntry.pos = Short2((index % header.numTiles[0]) * header.previewSize[0], (index / header.numTiles[0]) * header.previewSize[1]);
        queueEntry.size = Short2(header.previewSize[0], header.previewSize[1]);

        entry.originalFormat = 0;
        entry.size[0] = queueEntry.pm->info.size.x;
        entry.size[1] = queueEntry.pm->info.size.y;

        String ext = FileName(entry.fileName).getExtension().getFiltered(tolower);

        if (ext == "jpg" || ext == "jpeg")
            entry.originalFormat = 1;
        else if (ext == "png")
            entry.originalFormat = 2;

        {
            CriticalSection cs(queueMutex);
            queue.add(move(queueEntry));
        }

        io->setPos(entry.offset);
        io->writeLE<uint16_t>(entry.fileName.getNumBytes() + 1);
        io->write(entry.fileName.getBuffer(), entry.fileName.getNumBytes() + 1);
        io->write(&entry.fileSize, 17);

        modifications++;
        return 1;
    }

    int TexturePreviewCache::AddToQueue(const char* fileName)
    {
        auto fs = sys->GetFileSystem();

        FSStat_t stat;
        fs->Stat(fileName, &stat);

        auto hash = String::getHash(fileName);

        iterate2 (i, entries)
        {
            auto& entry = *i;

            if (entry.fileNameHash != hash || entry.fileName != fileName)
                continue;

            if (entry.fileSize == stat.sizeInBytes)
                return 0;
            else
            {
                entry.fileSize = stat.sizeInBytes;
                return AddEntryToQueue(i.getIndex());
            }
        }

        // build
        size_t index = entries.getLength();     // TODO: what if index too big
        auto& entry = entries.addEmpty();
        
        entry.offset = io->getSize();
        entry.fileName = fileName;
        entry.fileNameHash = hash;
        entry.fileSize = stat.sizeInBytes;

        return AddEntryToQueue(index);
    }

    void TexturePreviewCache::Flush()
    {
        if (modifications > 0)
        {
            while (status != TEXTURE_DOWNLOADED)
                pauseThread(5);

            header.num_entries = entries.getLength();

            io->rewind();
            io->write(&header, sizeof(header));
            printf("(Updated texture preview cache with %u entries)\n", header.num_entries);

            File::saveBuffer("texpreview.rgba", Pixmap::GetPixelDataForWriting(&renderTexPm),
                    Pixmap::GetBytesTotal(renderTexPm.info));
            
            Pixmap::DropContents(&renderTexPm);

            modifications = 0;
        }
    }

    const char* TexturePreviewCache::GetFormatAsString(int originalFormat)
    {
        switch (originalFormat)
        {
            case 1: return "JPEG";
            case 2: return "PNG";
            default: return "unknown";
        }
    }

    int TexturePreviewCache::ProcessEntries(uint64_t timelimitUsec)
    {
        int i = 0;

        if (!queue.isEmpty())
        {
            QueueEntry queueEntry;

            uint64_t beginTime = sys->GetGlobalMicros();

            rm->PushRenderBuffer(renderBuffer.get());
            rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);

            for (; sys->GetGlobalMicros() < beginTime + timelimitUsec; i++)
            {
                {
                    CriticalSection cs(queueMutex);

                    if (queue.isEmpty())
                        break;

                    queueEntry = move(queue[0]);
                    queue.remove(0);
                }

                PixmapWrapper container(queueEntry.pm.get(), false);

                auto tex = rm->CreateTexture(queueEntry.fileEntry.fileName);
                tex->SetContentsFromPixmap(&container);

                tp.DrawFilledRectangle(tex.get(), queueEntry.pos, queueEntry.size, RGBA_WHITE);

                rm->FlushCachedMaterialOptions();
            }

            rm->PopRenderBuffer();
        }

        if (status == ALL_SUBMITTED && queue.isEmpty())
        {
            PixmapWrapper wrapper(&renderTexPm, false);
            renderTex->GetContentsIntoPixmap(&wrapper);
            status = TEXTURE_DOWNLOADED;
            renderBuffer.reset();
        }

        return i;
    }

    bool TexturePreviewCache::RetrieveEntry(const char* fileName, TexturePreview_t& info)
    {
        auto hash = String::getHash(fileName);

        iterate2 (i, entries)
        {
            auto& entry = *i;

            if (entry.fileNameHash == hash && entry.fileName == fileName)
            {
                const Float2 uv[] = {
                    Float2((float)(i.getIndex() % header.numTiles[0]) / header.numTiles[0],       (float)(i.getIndex() / header.numTiles[0]) / header.numTiles[1]),
                    Float2((float)(i.getIndex() % header.numTiles[0] + 1) / header.numTiles[0],   (float)(i.getIndex() / header.numTiles[0] + 1) / header.numTiles[1])
                };

                const Short2 size = Short2(header.previewSize[0], header.previewSize[1]);

                info.fileSize = entry.fileSize;
                info.originalSize = Int2(entry.size[0], entry.size[1]);
                info.formatName = GetFormatAsString(entry.originalFormat);

                info.g = rm->CreateGraphicsFromTexture2(renderTex, uv);
                return true;
            }
        }

        return false;
    }

    // ====================================================================== //
    //  class UpdateTexturePreviewCacheStartupTask
    // ====================================================================== //

    IUpdateTexturePreviewCacheStartupTask* CreateUpdateTexturePreviewCacheStartupTask()
    {
        return new UpdateTexturePreviewCacheStartupTask();
    }

    UpdateTexturePreviewCacheStartupTask::UpdateTexturePreviewCacheStartupTask()
    {
        done = 0;
        count = 0;
    }

    bool UpdateTexturePreviewCacheStartupTask::Init(ISystem* sys, ITexturePreviewCache* previewCache)
    {
        this->sys = sys;
        this->previewCache = previewCache;
        return true;
    }

    int UpdateTexturePreviewCacheStartupTask::Analyze()
    {
        iterate2 (i, textureDirs)
            ScanDir(i);

        previewCache->AllSubmitted();

        return previewCache->GetQueueLength();
    }

    void UpdateTexturePreviewCacheStartupTask::OnMainThreadDraw()
    {
        //if (previewCache->GetQueueLength() == 0)
        //    return;
        
        done += glm::max(0, previewCache->ProcessEntries(12000ULL));
    }

    void UpdateTexturePreviewCacheStartupTask::ScanDir(const String& path)
    {
        auto fs = sys->GetFileSystem();
        unique_ptr<IDirectory> dir(fs->OpenDirectory(path, 0));

        const char* next;

        while (dir != nullptr && (next = dir->ReadDir()) != nullptr)
        {
            String fileName = path + "/" + next;
            FSStat_t stat;

            if (!fs->Stat(fileName, &stat))
                continue;

            if (!stat.isDirectory)
                count += glm::max(0, previewCache->AddToQueue(fileName));
            else
                ScanDir(fileName);
        }
    }

    void UpdateTexturePreviewCacheStartupTask::Start()
    {
        listener->SetStatusMessage("updating texture preview cache...");

        while (previewCache->GetQueueLength() > 0)
        {
            listener->SetProgress(done);
            pauseThread(15);
        }

        previewCache->Flush();
    }
}
