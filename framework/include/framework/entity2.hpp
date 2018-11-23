#ifndef framework_entity2_hpp
#define framework_entity2_hpp

#include <framework/base.hpp>

namespace zfw
{
    constexpr static intptr_t kInvalidEntity = -1;

    /**
     * Some of the concerns involved in the development of the new entity-component system:
     *  - a component will want to subscribe to certain classes of data (aspects) -- but it never *owns* an aspect type
     *  - entities, components and aspects must be possible to create from native and script code and any combination thereof
     *  - aspects must be pure data (but may be nontrivial types such as strings)
     *  - serialization of maps and game saves, binary or text-based
     *  - common entity-component protocol for independent implementations
     *    - some aspect can be standardized by the framework
     *  - networking
     *  - enable cache-friendly iteration
     *
     * An entity is precisely defined as:
     *  - a collection of aspects that's identified by a numeric ID
     *
     * A component can receive relevant notifications of new added entities by means of the
     * SubscribeToAspectType(IAspectType*) method.
     *
     * As for storage of the component data, the entity might elect between:
     *  - inline in instance (currently not implemented)
     *  - in a dedicated pool within EntityWorld2. In that case no instance of IEntity2 even needs to exist.
     */
//    class IEntity2
//    {
//        public:
//            virtual ~IEntity2() {}
//
//            virtual void Visit(IAspectVisitor& visitor) {}
//    };
//
//    class IAspectVisitor
//    {
//        public:
//            virtual void Visit(IAspectType* type, void* data) = 0;
//
//            template <typename AspectStruct>
//            void operator () (AspectStruct& data) {
//                this->Visit(AspectStruct::GetType(), &data);
//            }
//    };
}

#endif
