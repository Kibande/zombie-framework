
#include "rendering.hpp"

#include <signal.h>

#define VBO_DEFAULT_PAGESIZE            4096

namespace zr {
    using zfw::render::enabled;

    BatchState batch;

    size_t AddVertices_XYs_UV_RGBAb(volatile uint8_t *buffer, size_t count, CShort2* pos, CFloat2* uv, CByte4* colour)
    {
        size_t i;

        if (pos != nullptr)
        {
            int32_t *buffer_int = (int32_t *) buffer;

            for (i = 0; i < count; i++)
            {
                *buffer_int = *(int32_t *) pos;
                buffer_int += 4;
                ++pos;
            }
        }

        if (uv != nullptr)
        {
            float *buffer_float = (float *) (buffer + 4);

            for (i = 0; i < count; i++)
            {
                *buffer_float++ = uv->x;
                *buffer_float = uv->y;
                buffer_float += 3;
                ++uv;
            }
        }

        if (colour != nullptr)
        {
            uint32_t *buffer_uint = (uint32_t *) (buffer + 12);

            for (i = 0; i < count; i++)
            {
                *buffer_uint = *(uint32_t *) colour;
                buffer_uint += 4;
                ++colour;
            }
        }
        else
        {
            uint32_t *buffer_uint = (uint32_t *) (buffer + 12);

            for (i = 0; i < count; i++)
            {
                *buffer_uint = 0xFFFFFFFF;
                buffer_uint += 4;
            }
        }

        return count * 16;
    }

    const VertexFormat Batch::Format_XYs_UV_RGBAb = {
        16,
        {FMT_INT16, 2, 0},
        {0, 0, 0},
        {FMT_FLOAT32, 2, 4},
        {FMT_UINT8, 4, 12},
        AddVertices_XYs_UV_RGBAb
    };

    void Batch::Init()
    {
        batch.flags = BATCH_REMOTE | BATCH_THROW_ON_OVERFLOW;

        batch.vbo = 0;
        batch.bufferCapacity = 0;
        batch.bufferUsed = 0;
        //batch.vboPageSize = VBO_DEFAULT_PAGESIZE;

        AcquireResources();
    }

    void Batch::Exit()
    {
        DropResources();

        free(batch.buffer);
        batch.buffer = nullptr;
    }

    void Batch::AcquireResources()
    {
        if (batch.flags & BATCH_REMOTE)
            glGenBuffers(1, &batch.vbo);
    }

    void Batch::DropResources()
    {
        if (batch.vbo != 0)
        {
            glDeleteBuffers(1, &batch.vbo);
            batch.vbo = 0;
        }
    }

    void Batch::Begin(size_t capacity, int mode)
    {
        if (batch.flags & BATCH_MMAPPED)
        {
            ZFW_ASSERT(false)
        }
        else
        {
            if (capacity > batch.bufferCapacity)
            {
                // round up to 16 bytes
                batch.bufferCapacity = (capacity + 15) & ~15;

                zr_Dbgprintf(("Begin: rounding capacity %u -> %u\n", (unsigned)capacity, (unsigned)batch.bufferCapacity));

                batch.buffer = realloc(batch.buffer, batch.bufferCapacity);

                if (batch.flags & BATCH_REMOTE)
                {
                    SetArrayBuffer(batch.vbo);
                    glBufferData(GL_ARRAY_BUFFER, batch.bufferCapacity, nullptr, GL_STREAM_DRAW);
                }

                if (glGetError() != 0)
                    zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "Batch::Begin", zfw::Sys::tmpsprintf(50, "OpenGL error $%04X"));
            }
        }

        batch.bufferUsed = 0;
        batch.numVertices = 0;
    }

    /*void Batch::BeginMapped(PrimitiveMode mode, const VertexFormat* format, size_t numVertices)
    {
        SetArrayBuffer(batch.vbo);

        // check buffer capacity
        size_t requiredSize = numVertices * format->vertexSize;

        if (requiredSize > batch.vboCapacity)
        {
            // round up to 16 bytes
            batch.vboCapacity = (requiredSize + 15) & ~15;

            zr_Dbgprintf(("BeginMapped: %u -> %u\n", requiredSize, batch.vboCapacity));

            //batch.mappedBuffer = realloc(batch.mappedBuffer, batch.vboCapacity);

            // realloc buffer with undefined contents
            glBufferData(GL_ARRAY_BUFFER, batch.vboCapacity, nullptr, GL_STREAM_DRAW);

            if (glGetError() != 0)
                zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "Batch::BeginMapped", zfw::Sys::tmpsprintf(50, "OpenGL error $%04X"));
        }

        // map vertex buffer into process virtual memory
        batch.mappedBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

        if (batch.mappedBuffer == nullptr)
            zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "Batch::BeginMapped",
                    zfw::Sys::tmpsprintf(200, "Failed to map vertex buffer into process virtual memory (OpenGL error $%04X)", glGetError()));

        // copy what needs to be copied
        batch.format = *format;
        batch.primitiveMode = gl_primmode[mode];

        batch.vboUsed = 0;
        batch.numVertices = 0;
    }*/

    void Batch::Finish()
    {
        if (batch.flags & BATCH_MMAPPED)
        {
            ZFW_ASSERT(!(batch.flags & BATCH_MMAPPED))
        }
        else
        {
            if (batch.flags & BATCH_REMOTE)
            {
                SetArrayBuffer(batch.vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, batch.bufferUsed, batch.buffer);
            }
        }
    }
}
