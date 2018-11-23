#ifndef framework_entityworld2_hpp
#define framework_entityworld2_hpp

#include <framework/base.hpp>

namespace zfw
{
    class IEntityWorld2
    {
        public:
            virtual ~IEntityWorld2() = default;

            virtual intptr_t CreateEntity() = 0;
//            intptr_t CreateEntityFromBlueprint(blueprint);

            virtual void DestroyEntity(intptr_t id) = 0;

            /**
             * Get a cache-able pointer to a specific aspect of an entity.
             * @param id
             * @param type
             * @return pointer to aspect data, or nullptr if the aspect has not been set for this entity. Guaranteed
             *         to remain valid as long as the entity exists.
             */
            virtual void* GetEntityAspect(intptr_t id, IAspectType& type) = 0;

            /**
             *
             * @param id
             * @param type
             * @param data aspect data; this will be copied into IEntityWorld2's private storage
             * @return pointer to IEntityWorld2-owned copy of aspect data. Guaranteed to remain valid
             *         as long as the entity exists.
             */
            virtual void* SetEntityAspect(intptr_t id, IAspectType& type, const void* data) = 0;

            template <typename AspectStruct>
            AspectStruct* GetEntityAspect(intptr_t id) {
                return static_cast<AspectStruct*>(this->GetEntityAspect(id, AspectStruct::GetType()));
            }

            template <typename AspectStruct>
            void SetEntityAspect(intptr_t id, const AspectStruct& data) {
                this->SetEntityAspect(id, AspectStruct::GetType(), &data);
            }
    };
}

#endif
