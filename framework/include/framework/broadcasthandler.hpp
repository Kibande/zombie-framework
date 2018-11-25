#ifndef framework_broadcasthandler_hpp
#define framework_broadcasthandler_hpp

#include <framework/base.hpp>

namespace zfw
{
    enum class ComponentEvent {
        created,
        destroyed,
    };

    class IBroadcastSubscriber {
    public:
        virtual void OnComponentEvent(intptr_t entityId, IComponentType &type, void *data, ComponentEvent event) {}
        virtual void OnMessageBroadcast(intptr_t type, const void* payload) {}
    };

    class IBroadcastHandler {
    public:
        virtual ~IBroadcastHandler() = default;

        virtual void BroadcastComponentEvent(intptr_t entityId, IComponentType &type, void *data, ComponentEvent event) = 0;
        virtual void BroadcastMessage(intptr_t type, const void* payload) = 0;

        virtual void SubscribeToComponentType(IBroadcastSubscriber *sub, IComponentType &type) = 0;
        virtual void SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) = 0;

        template <typename MessageStruct>
        void BroadcastMessage(const MessageStruct& message) {
            this->BroadcastMessage(MessageStruct::msgType, &message);
        }

        template <typename ComponentDataStruct>
        void SubscribeToComponentType(IBroadcastSubscriber *sub) {
            this->SubscribeToComponentType(sub, GetComponentType<ComponentDataStruct>());
        }

        template <typename MessageStruct>
        void SubscribeToMessageType(IBroadcastSubscriber* sub) {
            this->SubscribeToMessageType(sub, MessageStruct::msgType);
        }

        virtual void UnsubscribeAll(IBroadcastSubscriber* sub) = 0;
    };
}

#endif
