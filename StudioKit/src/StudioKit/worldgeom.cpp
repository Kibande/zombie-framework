
#include <StudioKit/worldgeom.hpp>

#include <framework/engine.hpp>
#include <framework/errorbuffer.hpp>

#include <littl/Stream.hpp>

#include <vector>

namespace StudioKit
{
    using namespace zfw;

    struct MatGrp_t
    {
        std::string material;
        std::vector<WorldVertex_t> vertices;
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class WorldGeomTree : public IWorldGeomTree
    {
        public:
            virtual bool Init(zfw::IEngine* sys) override;

            virtual void AddSolidBrush(const WorldVertex_t* vertices, size_t numVertices, size_t material) override;
            virtual size_t GetMaterialByParams(const char* normparams) override;

            //virtual IWorldGeomNode* GetRootNode() = 0;

            virtual bool Process(OutputStream* materials, OutputStream* vertices) override;

        private:
            ErrorBuffer_t* eb;
            IEngine* sys;

            std::vector<MatGrp_t> matGrps;
    };

    // ====================================================================== //
    //  class WorldGeomTree)
    // ====================================================================== //

    IWorldGeomTree* CreateWorldGeomTree()
    {
        return new WorldGeomTree();
    }

    bool WorldGeomTree::Init(zfw::IEngine* sys)
    {
        SetEssentials(sys->GetEssentials());

        this->eb = GetErrorBuffer();
        this->sys = sys;

        return true;
    }

    void WorldGeomTree::AddSolidBrush(const WorldVertex_t* vertices, size_t numVertices, size_t material)
    {
        auto& grp = matGrps[material];

        for (size_t i = 0; i < numVertices; i++)
            grp.vertices.push_back(vertices[i]);
    }

    size_t WorldGeomTree::GetMaterialByParams(const char* normparams)
    {
        for (size_t i = 0; i < matGrps.size(); i++)
            if (matGrps[i].material == normparams)
                return i;

        matGrps.emplace_back(MatGrp_t{ normparams });
        return matGrps.size() - 1;
    }

    bool WorldGeomTree::Process(OutputStream* materials, OutputStream* vertices)
    {
        // ================================================================== //
        //  section zombie.WorldMaterials
        // ================================================================== //

        for (auto& group : matGrps)
            materials->writeString(group.material.c_str());

        // ================================================================== //
        //  section zombie.WorldVertices
        // ================================================================== //

        for (auto& group : matGrps)
        {
            vertices->writeLE<uint32_t>(group.vertices.size());
            vertices->write(group.vertices.data(), group.vertices.size() * sizeof(WorldVertex_t));
        }

        return true;
    }
}
