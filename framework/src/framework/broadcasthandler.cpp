
#include <framework/broadcasthandler.hpp>

#include <unordered_map>

namespace zfw
{
    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    struct BroadcastSubscriberChain {
        IBroadcastSubscriber* sub;
        BroadcastSubscriberChain* next;
    };

    class BroadcastHandler : public IBroadcastHandler {
    public:
        void BroadcastMessage(intptr_t type, const void* payload) override;

        void SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) override;

        void UnsubscribeAll(IBroadcastSubscriber* sub) override;

    private:
        std::unordered_map<intptr_t, BroadcastSubscriberChain*> byMessageType;
    };

    // ====================================================================== //
    //  class BroadcastHandler
    // ====================================================================== //

    unique_ptr<IBroadcastHandler> p_CreateBroadcastHandler() {
        return std::make_unique<BroadcastHandler>();
    }

    void BroadcastHandler::BroadcastMessage(intptr_t type, const void* payload) {
        auto it = byMessageType.find(type);

        if (it == byMessageType.end()) {
            return;
        }

        for (auto chain = it->second; chain; chain = chain->next) {
            chain->sub->OnMessageBroadcast(type, payload);
        }
    }

    void BroadcastHandler::SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) {
        auto it = byMessageType.find(type);

        auto chain = new BroadcastSubscriberChain;
        chain->sub = sub;

        if (it == byMessageType.end()) {
            chain->next = nullptr;
            byMessageType.emplace(type, chain);
        }
        else {
            chain->next = it->second;
            byMessageType[type] = chain;
        }
    }

    void BroadcastHandler::UnsubscribeAll(IBroadcastSubscriber* sub) {
        for (auto it = byMessageType.begin(); it != byMessageType.end(); it++) {
            BroadcastSubscriberChain* prev = nullptr;

            for (auto chain = it->second; chain; ) {
                if (chain->sub == sub) {
                    if (prev) {
                        prev->next = chain->next;
                    }
                    else {
                        byMessageType.erase(it);
                    }

                    auto* chain_to_delete = chain;
                    chain = chain->next;
                    delete chain_to_delete;
                }
                else {
                    prev = chain;
                    chain = chain->next;
                }
            }
        }
    }
}
