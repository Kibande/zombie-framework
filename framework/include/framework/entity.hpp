#pragma once

#include <framework/event.hpp>
#include <framework/framework.hpp>

struct cfx2_Node;

#define DECL_PARTIAL_SERIALIZE_UNSERIALIZE() virtual int Serialize(EntityWorld* world, OutputStream* output,\
        int flags) override;\
virtual int Unserialize(EntityWorld* world, InputStream* input, int flags) override;

#define DECL_SERIALIZE_UNSERIALIZE()\
    virtual int FullSerialize(EntityWorld* world, OutputStream* output, int flags) override;\
    DECL_PARTIAL_SERIALIZE_UNSERIALIZE()

#define SERIALIZE_PARTIAL_BEGIN(class_) int class_::Serialize(EntityWorld* world, OutputStream* output, int flags) {

#define SERIALIZE_BEGIN(class_, entname_) int class_::FullSerialize(EntityWorld* world, OutputStream* output,\
        int flags) {\
    output->writeString(entname_);\
    output->writeLE<int32_t>(GetID());\
    return Serialize(world, output, flags);\
}\
\
SERIALIZE_PARTIAL_BEGIN(class_)

#define SERIALIZE_BEGIN_2(class_) int class_::FullSerialize(EntityWorld* world, OutputStream* output, int flags) {\
    output->writeString(GetName());\
    output->writeLE<int32_t>(GetID());\
    return Serialize(world, output, flags);\
}\
\
SERIALIZE_PARTIAL_BEGIN(class_)

#define SERIALIZE_END }

#define UNSERIALIZE_BEGIN(class_) int class_::Unserialize(EntityWorld* world, InputStream* input, int flags) {\
    if (input != nullptr) {
#define UNSERIALIZE_LINK } else {
#define UNSERIALIZE_END } }

namespace zfw
{
    class IEntity
    {
        public:
            virtual ~IEntity() {}

            virtual int             ApplyProperties(cfx2_Node* properties) { return -1; }
            virtual int             Unserialize(EntityWorld* world, InputStream* input, int flags) { return -1; }
            virtual bool            Init() { return true; }

            virtual void            AcquireResources() {}
            virtual void            DropResources() {}

            virtual int             FullSerialize(EntityWorld* world, OutputStream* output, int flags) { return -1; }
            virtual int             Serialize(EntityWorld* world, OutputStream* output, int flags) { return -1; }

            virtual void            Draw(const UUID_t* modeOrNull) {}
            //virtual int             OnEvent(int h, const Event_t* ev) { return h; }
            virtual void            OnFrame(double delta) {}
            virtual void            OnTick() {}

            virtual int             GetID() const = 0;
            virtual void            SetID(int entID) = 0;
            virtual const char*     GetName() = 0;
            virtual const Float3&   GetPos() = 0;
            virtual void            SetPos(const Float3& pos) = 0;
    };

    class IPointEntity
    {
        public:
            virtual ~IPointEntity() {}

            virtual IEntity*        GetEntity() = 0;

            virtual bool            GetAABB(Float3& min, Float3& max) = 0;
            virtual bool            GetAABBForPos(const Float3& newPos, Float3& min, Float3& max) = 0;

            virtual void            SetCollisionHandler(shared_ptr<ICollisionHandler> handler) = 0;
            virtual const Float3&   GetOrientation() = 0;
            virtual void            SetOrientation(const Float3& orientation) = 0;
            virtual const Float3&   GetSpeed() = 0;
            virtual void            SetSpeed(const Float3& speed) = 0;
    };
    
    class ICollisionHandler
    {
        public:
            virtual ~ICollisionHandler() {}

            // return: Can move to (possibly adjusted) newPos?
            virtual bool            CollideMovementTo(IPointEntity* pe, const Float3& pos, Float3& newPos) = 0;
    };

    class IEntityFilter
    {
        protected:
            ~IEntityFilter() {}

        public:
            virtual bool            OnAddEntity(EntityWorld* world, IEntity* ent) = 0;
            virtual void            OnRemoveEntity(EntityWorld* world, IEntity* ent) = 0;
    };

    class IEntityReflection
    {
        protected:
            ~IEntityReflection() {}

        public:
            virtual IEntity*        GetEntity() = 0;

            virtual size_t          GetNumProperties() = 0;
            virtual bool            GetRWProperties(NamedValuePtr* buffer, size_t count) = 0;
    };

    template <class C>
    IEntity* CreateInstance()
    {
        return new C();
    }
}

namespace zfw
{
    template <class Interface>
    class EntityCollectorByInterface: public IEntityFilter
    {
        protected:
            li::List<Interface*> entities;

        public:
            auto GetList() -> decltype(entities)& { return entities; }
            virtual bool OnAddEntity(EntityWorld* world, IEntity* ent) override;
            virtual void OnRemoveEntity(EntityWorld* world, IEntity* ent) override;
    };

    template <class Interface>
    bool EntityCollectorByInterface<Interface>::OnAddEntity(EntityWorld* world, IEntity* ent)
    {
        Interface* iface = dynamic_cast<Interface*>(ent);

        if (iface != nullptr)
            entities.add(iface);

        return true;
    }

    template <class Interface>
    void EntityCollectorByInterface<Interface>::OnRemoveEntity(EntityWorld* world, IEntity* ent)
    {
        Interface* iface = dynamic_cast<Interface*>(ent);

        if (iface != nullptr)
            entities.removeItem(iface);
    }
}
