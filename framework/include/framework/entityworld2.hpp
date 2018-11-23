#ifndef framework_entityworld2_hpp
#define framework_entityworld2_hpp

#include <framework/base.hpp>

#include <functional>

namespace zfw
{
    class IEntityWorld2
    {
        public:
            static unique_ptr<IEntityWorld2> Create(IBroadcastHandler* broadcastHandler);
            virtual ~IEntityWorld2() = default;

            virtual intptr_t CreateEntity() = 0;
//            intptr_t CreateEntityFromBlueprint(blueprint);

            virtual void DestroyEntity(intptr_t id) = 0;

            /**
             * Get a cache-able pointer to a specific entity component.
             * @param id a valid entity ID or kInvalidEntity
             * @param type
             * @return pointer to component data, or nullptr if the component has not been set for this entity.
             *         Guaranteed to remain valid as long as the entity exists.
             */
            virtual void* GetEntityComponent(intptr_t id, IComponentType &type) = 0;

            /**
             *
             * @param id
             * @param type
             * @param data component data; this will be copied into IEntityWorld2's private storage
             * @return pointer to IEntityWorld2-owned copy of component data. Guaranteed to remain valid
             *         as long as the entity exists.
             */
            virtual void* SetEntityComponent(intptr_t id, IComponentType &type, const void *data) = 0;

            virtual void IterateEntitiesByComponent(IComponentType &type, std::function<void(intptr_t entityId, void* component_data)> callback) = 0;

            template <typename ComponentStruct>
            ComponentStruct* GetEntityComponent(intptr_t id) {
                return static_cast<ComponentStruct*>(this->GetEntityComponent(id, ComponentStruct::GetType()));
            }

            template <typename ComponentStruct>
            void SetEntityComponent(intptr_t id, const ComponentStruct &data) {
                this->SetEntityComponent(id, ComponentStruct::GetType(), &data);
            }
    };
}

#endif
