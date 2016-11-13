
#include "texturecache.hpp"

#include <littl/File.hpp>
#include <littl/Stack.hpp>
#include <littl/Thread.hpp>

#include <ctype.h>

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

namespace contentkit
{
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

    class TexturePreviewCacheImpl : public TexturePreviewCache
    {
        public:
            enum Status { SUBMITTING, ALL_SUBMITTED, TEXTURE_DOWNLOADED };

            struct QueueEntry
            {
                TexPreviewCacheEntry fileEntry;
                Short2 pos, size;

                media::DIBitmap* bmp;
            };

            Mutex queueMutex;

            Reference<SeekableIOStream> io;
            TexPreviewCacheHeader header;
            List<TexPreviewCacheEntry> entries;

            List<QueueEntry> queue;
            Status status;
            int modifications;

            media::DIBitmap renderTexBmp;
            Reference<zr::ITexture> renderTex;
            Object<zr::RenderBuffer> renderBuffer;

            int AddEntryToQueue(size_t index);
            bool Load(CShort2& previewSize, CShort2& numTiles);
            static const char* GetFormatAsString(int originalFormat);

        public:
            TexturePreviewCacheImpl(SeekableIOStream* io, CShort2& previewSize, CShort2& numTiles);
            virtual ~TexturePreviewCacheImpl();

            virtual void Init() override;

            virtual int AddToQueue(const char* fileName) override;
            virtual void AllSubmitted() override { status = ALL_SUBMITTED; }
            virtual void Flush() override;
            virtual size_t GetQueueLength() override { return queue.getLength(); }
            virtual int ProcessEntries(uint64_t timelimitUsec) override;

            virtual zr::ITexture* RetrieveEntry(const char* fileName, Entry& info) override;
    };

    class UpdateTexturePreviewCacheStartupTaskImpl : public IListDirCallback,
            public UpdateTexturePreviewCacheStartupTask
    {
        TexturePreviewCache* previewCache;

        List<String> textureDirs;
        Stack<String> currentDir;

        IStartupTaskProgressListener* listener;
        size_t done, count;

        virtual bool OnDirEntry(const char* name, const FileMeta_t* meta) override;
        void ScanDir(const String& path);

        public:
            UpdateTexturePreviewCacheStartupTaskImpl(TexturePreviewCache* previewCache);

            virtual int Analyze() override;
            virtual void Init(IStartupTaskProgressListener* progressListener) override { listener = progressListener; }
            virtual void OnMainThreadDraw() override;
            virtual void Start() override;

            virtual void AddTextureDirectory(const char* dir) override { textureDirs.add( dir ); }
    };

    TexturePreviewCacheImpl::TexturePreviewCacheImpl(SeekableIOStream* io, CShort2& previewSize, CShort2& numTiles)
        : io(io)
    {
        if (!Load(previewSize, numTiles))
        {
            header.version = 100;
            header.num_entries = 0;
            header.previewSize[0] = previewSize.x;
            header.previewSize[1] = previewSize.y;
            header.numTiles[0] = numTiles.x;
            header.numTiles[1] = numTiles.y;

            io->setPos(0);
            io->writeItems(&header, 1);
        }

        // Create RenderTex
        media::Bitmap::Alloc(&renderTexBmp, header.previewSize[0] * header.numTiles[0], header.previewSize[1] * header.numTiles[1], media::TEX_RGBA8);

        if (!File::load("texpreview.rgba", renderTexBmp.data.getPtrUnsafe(), renderTexBmp.size.y * renderTexBmp.stride))
            entries.clear();

        modifications = 0;

        status = SUBMITTING;
    }

    TexturePreviewCacheImpl::~TexturePreviewCacheImpl()
    {
        Flush();
    }

    void TexturePreviewCacheImpl::Init()
    {
        renderTex = render::Loader::CreateTexture("texpreview", &renderTexBmp);
        renderBuffer = new zr::RenderBuffer(renderTex->reference());
    }

    int TexturePreviewCacheImpl::AddEntryToQueue(size_t index)
    {
        TexPreviewCacheEntry& entry = entries[index];

        Object<media::DIBitmap> bmp = new media::DIBitmap;

        if (!media::MediaLoader::LoadBitmap(entry.fileName, bmp, false))
            return 0;

        QueueEntry queueEntry;
        queueEntry.fileEntry = entry;
        queueEntry.bmp = bmp.detach();
        queueEntry.pos = Short2((index % header.numTiles[0]) * header.previewSize[0], (index / header.numTiles[0]) * header.previewSize[1]);
        queueEntry.size = Short2(header.previewSize[0], header.previewSize[1]);

        entry.originalFormat = 0;
        entry.size[0] = queueEntry.bmp->size.x;
        entry.size[1] = queueEntry.bmp->size.y;

        String ext = FileName(entry.fileName).getExtension().getFiltered(tolower);

        if (ext == "jpg" || ext == "jpeg")
            entry.originalFormat = 1;
        else if (ext == "png")
            entry.originalFormat = 2;

        {
            li_synchronized(queueMutex)
            queue.add((QueueEntry&&) queueEntry);
        }

        io->setPos(entry.offset);
        io->write<uint16_t>(entry.fileName.getNumBytes() + 1);
        io->write(entry.fileName.getBuffer(), entry.fileName.getNumBytes() + 1);
        io->write(&entry.fileSize, 17);

        modifications++;
        return 1;
    }

    int TexturePreviewCacheImpl::AddToQueue(const char* fileName)
    {
        uint64_t fileSize = 0;
        File::queryFileSize(fileName, fileSize);

        auto hash = String::getHash(fileName);

        iterate2 (i, entries)
        {
            auto& entry = *i;

            if (entry.fileNameHash != hash || entry.fileName != fileName)
                continue;

            if (entry.fileSize == fileSize)
                return 0;
            else
            {
                entry.fileSize = fileSize;
                return AddEntryToQueue(i.getIndex());
            }
        }

        // build
        size_t index = entries.getLength();     // TODO: what if index too big
        auto& entry = entries.addEmpty();
        
        entry.offset = io->getSize();
        entry.fileName = fileName;
        entry.fileNameHash = hash;
        entry.fileSize = fileSize;

        return AddEntryToQueue(index);
    }

    void TexturePreviewCacheImpl::Flush()
    {
        if (modifications > 0)
        {
            while (status != TEXTURE_DOWNLOADED)
                pauseThread(5);

            header.num_entries = entries.getLength();

            io->setPos(0);
            io->writeItems(&header, 1);
            printf("(Updated texture preview cache with %u entries)\n", header.num_entries);

            File::save("texpreview.rgba", renderTexBmp.data.getPtrUnsafe(), renderTexBmp.size.y * renderTexBmp.stride);
            renderTexBmp.data.resize(0, false);

            modifications = 0;
        }
    }

    const char* TexturePreviewCacheImpl::GetFormatAsString(int originalFormat)
    {
        switch (originalFormat)
        {
            case 1: return "JPEG";
            case 2: return "PNG";
            default: return "unknown";
        }
    }

    bool TexturePreviewCacheImpl::Load(CShort2& previewSize, CShort2& numTiles)
    {
        if (!io->readItems(&header, 1))
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

            if (!io->readItems(&fileNameLength, 1))
                break;

            entry.fileName = io->readString();
            entry.fileNameHash = entry.fileName.getHash();

            // not sure how reliable this is
            if (io->read(&entry.fileSize, 17) != 17)
                break;

            printf("(TexturePreviewCache '%s')\n", entry.fileName.getBuffer());
            entries.add((TexPreviewCacheEntry &&) entry);
        }

        return true;
    }

    int TexturePreviewCacheImpl::ProcessEntries(uint64_t timelimitUsec)
    {
        QueueEntry queueEntry;

        uint64_t beginTime = Timer::getRelativeMicroseconds();

        zr::R::PushRenderBuffer(renderBuffer);
        zr::R::Set2DView();

        int i;

        for (i = 0; Timer::getRelativeMicroseconds() < beginTime + timelimitUsec; i++)
        {
            {
                li_synchronized(queueMutex)

                if (queue.isEmpty())
                    break;

                queueEntry = (QueueEntry&&) queue[0];
                queue.remove(0);
            }

            Reference<zr::ITexture> tex = render::Loader::CreateTexture(queueEntry.fileEntry.fileName, queueEntry.bmp);
            li::destroy(queueEntry.bmp);

            zr::R::SetTexture(tex);
            zr::R::DrawFilledRect(queueEntry.pos, queueEntry.pos + queueEntry.size, Float2(0.0f, 0.0f), Float2(1.0f, 1.0f), RGBA_WHITE);
        }

        zr::R::PopRenderBuffer();

        if (status == ALL_SUBMITTED && queue.isEmpty())
        {
            renderTex->Download(&renderTexBmp);
            status = TEXTURE_DOWNLOADED;
            renderBuffer.release();
        }

        return i;
    }

    zr::ITexture* TexturePreviewCacheImpl::RetrieveEntry(const char* fileName, Entry& info)
    {
        auto hash = String::getHash(fileName);

        iterate2 (i, entries)
        {
            auto& entry = *i;

            if (entry.fileNameHash == hash && entry.fileName == fileName)
            {
                // Oh you, broken ITexture.Download()
                info.uv[0] = Float2((float)(i.getIndex() % header.numTiles[0]) / header.numTiles[0], 1.0f - (float)(i.getIndex() / header.numTiles[0]) / header.numTiles[1]);
                info.uv[1] = Float2((float)(i.getIndex() % header.numTiles[0] + 1) / header.numTiles[0], 1.0f - (float)(i.getIndex() / header.numTiles[0] + 1) / header.numTiles[1]);
                info.size = Short2(header.previewSize[0], header.previewSize[1]);

                info.fileSize = entry.fileSize;
                info.originalSize = Int2(entry.size[0], entry.size[1]);
                info.format = GetFormatAsString(entry.originalFormat);

                return renderTex->reference();
            }
        }

        return nullptr;
    }

    UpdateTexturePreviewCacheStartupTaskImpl::UpdateTexturePreviewCacheStartupTaskImpl(TexturePreviewCache* previewCache)
            : previewCache(previewCache)
    {
        done = 0;
        count = 0;
    }

    int UpdateTexturePreviewCacheStartupTaskImpl::Analyze()
    {
        iterate2 (i, textureDirs)
            ScanDir(i);

        previewCache->AllSubmitted();

        return previewCache->GetQueueLength();
    }

    bool UpdateTexturePreviewCacheStartupTaskImpl::OnDirEntry(const char* name, const FileMeta_t* meta)
    {
        if (name[0] == '.' || name[0] == 0)
            return true;

        if (meta->type == FileMeta_t::F_FILE)
            count += glm::max(0, previewCache->AddToQueue(currentDir.top() + "/" + name));
        else if (meta->type == FileMeta_t::F_DIR)
            ScanDir(currentDir.top() + "/" + name);

        return true;
    }

    void UpdateTexturePreviewCacheStartupTaskImpl::OnMainThreadDraw()
    {
        if (previewCache->GetQueueLength() == 0)
            return;
        
        done += glm::max(0, previewCache->ProcessEntries(12000ULL));
    }

    void UpdateTexturePreviewCacheStartupTaskImpl::ScanDir(const String& path)
    {
        currentDir.push(path);
        Sys::ListDir(path, this);
        currentDir.pop();
    }

    void UpdateTexturePreviewCacheStartupTaskImpl::Start()
    {
        listener->SetStatusMessage("updating texture preview cache...");

        while (previewCache->GetQueueLength() > 0)
        {
            listener->SetProgress(done);
            pauseThread(15);
        }

        previewCache->Flush();
    }

    TexturePreviewCache* TexturePreviewCache::Create(const char* fileName, CShort2& previewSize, CShort2& numTiles)
    {
        SeekableIOStream* io = Sys::OpenRandomIO(fileName);

        if (io == nullptr)
            return nullptr;

        return new TexturePreviewCacheImpl(io, previewSize, numTiles);
    }

    UpdateTexturePreviewCacheStartupTask* UpdateTexturePreviewCacheStartupTask::Create(TexturePreviewCache* previewCache)
    {
        return new UpdateTexturePreviewCacheStartupTaskImpl(previewCache);
    }
}