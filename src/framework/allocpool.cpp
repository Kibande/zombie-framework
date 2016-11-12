
#include <framework/allocpool.hpp>

#define CLUSTER_SIZE_THRESHOLD      1024
#define CHUNK_SIZE_THRESHOLD        16
#define MAX_NEW_CHUNKS              16

namespace zfw
{
    using namespace li;

    struct Chunk
    {
        bool available;
        uint8_t* alloc_buffer;
        uint8_t* buffer;
        size_t size;
    };

    class AllocPoolImpl: public AllocPool
    {
        List<Chunk> pool;

        public:
            AllocPoolImpl();
            virtual ~AllocPoolImpl();

            virtual bool Alloc(size_t size, AllocChunk* chunk) override;
            virtual void Release(AllocChunk* chunk) override;
    };

    AllocPoolImpl::AllocPoolImpl()
    {
    }

    AllocPoolImpl::~AllocPoolImpl()
    {
        iterate2 (i, pool)
            free((*i).alloc_buffer);
    }

    bool AllocPoolImpl::Alloc(size_t size, AllocChunk* p_chunk)
    {
        iterate2 (i, pool)
        {
            auto& chunk = *i;

            if (chunk.available && size <= chunk.size && chunk.size - size < CHUNK_SIZE_THRESHOLD)
            {
                p_chunk->pool = this;
                p_chunk->key = i.getIndex();
                p_chunk->buffer = chunk.buffer;
                p_chunk->length = 0;

                chunk.available = false;
                return true;
            }
        }

        // round up to 16 bytes
        const size_t chunkAllocSize = (size + 15) & ~0xF;

        size_t numNewChunks = CLUSTER_SIZE_THRESHOLD / chunkAllocSize;

        if (numNewChunks < 1)
            numNewChunks = 1;
        else if (numNewChunks > MAX_NEW_CHUNKS)
            numNewChunks = MAX_NEW_CHUNKS;

        printf("allocpool: allocating %u chunks of %u bytes each (%u bytes total)\n",
                (unsigned)numNewChunks, (unsigned)chunkAllocSize, (unsigned)(numNewChunks * chunkAllocSize));

        uint8_t* alloc_buffer = (uint8_t *)malloc(numNewChunks * chunkAllocSize);

        if (alloc_buffer == nullptr)
            return false;

        size_t index = pool.getLength();
        pool.resize(pool.getLength() + numNewChunks, true);

        for (size_t i = 0; i < numNewChunks; i++)
        {
            Chunk newChunk;
            newChunk.available = true;
            newChunk.alloc_buffer = (i == 0) ? alloc_buffer : nullptr;
            newChunk.buffer = alloc_buffer;
            newChunk.size = chunkAllocSize;
            pool.add(newChunk);

            alloc_buffer += chunkAllocSize;
        }

        p_chunk->pool = this;
        p_chunk->key = index;
        p_chunk->buffer = pool[index].buffer;
        p_chunk->length = 0;

        pool[index].available = false;
        return true;
    }

    void AllocPoolImpl::Release(AllocChunk* chunk)
    {
        pool[chunk->key].available = true;
        chunk->buffer = nullptr;
    }

    AllocPool* AllocPool::Create()
    {
        return new AllocPoolImpl();
    }
}
