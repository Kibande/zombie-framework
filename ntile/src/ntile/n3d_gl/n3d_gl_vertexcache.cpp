
#include "n3d_gl_internal.hpp"

namespace n3d
{
    GLVertexCache::GLVertexCache(size_t initSize) : GLVertexBuffer()
    {
        owner = nullptr;
        bytesUsed = 0;
        mapped = nullptr;

        GLVertexBuffer::Alloc(initSize);
    }
    
    void* GLVertexCache::Alloc(IVertexCacheOwner* owner, size_t count)
    {
        if (owner != this->owner)
        {
            Flush();
            
            this->owner = owner;
        }
        
        if (bytesUsed + count > this->size)
        {
            Flush();
            
            // TODO: Grow VBO
        }
        
        ZFW_ASSERT(bytesUsed + count <= this->size)
        
        if (mapped == nullptr)
        {
            mapped = this->Map(false, true);
            ZFW_ASSERT(mapped != nullptr)
        }
        
        void* ptr = reinterpret_cast<uint8_t*>(mapped) + bytesUsed;
        bytesUsed += count;
        return ptr;
    }
    
    void GLVertexCache::Flush()
    {
        if (this->owner != nullptr)
        {
            this->Unmap();
            mapped = nullptr;
            
            this->owner->OnVertexCacheFlush(this, bytesUsed);
            bytesUsed = 0;
            
            this->owner = nullptr;
        }
    }
}
