
#include <framework/rendering.hpp>

namespace zr
{
    METHOD_NOT_IMPLEMENTED(void VertexBuffer::DropResources())
    
    VertexBuffer::VertexBuffer()
    {
        vbo = 0;
        vboCapacity = 0;
    }
    
    VertexBuffer::~VertexBuffer()
    {
        if (vbo != 0)
        {
            SetArrayBuffer(0);
            glDeleteBuffers(1, &vbo);
        }
    }
    
    void VertexBuffer::InitWithSize(int flags, size_t size)
    {
        ZFW_ASSERT(flags & VBUF_REMOTE)
        
        glGenBuffers(1, &vbo);
        ZFW_ASSERT(vbo != 0)
        
        SetArrayBuffer(vbo);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STATIC_DRAW);
        
        vboCapacity = size;
    }

    volatile uint8_t * VertexBuffer::MapForWriting()
    {
        zr::SetArrayBuffer(vbo);
        return (volatile uint8_t *) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    }

    void VertexBuffer::Unmap()
    {
        ZFW_ASSERT(glUnmapBuffer(GL_ARRAY_BUFFER) != GL_FALSE)
    }

    void VertexBuffer::UploadRange(size_t offset, size_t count, const void* data)
    {
        ZFW_ASSERT(offset + count <= this->vboCapacity)
        
        SetArrayBuffer(vbo);
        glBufferSubData(GL_ARRAY_BUFFER, offset, count, data);
    }
}
