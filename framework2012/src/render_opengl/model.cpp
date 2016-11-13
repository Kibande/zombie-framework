
#include "rendering.hpp"

#include <zshared/mediafile.hpp>

#include <littl/MemoryMappedFile.hpp>

#include <ctype.h>

namespace zr {
    using zshared::MediaFileReader;

    static bool equals(const VertexFormat& a, const VertexFormat& b)
    {
        if (a.vertexSize != b.vertexSize)
            return false;

        if (a.pos.components != b.pos.components || a.normal.components != b.normal.components
                || a.uv[0].components != b.uv[0].components || a.colour.components != b.colour.components)
            return false;

        if (a.pos.format != b.pos.format || a.normal.format != b.normal.format
                || a.uv[0].format != b.uv[0].format || a.colour.format != b.colour.format)
            return false;

        if (a.pos.offset != b.pos.offset || a.normal.offset != b.normal.offset
                || a.uv[0].offset != b.uv[0].offset || a.colour.offset != b.colour.offset)
            return false;

        return true;
    }

    static uint32_t hash(const VertexFormat& fmt)
    {
        uint32_t h = fmt.vertexSize;
        h ^= (fmt.pos.components << 4) ^ (fmt.normal.components << 6) ^ (fmt.uv[0].components << 8) ^ (fmt.colour.components << 10);
        h ^= (fmt.pos.format << 12) ^ (fmt.normal.format << 14) ^ (fmt.uv[0].format << 16) ^ (fmt.colour.format << 18);
        h ^= (fmt.pos.offset << 20) ^ (fmt.normal.offset << 23) ^ (fmt.uv[0].offset << 26) ^ (fmt.colour.offset << 29);
        return h;
    }

    static int compareByMatIndex(const MeshDataBlob* a, const MeshDataBlob* b)
    {
        return a->materialIndex - b->materialIndex;
    }

    static int compareByVertexFmt(const MeshDataBlob* a, const MeshDataBlob* b)
    {
        return hash(a->format) - hash(b->format);
    }

    Model::Model()
    {
        isAABBValid = false;
    }
    
    Model::~Model()
    {
        iterate2 (i, meshes)
            delete i;
    }
    
    Model* Model::CreateFromMeshes(Mesh** meshes, size_t count)
    {
        Model* model = new Model;
        model->meshes.load(meshes, count);
        return model;
    }

    Model* Model::CreateFromMeshBlobs(MeshDataBlob** blobs, size_t count)
    {
        // FIXME: The single worst function in the whole codebase.

        ZFW_ASSERT(count >= 1)

        const int meshFlags = VBUF_REMOTE;

        // Sort meshes by vertex format first, then by material
        qsort(blobs, count, sizeof(MeshDataBlob *), (int (*)(const void*, const void*)) compareByVertexFmt);
        qsort(blobs, count, sizeof(MeshDataBlob *), (int (*)(const void*, const void*)) compareByMatIndex);

        Object<Model> model = new Model;
        size_t bytesUsed = model->BuildMeshClusters(blobs, count);

        model->sharedVbuf = new VertexBuffer;
        model->sharedVbuf->InitWithSize(meshFlags, bytesUsed);

        iterate2 (i, model->meshClusters)
        {
            auto& mc = *i;

            size_t offset = mc.vertexOffset;
            size_t vertexFirst = 0;

            for (size_t m = mc.first; m < mc.first + mc.count; m++)
            {
                model->sharedVbuf->UploadRange(offset, blobs[m]->vertexDataSize, blobs[m]->data);
                offset += blobs[m]->vertexDataSize;

                Mesh * mesh = new Mesh;

                mesh->flags = MESH_INDEXED;
                mesh->primmode = //PM_TRIANGLE_LIST;
                        GL_TRIANGLES;

                mesh->format = /*blobs[m]->format*/ mc.vf;        // FIXME: this needs to share a common pointer to actually have any positive effect
                mesh->verticesFirst = vertexFirst;
                mesh->vbuf = model->sharedVbuf->reference();

                for (size_t zz = 0; zz < blobs[m]->numIndices; zz++)
                    ((uint32_t*)(blobs[m]->data + blobs[m]->vertexDataSize))[zz] += vertexFirst;

                mesh->SetAABB(blobs[m]->aabb);
                model->meshes.add(mesh);

                vertexFirst += blobs[m]->numVertices;
            }

            for (size_t m = mc.first; m < mc.first + mc.count; m++)
            {
                model->sharedVbuf->UploadRange(offset, blobs[m]->indexDataSize, blobs[m]->data + blobs[m]->vertexDataSize);

                model->meshes[m]->indicesOffset = offset;
                model->meshes[m]->numIndices = blobs[m]->numIndices;

                offset += blobs[m]->indexDataSize;
            }
        }

        return model.detach();
    }

    size_t Model::BuildMeshClusters(MeshDataBlob** blobs, size_t count)
    {
        MeshCluster mc;

        mc.vf = blobs[0]->format;
        mc.first = 0;
        mc.count = 1;

        size_t numVertices = blobs[0]->numVertices;
        size_t numIndices = blobs[0]->numIndices;

        size_t bytesUsed = 0;

        for (size_t i = 1; i <= count; i++)
        {
            if (i < count && equals(blobs[i]->format, mc.vf))
            {
                numVertices += blobs[i]->numVertices;
                numIndices += blobs[i]->numIndices;
                mc.count++;
            }
            else
            {
                mc.vf.pos.offset    += bytesUsed;
                mc.vf.normal.offset += bytesUsed;
                mc.vf.uv[0].offset  += bytesUsed;
                mc.vf.colour.offset += bytesUsed;

                mc.vertexOffset = bytesUsed;
                mc.clusterVertexSize = numVertices * mc.vf.vertexSize;
                mc.clusterIndexSize = numIndices * sizeof(uint32_t);

                //printf("-- Flushing %i]MeshCluster at offset %06X with %06X vertices, %06X indices\n\tand %06X + %06X = %06X total data.\n",
                //        meshClusters.getLength(), bytesUsed, numVertices, numIndices, mc.clusterVertexSize, mc.clusterIndexSize, mc.clusterVertexSize + mc.clusterIndexSize);

                meshClusters.add(mc);

                bytesUsed += mc.clusterVertexSize + mc.clusterIndexSize;

                if (i < count)
                {
                    mc.vf = blobs[i]->format;
                    mc.first = i;
                    mc.count = 1;

                    numVertices = blobs[i]->numVertices;
                    numIndices = blobs[i]->numIndices;
                }
            }
        }

        return bytesUsed;
    }
    
    void Model::GetBoundingBox(Float3& min, Float3& max)
    {
        if (!isAABBValid)
        {
            ZFW_ASSERT(meshes.getLength() >= 1)

            meshes[0]->GetAABB(aabb);

            for (size_t i = 1; i < meshes.getLength(); i++)
            {
                Float3 mbb[2];

                meshes[i]->GetAABB(mbb);

                aabb[0] = glm::min(aabb[0], mbb[0]);
                aabb[1] = glm::max(aabb[1], mbb[1]);
            }

            isAABBValid = true;
        }

        min = aabb[0];
        max = aabb[1];
    }

    Model* Model::LoadFromFile(const char* fileName, bool required)
    {
        // TODO: Proper error logging

        Object<MediaFileReader> reader = MediaFileReader::Open(Sys::OpenInput(fileName));

        if (reader == nullptr)
            return nullptr;

        List<MediaFileReader::Section> sections;
        
        ZFW_ASSERT(reader->ReadSectionTable(sections) >= 0)

        List<Material*> materials;
        List<MeshDataBlob*> blobs;

        //Object<Model> model = new Model;

        iterate2 (i, sections)
        {
            auto& s = *i;

            if (zshared::CmpFourCC(s.table_entry.FourCC, "Matl") == 0)
            {
                Reference<InputStream> input = reader->OpenSection(s);

                Object<Material> mat = Material::LoadFromDataBlock(input);
                materials.add(mat.detach());
            }
            else if (zshared::CmpFourCC(s.table_entry.FourCC, "Mesh") == 0)
            {
                Reference<InputStream> input = reader->OpenSection(s);
                String materialName = input->readString();

                if (isdigit(materialName[0]))
                {
                    unsigned materialIndex = materialName.toInt();

                    ZFW_ASSERT(materialIndex < materials.getLength())

                    //Mesh* mesh = Mesh::LoadFromDataBlock(input);

                    //ZFW_ASSERT (mesh != nullptr)

                    //mesh->SetMaterial(materials[materialIndex]->reference());
                    //model->meshes.add(mesh);

                    MeshDataBlob* preload = Mesh::PreloadBlobFromDataBlock(input);

                    ZFW_ASSERT (preload != nullptr)

                    preload->materialIndex = materialIndex;

                    blobs.add(preload);
                }
            }
            else if (zshared::CmpFourCC(s.table_entry.FourCC, "BBox") == 0)
            {
                Reference<InputStream> input = reader->OpenSection(s);
                uint32_t meshIndex = input->read<uint32_t>();

                /*ZFW_ASSERT(meshIndex < model->meshes.getLength());

                Float3 aabb[2];
                input->readItems(aabb, 2);

                model->meshes[meshIndex]->SetAABB(aabb);*/

                ZFW_ASSERT(meshIndex < blobs.getLength());

                input->readItems(blobs[meshIndex]->aabb, 2);
            }
        }

        // FIXME: create final model
        Object<Model> model = Model::CreateFromMeshBlobs(blobs.getPtrUnsafe(), blobs.getLength());

        iterate2(i, blobs)
            model->meshes[i.getIndex()]->mat = materials[i->materialIndex]->reference();

        // FIXME: release blobs?

        iterate2 (m, materials)
            m->release();

        return model.detach();
    }
    
    void Model::Render(const glm::mat4x4& transform, int rflags)
    {
        if (!(rflags & MODEL_DISABLE_MATERIALS))
        {
            iterate2 (i, meshes)
            {
                i->BindMaterial();
                i->Render(transform);
            }
        }
        else
        {
            iterate2 (i, meshes)
                i->Render(transform);
        }
    }
}