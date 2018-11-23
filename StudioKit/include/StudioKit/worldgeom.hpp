#pragma once

#include <framework/base.hpp>
#include <framework/datamodel.hpp>
#include <framework/modulehandler.hpp>

#include <reflection/magic.hpp>

namespace StudioKit
{
    class IWorldGeomNode;
    //class IWorldMaterial;
    //class IWorldVertexFormat;

    struct WorldVertex_t
    {
        zfw::Float3 pos;
        zfw::Float3 normal;
        zfw::Float2 uv;
    };

    class IWorldGeomTree
    {
        public:
            virtual ~IWorldGeomTree() {}

            virtual bool Init(zfw::IEngine* sys) = 0;

            virtual void AddSolidBrush(const WorldVertex_t* vertices, size_t numVertices, size_t material) = 0;
            virtual size_t GetMaterialByParams(const char* normparams) = 0;

            //virtual IWorldGeomNode* GetRootNode() = 0;

            virtual bool Process(zfw::OutputStream* materials, zfw::OutputStream* vertices) = 0;

            REFL_CLASS_NAME("IWorldGeomTree", 1)
    };

    //zombie_interface3(IWorldGeomNode, 0x727a3f96)
    //zombie_end_interface

    /*zombie_interface3(IWorldMaterial, 0)
    ;
    zombie_end_interface*/

    //zombie_interface3(IWorldVertexFormat, 0)
    //zombie_end_interface

#ifdef STUDIO_KIT_STATIC
    IWorldGeomTree* CreateWorldGeomTree();

    ZOMBIE_IFACTORYLOCAL(WorldGeomTree)
#else
    ZOMBIE_IFACTORY(WorldGeomTree, "StudioKit")
#endif
}
