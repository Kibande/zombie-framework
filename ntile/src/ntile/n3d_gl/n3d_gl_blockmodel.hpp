#ifndef NTILE_N3D_GL_BLOCKMODEL_HPP
#define NTILE_N3D_GL_BLOCKMODEL_HPP

#include "../nbase.hpp"
#include "../n3d.hpp"

#include <littl/cfx2.hpp>
#include <littl/HashMap.hpp>
#include <littl/String.hpp>

namespace n3d
{
    struct Joint_t;
    struct MeshBuildContext_t;
    struct ModelVertex;

    struct AABB_t
    {
        Float3 min, max;
        Joint_t* joint;
    };

    struct JointRange_t
    {
        uint32_t first, count;
        Joint_t* joint;
    };

    // IModel for now; we'll need to decide whether we're to keep this class or chuck it away
    class CharacterModel : public IModel
    {
        friend class IResource2;

    public:
        CharacterModel(li::String&& path);
        ~CharacterModel();

        bool BindDependencies(IResourceManager2* resMgr) { return true; }
        bool Preload(IResourceManager2* resMgr);
        void Unload();
        bool Realize(IResourceManager2* resMgr);
        void Unrealize();

        // IResource2*
        virtual void* Cast(const std::type_index& resourceClass) final override { return DefaultCast(this, resourceClass); }
        virtual State_t GetState() const final override { return state; }
        virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
        {
            return DefaultStateTransitionTo(this, targetState, resMgr);
        }

        // IModel
        void Draw() override;
        Mesh* GetMeshByIndex(unsigned int index) override;

        void AnimationTick() override;
        Joint_t* FindJoint(const char* name) override;
        Animation* GetAnimationByName(const char* name) override;
        Float3 GetJointPos(Joint_t* joint) override;
        void StartAnimation(Animation* anim) override;

        // Collision
        Float3& GetAABBMin() { return aabbMinMax[0]; }
        Float3& GetAABBMax() { return aabbMinMax[1]; }

    protected:
        void AddAnimation(cfx2::Node animNode);
        bool AddCuboid(MeshBuildContext_t* ctx, cfx2::Node cuboidNode);
        bool BuildMesh(cfx2::Node meshNode, Mesh*& mesh);

        void UpdateAnyRunningAnimations();
        void UpdateCollisions();
        void UpdateJoint(Joint_t* joint, Joint_t* parent, bool force);
        void UpdateJointRange(const JointRange_t& range);
        void UpdateVertices();

        State_t state;
        li::String path;

        // Base Mesh
        unique_ptr<IModel> model;

        // Collisons
        li::List<AABB_t> collisions;

        // Skeleton, animations
        unique_ptr<Joint_t> skeleton;
        li::HashMap<li::String, Animation*> animations;

        li::List<Animation*> activeAnims;
        bool anyRunningAnimations, forceSingleRefresh;

        // Skinning
        IVertexBuffer* vb;
        IVertexFormat* vf;
        ModelVertex* vertices;
        li::List<JointRange_t> jointRanges;

        Float3 aabbMinMax[2];
    };
}

#endif //NTILE_N3D_GL_BLOCKMODEL_HPP
