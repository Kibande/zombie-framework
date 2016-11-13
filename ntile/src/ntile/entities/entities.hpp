#pragma once

#include "../items.hpp"
#include "../nbase.hpp"
#include "../n3d.hpp"
#include "../stats.hpp"

#include <framework/abstractentity.hpp>
#include <framework/pointentity.hpp>

#include <littl/cfx2.hpp>
#include <littl/HashMap.hpp>

#include <vector>

namespace ntile
{
    extern UUID_t DRAW_ENT_PICKING;
    extern UUID_t DRAW_EDITOR_MODE;

    struct Joint_t;
    struct MeshBuildContext_t;
    struct ModelVertex;
    struct WorldVertex;

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

    class CharacterModel : public IResource2
    {
        friend class IResource2;

        public:
            struct Animation;

            CharacterModel(String&& path);
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

            // Basic
            void Draw();

            // Collision
            Float3& GetAABBMin() { return aabbMinMax[0]; }
            Float3& GetAABBMax() { return aabbMinMax[1]; }

            // Skeleton, animations
            void AnimationTick();
            Joint_t* FindJoint(const char* name);
            Animation* GetAnimationByName(const char* name);
            Float3 GetJointPos(Joint_t* joint);
            void StartAnimation(Animation* anim);

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
            String path;

            // Base Mesh
            unique_ptr<IModel> model;

            // Collisons
            List<AABB_t> collisions;

            // Skeleton, animations
            unique_ptr<Joint_t> skeleton;
            HashMap<String, Animation*> animations;

            List<Animation*> activeAnims;
            bool anyRunningAnimations, forceSingleRefresh;

            // Skinning
            IVertexBuffer* vb;
            IVertexFormat* vf;
            ModelVertex* vertices;
            List<JointRange_t> jointRanges;

            Float3 aabbMinMax[2];
    };

    class IEntityMovementListener
    {
        public:
            virtual void OnSetPos(IPointEntity* pe, const Float3& oldPos, const Float3& newPos) = 0;
    };

    class ICommonEntity
    {
        public:
            virtual void EditingModeInit() {}

            virtual void OnClicked(IGameScreen* gs, int button) {}

            virtual void SetMovementListener(IEntityMovementListener* listener) {}
    };
}

namespace ntile
{
namespace entities
{
    class char_base : public PointEntityBase, public ICommonEntity
    {
        protected:
            IModel* model;
            Float3 aabbMin, aabbMax;
            float angle;

            BaseStats baseStats;
            CharacterStats stats;

            glm::mat4x4 modelView;
        
            IEntityMovementListener* movementListener;

            char_base(Int3& pos, float angle);

            virtual void Draw(const UUID_t* modeOrNull) override;
        
        public:
            char_base() { model = nullptr; movementListener = nullptr; }

            virtual bool GetAABB(Float3& min, Float3& max) override;
            virtual bool GetAABBForPos(const Float3& newPos, Float3& min, Float3& max) override;

            const glm::mat4x4& GetModelView() { return modelView; }

            virtual void SetMovementListener(IEntityMovementListener* listener) override { movementListener = listener; }
            virtual void SetPos(const Float3& pos) override;
    };

    class char_player : public char_base
    {
        protected:
            Int2 motionVec;
            int t, lastAnim;

            Joint_t* sword_tip;

        public:
            char_player(Int3 pos, float angle);

            virtual void OnTick() override;

            void AddMotionX(int vec) { motionVec.x += vec; }
            void AddMotionY(int vec) { motionVec.y += vec; }

            Int2 GetMotionVec() { return motionVec; }

            Float3 Hack_GetTorchLightPos();
            void Hack_ShieldAnim();
            void Hack_SlashAnim();
    };

    class prop_base : public PointEntityBase, public IEntityReflection, public ICommonEntity
    {
        protected:
            String modelPath;

            String name_buffer;
            IModel* model;

            ITexture* editorImage;

            IEntityMovementListener* movementListener;

        public:
            prop_base();

            DECL_SERIALIZE_UNSERIALIZE()

            virtual int ApplyProperties(cfx2_Node* properties) override;
            virtual bool Init() override;
            virtual void EditingModeInit() override;

            virtual void Draw(const UUID_t* modeOrNull) override;

            virtual bool GetAABB(Float3& min, Float3& max) override;
            virtual bool GetAABBForPos(const Float3& newPos, Float3& min, Float3& max) override;

            virtual void SetMovementListener(IEntityMovementListener* listener) override { movementListener = listener; }
            virtual void SetPos(const Float3& pos) override;

            // zfw.IEntityReflection
            virtual IEntity* GetEntity() override { return this; }
            virtual size_t GetNumProperties() override;
            virtual bool GetRWProperties(NamedValuePtr* buffer, size_t count) override;
    };

    /*class prop_tree : public prop_base
    {
        public:
            prop_tree(CFloat3& pos);
            static IEntity* Create() { return new prop_tree(Float3()); }
            DECL_SERIALIZE_UNSERIALIZE()
    };*/

    class prop_treasurechest : public prop_base
    {
        public:
            prop_treasurechest();
            DECL_SERIALIZE_UNSERIALIZE()

            virtual void OnClicked(IGameScreen* gs, int button) override;
    };

    class water_body : public PointEntityBase
    {
        public:
            water_body();
            ~water_body();

            virtual bool Init() override;

            virtual void Draw(const UUID_t* uuidOrNull) override;
            virtual void OnTick() override;

        private:
            void InitializeVertices(WorldVertex* p_vertices);
            void RandomizeHeightmap();
            void ResetGeometry();
            void UpdateGeometry();
            void UpdateHeightmap();

            Int2 tiles;
            size_t numVertices;

            std::vector<int16_t> waveHeightmap, randomHeightmap;

            unique_ptr<n3d::IVertexBuffer> vertexBuf;
    };

    class abstract_base : public AbstractEntityBase, public IEntityReflection, public ICommonEntity
    {
        String editorImagePath;

        String name_buffer;
        ITexture* editorImage;

        public:
            abstract_base();

            DECL_SERIALIZE_UNSERIALIZE()

            virtual int ApplyProperties(cfx2_Node* properties) override;
            virtual bool Init() override;
            virtual void EditingModeInit() override;

            virtual void Draw(const UUID_t* modeOrNull) override;

            // zfw.IEntityReflection
            virtual IEntity* GetEntity() override { return this; }
            virtual size_t GetNumProperties() override;
            virtual bool GetRWProperties(NamedValuePtr* buffer, size_t count) override;
    };

    /*class weapon_base : extends(PointEntityBase)
    {
    };

    class weapon_melee : extends(weapon_base)
    {
    };

    class weapon_sword_base : extends(weapon_melee)
    {
    };*/
}
}
