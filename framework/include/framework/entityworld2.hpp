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

            // Don't expect these to be particularly fast, as the lookup needs to go through 2 maps (aspect type, entity id)
            // TODO: make it possible to use component ID directly
            // TODO: make it possible to use aspect pool index directly
            virtual void* GetEntityAspect(intptr_t id, IAspectType& type) = 0;
            virtual void SetEntityAspect(intptr_t id, IAspectType& type, const void* data) = 0;

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
