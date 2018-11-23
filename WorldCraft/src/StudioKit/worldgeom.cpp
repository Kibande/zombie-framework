
#include <StudioKit/worldgeom.hpp>

#include <framework/engine.hpp>
#include <framework/errorbuffer.hpp>

#include <bleb/repository.hpp>

#include <littl+bleb/ByteIOStream.hpp>
#include <littl+bleb/StreamByteIO.hpp>

namespace StudioKit
{
    using namespace li;
    using namespace zfw;

    struct MatGrp_t
    {
        String material;
        List<WorldVertex_t> vertices;
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class WorldGeomTree : public IWorldGeomTree
    {
        public:
            WorldGeomTree();

            virtual void SetOutputFile(unique_ptr<li::IOStream>&& file) override { this->file = std::move(file); }

            virtual bool Init(zfw::IEngine* sys, const char* authored_using) override;

            virtual void AddEntity(const char* entityCfx2) override;
            virtual void AddSolidBrush(const WorldVertex_t* vertices, size_t numVertices, size_t material) override;
            virtual size_t GetMaterialByParams(const char* normparams) override;

            //virtual IWorldGeomNode* GetRootNode() = 0;

            virtual bool Process() override;

        private:
            ErrorBuffer_t* eb;
            IEngine* sys;

            unique_ptr<li::IOStream> file;
            li::StreamByteIO bio;
            bleb::Repository repo;

            List<String> entitiesCfx2;
            List<MatGrp_t> matGrps;
    };

    // ====================================================================== //
    //  class WorldGeomTree)
    // ====================================================================== //

    IWorldGeomTree* CreateWorldGeomTree()
    {
        return new WorldGeomTree();
    }

    WorldGeomTree::WorldGeomTree() : repo(&bio)
    {
    }

    bool WorldGeomTree::Init(zfw::IEngine* sys, const char* authored_using)
    {
        SetEssentials(sys->GetEssentials());

        this->eb = GetErrorBuffer();
        this->sys = sys;

        bio.set(file.get());
        
        if (!repo.open(true))
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", sprintf_4095("bleb error: %s", repo.getErrorDesc()),
                    "function", li_functionName
                    ), false;

        repo.setObjectContents("metadata/authored_using", authored_using, bleb::kPreferInlinePayload);

        return true;
    }

    void WorldGeomTree::AddEntity(const char* entityCfx2)
    {
        entitiesCfx2.add(entityCfx2);
    }

    void WorldGeomTree::AddSolidBrush(const WorldVertex_t* vertices, size_t numVertices, size_t material)
    {
        auto& grp = matGrps[material];

        for (size_t i = 0; i < numVertices; i++)
            grp.vertices.add(vertices[i]);
    }

    size_t WorldGeomTree::GetMaterialByParams(const char* normparams)
    {
        iterate2 (i, matGrps)
            if ((*i).material == normparams)
                return i.getIndex();

        auto& grp = matGrps.addEmpty();
        grp.material = normparams;
        return matGrps.getLength() - 1;
    }

    bool WorldGeomTree::Process()
    {
        // ================================================================== //
        //  section zombie.WorldMaterials
        // ================================================================== //

        auto materials_ = repo.openStream("zombie.WorldMaterials", bleb::kStreamCreate | bleb::kStreamTruncate);
        zombie_assert(materials_);
        ByteIOStream materials(materials_.get());

        iterate2 (i, matGrps)
            materials.writeString((*i).material);

        materials_.reset();

        // ================================================================== //
        //  section zombie.WorldVertices
        // ================================================================== //

        auto vertices_ = repo.openStream("zombie.WorldVertices", bleb::kStreamCreate | bleb::kStreamTruncate);
        zombie_assert(vertices_);
        ByteIOStream vertices(vertices_.get());

        size_t totalVerts = 0;

        iterate2 (i, matGrps)
        {
            vertices.writeLE<uint32_t>((*i).vertices.getLength());
            vertices.write((*i).vertices.c_array(), (*i).vertices.getLength() * sizeof(WorldVertex_t));

            totalVerts += (*i).vertices.getLength();
        }

        vertices_.reset();

        // ================================================================== //
        //  section zombie.Entities
        // ================================================================== //

        if (!entitiesCfx2.isEmpty())
        {
            auto entities_ = repo.openStream("zombie.Entities", bleb::kStreamCreate | bleb::kStreamTruncate);
            zombie_assert(entities_);
            ByteIOStream entities(entities_.get());

            iterate2 (i, entitiesCfx2)
                entities.writeString(*i);
        }

        sys->Printf(kLogInfo, "%8i material groups",    (int) matGrps.getLength());
        sys->Printf(kLogInfo, "%8i faces",              (int) totalVerts / 3);
        sys->Printf(kLogInfo, "%8i total vertices",     (int) totalVerts);
        sys->Printf(kLogInfo, "%8i entities",           (int) entitiesCfx2.getLength());

        // TODO: Partition

        /*
        if (wgt == nullptr)
            return ErrorBuffer::SetError2(eb, EX_OPERATION_FAILED, 2,
                    "desc", mf->GetErrorDesc(),
                    "function", li_functionName
                    ), false;*/

        return true;
    }
}
