#include <framework/aspecttype.hpp>
#include <framework/entityworld2.hpp>
#include <framework/utility/util.hpp>

#include <unordered_map>
#include <framework/broadcasthandler.hpp>

namespace zfw
{
    using std::make_unique;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    // TODO: re-implement in a cache friendly way
    class PerInstanceDataPool {
    public:
        PerInstanceDataPool(IAspectType& type) : type(type) {}
        ~PerInstanceDataPool();

        void* Get(intptr_t entityId);
        void* GetOrAlloc(intptr_t entityId, bool* wasCreated_out);

    private:
        IAspectType& type;
        std::unordered_map<intptr_t, void*> perInstanceData;
    };

    class EntityWorld2 : public IEntityWorld2 {
    public:
        EntityWorld2(IBroadcastHandler* broadcast) : broadcast(broadcast) {}

        intptr_t CreateEntity() final;

        void DestroyEntity(intptr_t id) final;

        void* GetEntityAspect(intptr_t id, IAspectType& type) final;
        void SetEntityAspect(intptr_t id, IAspectType& type, const void* data) final;

    private:
        IBroadcastHandler* broadcast;

        intptr_t nextId;

        // TODO: re-do as vector by component id
        std::unordered_map<IAspectType*, unique_ptr<PerInstanceDataPool>> aspectPools;
    };

    // ====================================================================== //
    //  class EntityWorld2
    // ====================================================================== //

    unique_ptr<IEntityWorld2> p_CreateEntityWorld2(IBroadcastHandler* broadcast) {
        return std::make_unique<EntityWorld2>(broadcast);
    }

    intptr_t EntityWorld2::CreateEntity() {
        return this->nextId++;
    }

    void EntityWorld2::DestroyEntity(intptr_t id) {
        // FIXME!
    }

    void* EntityWorld2::GetEntityAspect(intptr_t id, IAspectType& type) {
        auto iter = aspectPools.find(&type);

        if (iter == aspectPools.end()) {
            return nullptr;
        }
        else {
            return iter->second->Get(id);
        }
    }

    void EntityWorld2::SetEntityAspect(intptr_t id, IAspectType& type, const void* data) {
        auto iter = aspectPools.find(&type);

        PerInstanceDataPool* pool;

        if (iter == aspectPools.end()) {
            auto pool_ = make_unique<PerInstanceDataPool>(type);
            pool = pool_.get();
            aspectPools[&type] = move(pool_);
        }
        else {
            pool = iter->second.get();
        }

        bool wasCreated;
        void* aspect_stored = pool->GetOrAlloc(id, &wasCreated);

        if (!wasCreated) {
            type.Destruct(aspect_stored);
        }

        type.ConstructFrom(aspect_stored, data);

        if (wasCreated) {
            broadcast->BroadcastAspectEvent(id, type, aspect_stored, AspectEvent::created);
        }
        else {
            //broadcast->BroadcastAspectEvent(id, type, aspect_stored, AspectEvent::updated);
        }
    }

    // ====================================================================== //
    //  class PerInstanceDataPool
    // ====================================================================== //

    PerInstanceDataPool::~PerInstanceDataPool() {
        for (auto& pair : perInstanceData) {
            type.Destruct(pair.second);
            free(pair.second);
        }
    }

    void* PerInstanceDataPool::Get(intptr_t entityId) {
        auto iter = perInstanceData.find(entityId);

        if (iter == perInstanceData.end()) {
            return nullptr;
        }
        else {
            return iter->second;
        }
    }

    void* PerInstanceDataPool::GetOrAlloc(intptr_t entityId, bool* wasCreated_out) {
        auto iter = perInstanceData.find(entityId);

        if (iter == perInstanceData.end()) {
            auto size = type.sizeof_;
            auto data = malloc(size);
            perInstanceData[entityId] = data;
            *wasCreated_out = true;
            return data;
        }
        else {
            *wasCreated_out = false;
            return iter->second;
        }
    }
}
