
#include "n3d_gl_internal.hpp"

#include "../modelloader.hpp"

#include <framework/filesystem.hpp>
#include <framework/system.hpp>

namespace n3d
{
    GLModel::GLModel(String&& path) : state(CREATED), path(std::forward<String>(path))
    {
    }

    GLModel::~GLModel()
    {
        Unrealize();
        Unload();
    }
    
    void GLModel::Draw()
    {
        for (auto& mesh : meshes)
        {
            glr->PushTransform(mesh.transform);
            glr->DrawPrimitives(mesh.vb.get(), mesh.primitiveType, mesh.format, mesh.offset, mesh.count);
            glr->PopTransform();
        }
    }

    Mesh* GLModel::GetMeshByIndex(unsigned int index)
    {
        if (index < meshes.size())
            return &meshes[index];

        return nullptr;
    }

    bool GLModel::IsPathLoadable(const String& path)
    {
        // Only check that path is a file
        FSStat_t stat;
        return g_sys->GetFileSystem()->Stat(path + "/mdl1", &stat) && !stat.isDirectory;
    }

    bool GLModel::Preload(IResourceManager2* resMgr)
    {
        if (this->mdl1.size())
            return true;

        unique_ptr<InputStream> mdl1(g_sys->OpenInput((String) path + "/mdl1"));
        
        if (!mdl1)
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", (const char*) sprintf_t<255>("Failed to load model '%s'.", path.c_str()),
                    "function", li_functionName
                    ), false;

        this->mdl1.resize(mdl1->getSize());
        mdl1->read(&this->mdl1[0], this->mdl1.size());

        return true;
    }

    bool GLModel::Realize(IResourceManager2* resMgr)
    {
        if (meshes.size())
            return true;

        auto vf = glr->GetModelVertexFormat();

        const size_t vertexSize = vf->GetVertexSize();
		const size_t numVertices = mdl1.size() / vertexSize;

        meshes.emplace_back();
        auto& mesh = meshes.back();

		mesh.vb.reset(glr->CreateVertexBuffer());
		mesh.vb->Upload(&mdl1[0], numVertices * vertexSize);
        mdl1.clear();

		mesh.primitiveType = PRIMITIVE_TRIANGLES;
		mesh.format = vf;
		mesh.offset = 0;
		mesh.count = numVertices;

		//mesh.transform = glm::translate(glm::mat4x4(), glm::vec3(512.0f, 640.0f, 0.0f));
        return true;
    }

    void GLModel::Unload()
    {
        mdl1.clear();
    }

    void GLModel::Unrealize()
    {
        for (auto& mesh : meshes)
            mesh.vb.reset();

        meshes.clear();
    }
}
