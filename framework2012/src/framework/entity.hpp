#pragma once

#include <framework/event.hpp>
#include <framework/framework.hpp>

#include <littl/cfx2.hpp>

#define DECL_PARTIAL_SERIALIZE_UNSERIALIZE() virtual int Serialize(World* world, OutputStream* output, int flags) override;\
virtual int Unserialize(World* world, InputStream* input, int flags) override;

#define DECL_SERIALIZE_UNSERIALIZE()\
    virtual int FullSerialize(World* world, OutputStream* output, int flags) override;\
    DECL_PARTIAL_SERIALIZE_UNSERIALIZE()

#define SERIALIZE_PARTIAL_BEGIN(class_) int class_::Serialize(World* world, OutputStream* output, int flags) {
#define SERIALIZE_BEGIN(class_, entname_) int class_::FullSerialize(World* world, OutputStream* output, int flags) {\
    output->writeString(entname_);\
    output->write<int>(GetID());\
    return Serialize(world, output, flags);\
}\
\
SERIALIZE_PARTIAL_BEGIN(class_)
#define SERIALIZE_END }

#define UNSERIALIZE_BEGIN(class_) int class_::Unserialize(World* world, InputStream* input, int flags) {\
    if (input != nullptr) {
#define UNSERIALIZE_LINK } else {
#define UNSERIALIZE_END } }

namespace zfw
{
    NAME(ACTOR_MOVEMENT_JUMP)
    NAME(ACTOR_MOVEMENT_WALK)

    NAME(DRAW_HUD_LAYER)
    NAME(DRAW_MASK)
    NAME(DRAW_STATIC)

    NAME(IENTITY)
    NAME(ABSTRACTENTITY)
    NAME(POINTENTITY)

    class ICollisionHandler;
    class World;

    /*
        zombie framework entity system
            IEntity -- AbstractEntity
                        -- AIEntity
                        -+ control_sideview
                    -- PointEntity
                        -- ActorEntity

        some rules on entity declarations:
            ALL METHODS (except static Create()) must be non-inline
            every non-abstract entity class must implement GetInterface (if necessary)
    */

    struct EntityClassInfo
    {
        const char* entName;
    };

    class IEntity
    {
        public:
            virtual ~IEntity() {}

            virtual void            Init() {}
            virtual void*           GetInterface(Name interfaceName) { return (interfaceName == IENTITY) ? this : nullptr; }
            virtual int             FullSerialize(World* world, OutputStream* output, int flags) { return -1; }
            virtual int             Serialize(World* world, OutputStream* output, int flags) { return -1; }
            virtual int             Unserialize(World* world, InputStream* input, int flags) { return -1; }

            virtual void            AcquireResources() {}
            virtual void            DropResources() {}

            virtual void            Draw(Name name) {}
            virtual int             OnEvent(int h, const Event_t* ev) { return h; }
            virtual void            OnFrame(double delta) {}

            virtual int             GetID() const = 0;
            virtual void            SetID(int entID) = 0;
            virtual const Float3&   GetPos() = 0;
            virtual void            SetPos(const Float3& pos) = 0;
    };

    //
    // Built-in Entity Types: Abstract, Point
    //

    class AbstractEntity : public IEntity
    {
        protected:
            int entID;

            // defined even for Abstract entities for convenient usage in world editor
            Float3 pos;

        public:
            AbstractEntity() : entID(-1) {}
        
            virtual int             GetID() const override { return entID; }
            virtual void            SetID(int entID) override { this->entID = entID; }
            virtual void*           GetInterface(Name interfaceName) override;
            virtual CFloat3&        GetPos() override { return pos; }
            virtual void            SetPos(CFloat3& pos) override { this->pos = pos; }
    };

    class PointEntity : public IEntity
    {
        protected:
            int entID;

            Float3 orientation, pos, speed;

            ICollisionHandler* collHandler;

        public:
            PointEntity() : entID(-1), collHandler(nullptr) {}
            DECL_PARTIAL_SERIALIZE_UNSERIALIZE()

            virtual void            OnFrame(double delta) override;

            virtual bool            GetBoundingBox(Float3& min, Float3& max) { return false; }
            virtual bool            GetBoundingBoxForPos(CFloat3& newPos, Float3& min, Float3& max);
            virtual int             GetID() const override { return entID; }
            virtual void            SetID(int entID) override { this->entID = entID; }
            virtual void*           GetInterface(Name interfaceName) override;
            virtual CFloat3&        GetPos() override { return pos; }
            virtual CFloat3&        GetOrientation() { return orientation; }
            virtual CFloat3&        GetSpeed() { return speed; }

            virtual void            SetCollisionHandler(ICollisionHandler* handler) { collHandler = handler; }
            virtual void            SetPos(CFloat3& pos) override { this->pos = pos; }
            virtual void            SetOrientation(CFloat3& orientation) { this->orientation = orientation; }
            virtual void            SetSpeed(CFloat3& speed) { this->speed = speed; }
    };

    //
    // Built-in Abstract Entity Superclasses: AIEntity
    //

    class AIEntity : public AbstractEntity
    {
        double ticksAccum;
        
        protected:
            AIEntity();
        
        public:
            virtual void OnFrame(double delta) override;
            virtual void OnThink(int ticks) = 0;
    };

    //
    // Built-in Point Entity Superclasses: ActorEntity
    //

    class ActorEntity : public PointEntity
    {
        public:
            virtual void    SetMovement(bool enable, Name movementType, CFloat3& direction) = 0;
    };

    //
    // Built-in Abstract Entity Classes: control_1stperson, control_sideview
    //

    /*class control_1stperson : public AbstractEntity
    {
        public:
            enum { K_FWD, K_BACK, K_LEFT, K_RIGHT, K_MAX };

            struct Controls {
                Vkey_t keys[K_MAX];
            };

        protected:
            ActorEntity         *linked_ent;

            Controls ctl;
            float speed;

            int state[K_MAX];

        public:
            control_1stperson();
            static IEntity* Create() { return new control_1stperson; }

            virtual Controls& GetControls() { return ctl; }
            virtual bool LinkEntity(ActorEntity* ent);
            virtual int OnEvent(int h, const Event_t* ev) override;
            virtual void OnFrame(double delta) override;
            void SetSpeed(float speed) { this->speed = speed; }
    };*/
    
    class control_sideview : public AbstractEntity
    {
        public:
            enum { K_LEFT, K_RIGHT, K_JUMP, K_MAX };
            
            struct Controls {
                Vkey_t keys[K_MAX];
            };
            
        protected:
            ActorEntity     *linked_ent;
            Controls        ctl;
            
            int state[K_MAX];
            
        public:
            control_sideview();
            static IEntity* Create() { return new control_sideview; }
            
            virtual Controls& GetControls();
            virtual void LinkEntity(ActorEntity* ent);
            virtual int OnEvent(int h, const Event_t* ev) override;
    };

    //
    //
    //
    
    class IEntityFilter
    {
        public:
            virtual void OnAddEntity(IEntity* ent) = 0;
            virtual void OnRemoveEntity(IEntity* ent) = 0;
    };

    template <class Interface>
    class EntityCollectorByInterface: public IEntityFilter
    {
        protected:
            const Name interfaceName;
            List<Interface*> entities;

        public:
            EntityCollectorByInterface(Name interfaceName) : interfaceName(interfaceName) {}

            auto GetList() -> decltype(entities)& { return entities; }
            virtual void OnAddEntity(IEntity* ent) override;
            virtual void OnRemoveEntity(IEntity* ent) override;
    };

    class Entity
    {
        public:
            static IEntity* Create(const char* entdef, bool required);
            //static IEntity* LoadFromFile(const char* path, int preferredAppearance, bool required);

            static void Register(const char* entdef, IEntity* (* Create)());
    };
    
    class World
    {
        friend class StageLayerWorld;
        
        protected:
            List<IEntity*>          entities;
            List<IEntityFilter*>    entityFilters;
        
        public:
            ~World();
            
            void AddEntity(IEntity* ent);
            void AddEntityFilter(IEntityFilter* filter);
            void Draw(Name name);
            IEntity* GetEntityByID(int entID);
            auto GetEntityList() -> decltype(entities)& { return entities; }
            void OnFrame(double delta);
            void RemoveAllEntities(bool destroy);
            void RemoveEntity(IEntity* ent, bool destroy);
            void RemoveEntityFilter(IEntityFilter* filter);
            void Serialize(OutputStream* output, int flags);
            void Unserialize(InputStream* input, int flags);
    };

    //
    // Entity- and World-related Handlers
    //

    class ICollisionHandler
    {
        public:
            virtual bool CHCanMove(PointEntity *ent, CFloat3& pos, CFloat3& newPos) = 0;
    };
}

namespace zfw
{
    template <class Interface> void EntityCollectorByInterface<Interface>::OnAddEntity(IEntity* ent)
    {
        Interface* iface = reinterpret_cast<Interface*>(ent->GetInterface(interfaceName));

        if (iface != nullptr)
            entities.add(iface);
    }

    template <class Interface> void EntityCollectorByInterface<Interface>::OnRemoveEntity(IEntity* ent)
    {
        Interface* iface = reinterpret_cast<Interface*>(ent->GetInterface(interfaceName));

        if (iface != nullptr)
            entities.removeItem(iface);
    }
}