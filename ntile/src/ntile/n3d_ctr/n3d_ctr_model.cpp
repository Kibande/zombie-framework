
#include "n3d_ctr_internal.hpp"

#include <framework/system.hpp>
namespace n3d
{
    CTRModel::~CTRModel()
    {
        iterate2 (i, meshes)
            delete i;
    }
    
    void CTRModel::Draw()
    {
        iterate2 (i, meshes)
        {
            auto mesh = *i;

            //ctrr->PushTransform(mesh->transform);
            //g_sys->Printf(kLogInfo, "%u", mesh->count); 432
            ctrr->DrawPrimitives(mesh->vb.get(), mesh->primitiveType, mesh->format, mesh->offset, mesh->count);
            //ctrr->PopTransform();
        }
    }
    
    Mesh* CTRModel::GetMeshByIndex(unsigned int index)
    {
        if (index < meshes.getLength())
            return meshes[index];

        return nullptr;
    }
}
