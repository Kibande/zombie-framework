#pragma once

#include "../nbase.hpp"

#include <RenderingKit/RenderingKit.hpp>

#include <zshared/mediafile2.hpp>

#include <framework/base.hpp>
#include <framework/datamodel.hpp>

#include <littl/cfx2.hpp>
#include <littl/List.hpp>
#include <littl/HashMap.hpp>

#include <glm/gtc/quaternion.hpp>

namespace ntile
{
    struct Joint_t;
    struct MeshBuildContext_t;
    struct ModelVertex;

    class IOnAnimationEnding;

    struct AABB_t
    {
        Float3 min, max;
        Joint_t* joint;
    };

    struct Joint_t
    {
        String name;

        Float3 pos, diffpos;
        glm::quat boneRotation;

        glm::mat4 boneToParentSpace;
        glm::mat4 boneToModelSpace;
        glm::mat4 modelToBoneSpace;

        li::List<Joint_t*> children;

        bool dirty, wasDirty, highlighted;
        uint32_t skeletonMeshFirst;

        Joint_t()
        {
            dirty = true;
            highlighted = false;
        }

        ~Joint_t()
        {
            children.deleteAllItems();
        }

        Joint_t* Find(const char* name)
        {
            if (this->name == name)
                return this;

            Joint_t* joint;

            for (auto child : children)
                if ((joint = child->Find(name)) != nullptr)
                    return joint;

            return nullptr;
        }
    };

    struct JointRange_t
    {
        uint32_t first, count;
        Joint_t* joint;
    };

    struct StudioBoneAnimKeyframe_t
    {
        float time;
        Float3 pitchYawRoll;
    };

    struct StudioBoneAnim_t
    {
        size_t index;   // can't store BoneAnim_t* because it's in a list

        Joint_t* joint;

        List<StudioBoneAnimKeyframe_t> keyframes;
    };

    struct StudioPrimitive_t
    {
        enum Type { CUBOID };

        int type;
        Float3 a, b;
        Byte4 colour;

        String bone;
        bool collision;
    };

    struct StudioMesh_t
    {
        String name;
        List<StudioPrimitive_t*> primitives;
    };

    struct Mesh
    {
        RenderingKit::IGeomChunk* gc;
        RenderingKit::RKPrimitiveType_t primitiveType;
        RenderingKit::IVertexFormat* formatRef;
        uint32_t offset, count;
        
        glm::mat4x4 transform;
    };

    class CharacterModel
    {
        public:
            struct Animation;

            CharacterModel(ErrorBuffer_t* eb, IResourceManager2* res);
            ~CharacterModel();

            // Basic
            void Draw();
            bool Load(const char* path);
            bool Save(zshared::MediaFile* modelFile);

            // Collision
            Float3& GetAABBMin() { return aabbMinMax[0]; }
            Float3& GetAABBMax() { return aabbMinMax[1]; }

            // Skeleton, animations
            void AnimationTick();
            Joint_t* FindJoint(const char* name);
            Animation* GetAnimationByName(const char* name);
            Float3 GetJointPos(Joint_t* joint);
            void SetAnimTimeScale(float ts) { timeScale = ts; }
            void StartAnimation(Animation* anim);
            void StopAllAnimations();

            Animation* StudioAddAnimation(const char* name, float duration);
            void StudioAddKeyframeAtTime(Animation* anim, StudioBoneAnim_t* sa, float time, const StudioBoneAnimKeyframe_t* data);
            void StudioAnimationTick();
            void StudioDrawBones();
            float StudioGetAnimationDuration(Animation* anim);
            const char* StudioGetAnimationName(size_t index);
            const char* StudioGetAnimationName(Animation* anim);
            StudioBoneAnim_t* StudioGetBoneAnimation(Animation* anim, Joint_t* joint);
            void StudioGetBoneAnimationAtTime(StudioBoneAnim_t* sa, float time, Float3& pitchYawRoll_out);
            Joint_t* StudioGetJoint(Joint_t* parent, size_t index);
            bool StudioGetPrimitivesList(size_t meshIndex, StudioPrimitive_t**& list_out, size_t& count_out);
            StudioBoneAnimKeyframe_t* StudioGetKeyframeByIndex(StudioBoneAnim_t* sa, int index);
            intptr_t StudioGetKeyframeIndexNear(StudioBoneAnim_t* sa, float time, int direction);
            void StudioRemoveKeyframe(Animation* anim, StudioBoneAnim_t* sa, int index);
            void StudioSetAnimationTime(Animation* anim, float time);
            void StudioSetHighlightedJoint(Joint_t* joint);
            void StudioSetOnAnimationEnding(IOnAnimationEnding* listener) { onAnimationEnding = listener; }
            void StudioStartAnimationEditing(Animation* anim);
            void StudioStopAnimationEditing(Animation* anim);
            void StudioUpdateKeyframe(Animation* anim, StudioBoneAnim_t* sa, int index);

        protected:
            ErrorBuffer_t* eb;
            IResourceManager2* res;

            // Base Mesh
            RenderingKit::IShader* program;
            shared_ptr<RenderingKit::IGeomBuffer> vbRef;
            shared_ptr<RenderingKit::IVertexFormat> vfRef;
            unique_ptr<RenderingKit::IMaterial> materialRef;

            List<Mesh*> meshes;
            RenderingKit::IGeomChunk* skeletonMesh;

            // Studio-specific
            List<StudioMesh_t*> studioMeshes;

            // Collisons
            List<AABB_t> collisions;

            // Skeleton, animations
            std::unique_ptr<Joint_t> skeleton;
            List<Animation*> animations;

            List<Animation*> activeAnims;
            bool anyRunningAnimations, forceSingleRefresh;

            // Studio-specific
            float timeScale;
            Joint_t* highlightedJoint;
            int highlightTicks;
            IOnAnimationEnding* onAnimationEnding;

            // Skinning
            ModelVertex* vertices;
            List<JointRange_t> jointRanges;

            Float3 aabbMinMax[2];

            void AddAnimation(cfx2::Node animNode);
            bool AddCuboid(MeshBuildContext_t* ctx, cfx2::Node cuboidNode);
            bool BuildMesh(cfx2::Node meshNode, Mesh*& mesh);

            void UpdateAnyRunningAnimations();
            void UpdateCollisions();
            void UpdateJoint(Joint_t* joint, Joint_t* parent, bool force);
            void UpdateJointRange(const JointRange_t& range);
            void UpdateVertices();
            void WriteJoint(OutputStream* os, Joint_t* joint);
    };

    class IOnAnimationEnding
    {
        public:
            virtual void OnAnimationEnding(CharacterModel* mdl, CharacterModel::Animation* anim) = 0;
    };
}
