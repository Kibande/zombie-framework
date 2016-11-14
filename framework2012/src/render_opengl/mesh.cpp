
#include "rendering.hpp"

#include <zshared/modelfile.hpp>

namespace zr {
    using zshared::MeshDataHeader;

    /*static size_t GetIndexSize(uint32_t numIndices)
    {
        if (numIndices < 0x100)
            return 1;
        else if (numIndices < 0x10000)
            return 2;
        else
            return 4;
    }*/

    /*static size_t GetVertexSize(const Mesh::Blueprint* bp)
    {
        ZFW_ASSERT(bp->coords != nullptr)
        
        size_t s = 12;
        
        if (bp->normals != nullptr)
            s += 12;
        
        if (bp->uvs != nullptr)
            s += 8;
        
        if (bp->colours != nullptr)
            s += 8;
        
        // round up to 16 bytes
        return (s + 15) & ~15;
    }*/

    /*Mesh* Mesh::CreateFromMemory(int flags, const Blueprint* bp)
    {
        li::Object<Mesh> mesh = new Mesh;
        mesh->Init();

        const size_t vertexDataSize = GetVertexSize(bp) * bp->numVertices;
        const size_t indexDataSize = GetIndexSize(bp->numIndices) * bp->numIndices;
        
        mesh->vbuf = new VertexBuffer;
        mesh->vbuf->InitWithSize(flags, vertexDataSize + indexDataSize);
        
        volatile uint8_t *mapped = mesh->vbuf->MapForWriting();

        //mesh->UploadBlueprint();
        
        return nullptr;
    }*/

    Mesh* Mesh::CreateFromMemory(int flags, PrimitiveMode mode, const VertexFormat *format, const void *vertices, size_t numVertices, const void *indices, size_t numIndices)
    {
        const size_t vertexDataSize = format->vertexSize * numVertices;
        const size_t indexDataSize = /*GetIndexSize(numIndices)*/ sizeof(media::VertexIndex_t) * numIndices;
        
        li::Reference<VertexBuffer> vbuf = new VertexBuffer;
        vbuf->InitWithSize(flags, vertexDataSize + indexDataSize);
        vbuf->UploadRange(0, vertexDataSize, vertices);
        vbuf->UploadRange(vertexDataSize, indexDataSize, indices);
        
        li::Object<Mesh> mesh = new Mesh;
        //mesh->InitWithVbuf(mesh->vbuf, flags, mode, format, numVertices, numIndices);
        
        mesh->flags = flags;
        mesh->primmode = gl_primmode[mode];
        mesh->format = *format;
        mesh->numVertices = numVertices;
        mesh->numIndices = numIndices;
        mesh->verticesFirst = 0;
        mesh->indicesOffset = vertexDataSize;
        mesh->vbuf = vbuf.detach();

        return mesh.detach();
    }

    Mesh* Mesh::CreateFromMemory(const media::DIMesh* blueprint)
    {
        ZFW_ASSERT(blueprint->flags & render::R_3D)

        VertexFormat format = {3 * sizeof(float), {3, FMT_FLOAT32, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

        if (blueprint->flags & render::R_NORMALS)
        {
            format.normal.components = 3;
            format.normal.format = FMT_FLOAT32;
            format.normal.offset = format.vertexSize;

            format.vertexSize += 3 * sizeof(float);
        }

        if (blueprint->flags & render::R_UVS)
        {
            format.uv[0].components = 2;
            format.uv[0].format = FMT_FLOAT32;
            format.uv[0].offset = format.vertexSize;

            format.vertexSize += 2 * sizeof(float);
        }

        const size_t vertexDataSize = format.vertexSize * blueprint->numVertices;
        const size_t indexDataSize = sizeof(media::VertexIndex_t) * blueprint->numIndices;

        li::Reference<VertexBuffer> vbuf = new VertexBuffer;
        vbuf->InitWithSize(VBUF_REMOTE, vertexDataSize + indexDataSize);

        volatile uint8_t * vertices = vbuf->MapForWriting();

        ZFW_ASSERT(vertices != nullptr)

        for (size_t i = 0; i < blueprint->numVertices; i++)
        {
            volatile float* p = (volatile float *)(vertices + format.vertexSize * i + format.pos.offset);

            p[0] = blueprint->coords[i * 3];
            p[1] = blueprint->coords[i * 3 + 1];
            p[2] = blueprint->coords[i * 3 + 2];
        }

        if (blueprint->flags & render::R_NORMALS)
            for (size_t i = 0; i < blueprint->numVertices; i++)
            {
                volatile float* p = (volatile float *)(vertices + format.vertexSize * i + format.normal.offset);

                p[0] = blueprint->normals[i * 3];
                p[1] = blueprint->normals[i * 3 + 1];
                p[2] = blueprint->normals[i * 3 + 2];
            }

        if (blueprint->flags & render::R_UVS)
            for (size_t i = 0; i < blueprint->numVertices; i++)
            {
                volatile float* p = (volatile float *)(vertices + format.vertexSize * i + format.uv[0].offset);

                p[0] = blueprint->uvs[i * 2];
                p[1] = blueprint->uvs[i * 2 + 1];
            }
        
        vbuf->Unmap();

        if (indexDataSize > 0)
            vbuf->UploadRange(vertexDataSize, indexDataSize, blueprint->indices.getPtrUnsafe());

        li::Object<Mesh> mesh = new Mesh;

        mesh->flags = (blueprint->layout & media::MESH_INDEXED) ? (zr::MESH_INDEXED) : 0;
        mesh->primmode = (blueprint->format == media::MPRIM_TRIANGLES) ? (GL_TRIANGLES) : (GL_LINES);
        mesh->format = format;
        mesh->numVertices = blueprint->numVertices;
        mesh->numIndices = blueprint->numIndices;
        mesh->verticesFirst = 0;
        mesh->indicesOffset = vertexDataSize;
        mesh->vbuf = vbuf.detach();

        return mesh.detach();
    }

    /*Mesh* Mesh::LoadFromDataBlock(InputStream* stream)
    {
        MeshDataHeader mhdr;

        if (stream->readItems(&mhdr, 1) != 1)
            return nullptr;

        ZFW_ASSERT(mhdr.fmtFlags & zshared::IOFMT_VERTEX3F)

        VertexFormat format = {3 * sizeof(float), {3, FMT_FLOAT32, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

        if (mhdr.fmtFlags & zshared::IOFMT_NORMAL3F)
        {
            format.normal.components = 3;
            format.normal.format = FMT_FLOAT32;
            format.normal.offset = format.vertexSize;

            format.vertexSize += 3 * sizeof(float);
        }

        if (mhdr.fmtFlags & zshared::IOFMT_UV2F)
        {
            format.uv[0].components = 2;
            format.uv[0].format = FMT_FLOAT32;
            format.uv[0].offset = format.vertexSize;

            format.vertexSize += 2 * sizeof(float);
        }

        const size_t vertexDataSize = format.vertexSize * mhdr.numVertices;
        const size_t indexDataSize = sizeof(uint32_t) * mhdr.numIndices;

        li::Reference<VertexBuffer> vbuf = new VertexBuffer;
        vbuf->InitWithSize(VBUF_REMOTE, vertexDataSize + indexDataSize);

        volatile uint8_t * vertices = vbuf->MapForWriting();

        if (vertices == nullptr)
            Sys::Log(ERR_DISPLAY_UNCAUGHT, "VertexBuffer::MapForWriting failed with OpenGL error 0x%04X", glGetError());

        ZFW_ASSERT(vertices != nullptr)

        // Hold your breath... and pray that this doesn't blow up
        stream->read((void *) vertices, vertexDataSize + indexDataSize);

        vbuf->Unmap();

        li::Object<Mesh> mesh = new Mesh;

        mesh->flags = (mhdr.ioFlags & zshared::IOMESH_INDEXED) ? (zr::MESH_INDEXED) : 0;
        mesh->primmode = (mhdr.ioFlags & zshared::IOMESH_LINEPRIM) ? (GL_LINES) : (GL_TRIANGLES);
        mesh->format = format;
        mesh->numVertices = mhdr.numVertices;
        mesh->numIndices = mhdr.numIndices;
        mesh->indicesOffset = vertexDataSize;
        mesh->vbuf = vbuf.detach();

        return mesh.detach();
    }*/

    MeshDataBlob* Mesh::PreloadBlobFromDataBlock(InputStream* stream)
    {
		// FIXME: Doesn't support 16-bit indices
		ZFW_ASSERT(false)

        MeshDataHeader mhdr;

        if (stream->readItems(&mhdr, 1) != 1)
            return nullptr;

        ZFW_ASSERT(mhdr.fmtFlags & zshared::IOFMT_VERTEX3F)

        VertexFormat format = {3 * sizeof(float), {3, FMT_FLOAT32, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

        if (mhdr.fmtFlags & zshared::IOFMT_NORMAL3F)
        {
            format.normal.components = 3;
            format.normal.format = FMT_FLOAT32;
            format.normal.offset = format.vertexSize;

            format.vertexSize += 3 * sizeof(float);
        }

        if (mhdr.fmtFlags & zshared::IOFMT_UV2F)
        {
            format.uv[0].components = 2;
            format.uv[0].format = FMT_FLOAT32;
            format.uv[0].offset = format.vertexSize;

            format.vertexSize += 2 * sizeof(float);
        }

        const size_t vertexDataSize = format.vertexSize * mhdr.numVertices;
        const size_t indexDataSize = sizeof(uint32_t) * mhdr.numIndices;

        uint8_t * data = (uint8_t *)malloc(vertexDataSize + indexDataSize);

        ZFW_ASSERT(data != nullptr)

        stream->read(data, vertexDataSize + indexDataSize);

        MeshDataBlob* blob = new MeshDataBlob;

        blob->data = data;
        blob->vertexDataSize = vertexDataSize;
        blob->indexDataSize = indexDataSize;

        blob->flags = (mhdr.ioFlags & zshared::IOMESH_INDEXED) ? (zr::MESH_INDEXED) : 0;
        blob->primmode = (mhdr.ioFlags & zshared::IOMESH_LINEPRIM) ? (GL_LINES) : (GL_TRIANGLES);
        blob->format = format;
        blob->numVertices = mhdr.numVertices;
        blob->numIndices = mhdr.numIndices;

        return blob;
    }
    
    Mesh::Mesh()
    {
        flags = 0;
        numVertices = 0;
        numIndices = 0;
        verticesFirst = 0;
        indicesOffset = 0;
        isAABBValid = false;
    }

    Mesh::~Mesh()
    {
    }

    void Mesh::BindMaterial()
    {
        ZFW_ASSERT(mat != nullptr)

        mat->Bind();
    }

    void Mesh::GetAABB(Float3 aabb[2])
    {
        ZFW_ASSERT(isAABBValid)

        memcpy(aabb, this->aabb, 2 * sizeof(Float3));
    }

    void Mesh::SetAABB(CFloat3 aabb[2])
    {
         memcpy(this->aabb, aabb, 2 * sizeof(Float3));
         isAABBValid = true;
    }

    void Mesh::RenderVertices()
    {
        ZFW_ASSERT(this->vbuf != nullptr)
        ZFW_ASSERT(vbuf->vbo != 0)
        
        SetArrayBuffer(vbuf->vbo);
        
        SetUpRendering(format);
        
        if (flags & MESH_INDEXED)
        {
            SetElementBuffer(vbuf->vbo);

            glDrawElements(primmode, numIndices, GL_UNSIGNED_SHORT, (void *) indicesOffset);
        }
        else
        {
            glDrawArrays(primmode, 0, numVertices);
        }
        
        //if (glGetError() != 0)
        //    zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "Mesh::Render", zfw::Sys::tmpsprintf(50, "OpenGL error $%04X"));
    }

    void Mesh::Render(const glm::mat4x4& transform)
    {
        glPushMatrix();
        glMultMatrixf( &transform[0][0] );

        //glColor4fv( &render::currentColour[0] );

        RenderVertices();

        glPopMatrix();
    }
}