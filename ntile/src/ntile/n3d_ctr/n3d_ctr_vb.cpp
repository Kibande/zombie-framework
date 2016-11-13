
#include "n3d_ctr_internal.hpp"

namespace n3d
{
    CTRVertexBuffer::CTRVertexBuffer()
    {
        glInitBufferCTR(&vbo);
    }

    CTRVertexBuffer::~CTRVertexBuffer()
    {
        glShutdownBufferCTR(&vbo);
    }

    bool CTRVertexBuffer::Alloc(size_t size)
    {
        glNamedBufferData((GLuint) &vbo, size, nullptr, GL_STREAM_DRAW);
        return true;
    }

    void* CTRVertexBuffer::Map(bool read, bool write)
    {
        int access;

        if (read && write)
            access = GL_READ_WRITE;
        else if (read)
            access = GL_READ_ONLY;
        else
            access = GL_WRITE_ONLY;

        return glMapNamedBuffer((GLuint) &vbo, access);
    }

    void CTRVertexBuffer::Unmap()
    {
        glUnmapNamedBuffer((GLuint) &vbo);
    }

    bool CTRVertexBuffer::Upload(const uint8_t* data, size_t length)
    {
        glNamedBufferData((GLuint) &vbo, length, data, GL_STREAM_DRAW);

        return true;
    }

    bool CTRVertexBuffer::UploadRange(size_t offset, size_t length, const uint8_t* data)
    {
        //zombie_assert(offset + length <= vbo.currentSize);

        glNamedBufferSubData((GLuint) &vbo, offset, length, data);
        return true;
    }
}
