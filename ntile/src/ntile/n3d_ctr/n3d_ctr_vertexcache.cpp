
#include "n3d_ctr_internal.hpp"

namespace n3d
{
    CTRVertexCache::CTRVertexCache(size_t initSize) : CTRVertexBuffer()
    {
        owner = nullptr;
        offset = 0;
        bytesUsed = 0;
        mapped = nullptr;

        CTRVertexBuffer::Alloc(initSize);
    }
    
    void* CTRVertexCache::Alloc(IVertexCacheOwner* owner, size_t count)
    {
        if (owner != this->owner)
        {
            Flush();
            
            this->owner = owner;
        }
        
        zombie_assert(bytesUsed + count <= this->vbo.size);
        
        if (mapped == nullptr)
        {
            mapped = this->Map(false, true);
            zombie_assert(mapped != nullptr);
        }
        
        void* ptr = reinterpret_cast<uint8_t*>(mapped) + offset + bytesUsed;
        bytesUsed += count;
        return ptr;
    }
    
    void CTRVertexCache::Flush()
    {
        if (this->owner != nullptr)
        {
            this->Unmap();
            mapped = nullptr;
            
            if (bytesUsed)
            {
                this->owner->OnVertexCacheFlush(this, offset, bytesUsed);
                offset += bytesUsed;
            }
            bytesUsed = 0;
            
            this->owner = nullptr;
        }
    }

    void CTRVertexCache::Reset()
    {
        offset = 0;
    }
}
