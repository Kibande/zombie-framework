#ifndef framework_broadcasthandler_hpp
#define framework_broadcasthandler_hpp

#include <framework/base.hpp>

namespace zfw
{
    class IBroadcastSubscriber {
    public:
        virtual void OnMessageBroadcast(intptr_t type, const void* payload) {}
    };

    class IBroadcastHandler {
    public:
        virtual ~IBroadcastHandler() = default;

        virtual void BroadcastMessage(intptr_t type, const void* payload) = 0;

        virtual void SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) = 0;

        template <typename MessageStruct>
        void SubscribeToMessageType(IBroadcastSubscriber* sub) {
            this->SubscribeToMessageType(sub, MessageStruct::msgType);
        }

        virtual void UnsubscribeAll(IBroadcastSubscriber* sub) = 0;
    };
}

#endif
