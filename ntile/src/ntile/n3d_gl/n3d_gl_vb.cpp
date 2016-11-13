
#include "n3d_gl_internal.hpp"

namespace n3d
{
    GLVertexBuffer::GLVertexBuffer()
    {
        numRefs = 1;
        glGenBuffers(1, &vbo);
        size = 0;
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        glDeleteBuffers(1, &vbo);
    }

    bool GLVertexBuffer::Alloc(size_t size)
    {
        gl.BindVBO(vbo);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_STREAM_DRAW);

        this->size = size;
        return true;
    }

    void* GLVertexBuffer::Map(bool read, bool write)
    {
        gl.BindVBO(vbo);

        int access;

        if (read && write)
            access = GL_READ_WRITE;
        else if (read)
            access = GL_READ_ONLY;
        else
            access = GL_WRITE_ONLY;

        return glMapBuffer(GL_ARRAY_BUFFER, access);
    }

    void GLVertexBuffer::Unmap()
    {
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    bool GLVertexBuffer::Upload(const uint8_t* data, size_t length)
    {
        gl.BindVBO(vbo);
        glBufferData(GL_ARRAY_BUFFER, length, data, GL_STREAM_DRAW);

        this->size = length;
        return true;
    }

    bool GLVertexBuffer::UploadRange(size_t offset, size_t length, const uint8_t* data)
    {
        // FIXME: Range checking?

        gl.BindVBO(vbo);
        glBufferSubData(GL_ARRAY_BUFFER, offset, length, data);
        return true;
    }
}
