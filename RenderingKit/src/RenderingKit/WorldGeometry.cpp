#include "RenderingKitImpl.hpp"

#include <RenderingKit/WorldGeometry.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/errorcheck.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/system.hpp>

#include <littl/Stream.hpp>

#include <vector>

namespace RenderingKit {
    using namespace zfw;

    struct WorldVertex_t {
        float pos[3];
        float normal[3];
        float uv[2];
    };

    static const VertexAttrib_t worldVertexAttribs[] = {
        { "in_Position",    0,  RK_ATTRIB_FLOAT_3 },
        { "in_Normal",      12, RK_ATTRIB_FLOAT_3 },
        { "in_UV",          24, RK_ATTRIB_FLOAT_2 },
        {}
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class WorldGeometry : public IWorldGeometry {
        struct MatGrp_t {
            IMaterial* material;
            shared_ptr<IVertexFormat> vertexFormat;

            size_t numVertices;
            shared_ptr<IGeomChunk> geometry;
        };

    public:
        WorldGeometry(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* path, const char* worldShaderRecipe);

        virtual void Draw() override;

        void AddMaterialGroup(IMaterial* material);
        shared_ptr<IGeomChunk> AllocMatGrpVertices(size_t matIndex, size_t numVertices);
        shared_ptr<IVertexFormat> GetMatGrpVertexFormat(size_t matIndex);

        // IResource2
        bool BindDependencies(IResourceManager2* resMgr);
        bool Preload(IResourceManager2* resMgr);
        void Unload();
        bool Realize(IResourceManager2* resMgr);
        void Unrealize();

        virtual void* Cast(const zfw::TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }

        virtual State_t GetState() const final override { return state; }

        virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override {
            return DefaultStateTransitionTo(this, targetState, resMgr);
        }

    private:
        State_t state = CREATED;
        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;

        std::string path, worldShaderRecipe;

        shared_ptr<IGeomBuffer> gb;

        std::vector<MatGrp_t> matGrps;

        friend class IResource2;
    };

    unique_ptr<IWorldGeometry> p_CreateWorldGeometry(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
            const char* path, const char* worldShaderRecipe)
    {
        return std::make_unique<WorldGeometry>(eb, rk, rm, path, worldShaderRecipe);
    }

    // ====================================================================== //
    //  class WorldGeometry
    // ====================================================================== //

    WorldGeometry::WorldGeometry(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
            const char* path, const char* worldShaderRecipe)
        : eb(eb), rk(rk), rm(rm), path(path), worldShaderRecipe(worldShaderRecipe)
    {
    }

    void WorldGeometry::AddMaterialGroup(IMaterial* material) {
        matGrps.emplace_back(MatGrp_t{ material, nullptr, 0, nullptr });
    }

    shared_ptr<IGeomChunk> WorldGeometry::AllocMatGrpVertices(size_t matIndex, size_t numVertices) {
        zombie_assert(matIndex < matGrps.size());

        auto& group = matGrps[matIndex];
        group.geometry.reset(gb->CreateGeomChunk());
        return group.geometry;
    }

    bool WorldGeometry::BindDependencies(IResourceManager2* resMgr) {
        // TODO

        return true;
    }

    void WorldGeometry::Draw() {
        for (auto& group : matGrps) {
            rm->DrawPrimitives(group.material, RK_TRIANGLES, group.geometry.get());
        }
    }

    shared_ptr<IVertexFormat> WorldGeometry::GetMatGrpVertexFormat(size_t matIndex) {
        auto& grp = matGrps[matIndex];
        return grp.vertexFormat;
    }

    bool WorldGeometry::Preload(IResourceManager2* resMgr) {
        // TODO

        return true;
    }

    bool WorldGeometry::Realize(IResourceManager2* resMgr) {
        gb = rm->CreateGeomBuffer("WorldGeometry.gb");
        ErrorCheck(gb);

        // ================================================================== //
        //  Move to WorldLoader: section zombie.WorldMaterials
        // ================================================================== //

        std::unique_ptr<InputStream> materials(rk->GetSys()->OpenInput("zombie.WorldMaterials"));

        if (materials == nullptr)
            return ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 1,
                "desc", "The map is corrupted (missing section zombie.WorldMaterials)"
            ), false;

        while (!materials->eof()) {
            auto desc = materials->readString();

            const char* texture;
            if (!Params::GetValueForKey(desc, "texture", texture))
                texture = "path=media/texture/_NULL.jpg";

            char* materialRecipe = Params::BuildAlloc(2,
                "shader", worldShaderRecipe.c_str(),
                "texture:tex", texture
            );

            auto material = resMgr->GetResource<IMaterial>(materialRecipe, IResourceManager2::kResourceRequired);
            free(materialRecipe);

            ErrorCheck(material);

            AddMaterialGroup(material);
        }

        materials.reset();

        // ================================================================== //
        //  END Move to WorldLoader: section zombie.WorldMaterials
        // ================================================================== //

        for (auto& group : matGrps) {
            group.vertexFormat = rm->CompileVertexFormat(group.material->GetShader(), 32, worldVertexAttribs, false);
        }

        // ================================================================== //
        //  Move to WorldLoader: section zombie.WorldVertices
        // ================================================================== //

        std::unique_ptr<InputStream> vertices(rk->GetSys()->OpenInput("zombie.WorldVertices"));

        if (vertices == nullptr)
            return ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 1,
                "desc", "The map is corrupted (missing section zombie.WorldVertices)"
            ), false;

        std::vector<uint8_t> vertexBuffer;

        for (size_t matIndex = 0; !vertices->eof(); matIndex++) {
            zombie_assert(matIndex < matGrps.size());

            uint32_t numVertsForMaterial = 0;

            vertices->readLE(&numVertsForMaterial);

            auto gc = AllocMatGrpVertices(matIndex, numVertsForMaterial);
            zombie_assert(gc != nullptr);

            auto fmt = GetMatGrpVertexFormat(matIndex);
            gc->AllocVertices(fmt.get(), numVertsForMaterial, 0);

            vertexBuffer.resize(numVertsForMaterial * fmt->GetVertexSize());
            vertices->read(&vertexBuffer[0], numVertsForMaterial * fmt->GetVertexSize());

            gc->UpdateVertices(0, &vertexBuffer[0], numVertsForMaterial * fmt->GetVertexSize());
        }

        vertices.reset();

        // ================================================================== //
        //  END Move to WorldLoader: section zombie.WorldVertices
        // ================================================================== //

        return true;
    }

    void WorldGeometry::Unload() {
    }

    void WorldGeometry::Unrealize() {
        matGrps.clear();
        gb.reset();
    }
}
