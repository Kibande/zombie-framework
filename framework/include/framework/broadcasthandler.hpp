#ifndef framework_broadcasthandler_hpp
#define framework_broadcasthandler_hpp

#include <framework/base.hpp>

namespace zfw
{
    enum class AspectEvent {
        created,
        destroyed,
    };

    class IBroadcastSubscriber {
    public:
        virtual void OnAspectEvent(intptr_t entityId, IAspectType& type, void* data, AspectEvent event) {}
        virtual void OnMessageBroadcast(intptr_t type, const void* payload) {}
    };

    class IBroadcastHandler {
    public:
        virtual ~IBroadcastHandler() = default;

        virtual void BroadcastAspectEvent(intptr_t entityId, IAspectType& type, void* data, AspectEvent event) = 0;
        virtual void BroadcastMessage(intptr_t type, const void* payload) = 0;

        virtual void SubscribeToAspectType(IBroadcastSubscriber* sub, IAspectType& type) = 0;
        virtual void SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) = 0;

        template <typename MessageStruct>
        void BroadcastMessage(const MessageStruct& message) {
            this->BroadcastMessage(MessageStruct::msgType, &message);
        }

        template <typename ComponentDataStruct>
        void SubscribeToAspectType(IBroadcastSubscriber* sub) {
            this->SubscribeToAspectType(sub, ComponentDataStruct::GetType());
        }

        template <typename MessageStruct>
        void SubscribeToMessageType(IBroadcastSubscriber* sub) {
            this->SubscribeToMessageType(sub, MessageStruct::msgType);
        }

        virtual void UnsubscribeAll(IBroadcastSubscriber* sub) = 0;
    };
}

#endif
