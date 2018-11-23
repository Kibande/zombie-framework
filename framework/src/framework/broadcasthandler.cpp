
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
        void BroadcastAspectEvent(intptr_t entityId, IAspectType& type, void* data, AspectEvent event) final;
        void BroadcastMessage(intptr_t type, const void* payload) override;

        void SubscribeToAspectType(IBroadcastSubscriber* sub, IAspectType& type) final;
        void SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) override;

        void UnsubscribeAll(IBroadcastSubscriber* sub) override;

    private:
        template <typename Key, typename Map>
        void Subscribe(IBroadcastSubscriber* sub, Key&& key, Map& map);

        std::unordered_map<IAspectType*, BroadcastSubscriberChain*> byComponentDataType;
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

    void BroadcastHandler::BroadcastAspectEvent(intptr_t entityId, IAspectType& type, void* data, AspectEvent event) {
        auto it = byComponentDataType.find(&type);

        if (it == byComponentDataType.end()) {
            return;
        }

        for (auto chain = it->second; chain; chain = chain->next) {
            chain->sub->OnAspectEvent(entityId, type, data, event);
        }
    }

    template <typename Key, typename Map>
    void BroadcastHandler::Subscribe(IBroadcastSubscriber* sub, Key&& key, Map& map) {
        auto it = map.find(key);

        auto chain = new BroadcastSubscriberChain;
        chain->sub = sub;

        if (it == map.end()) {
            chain->next = nullptr;
            map.emplace(move(key), chain);
        }
        else {
            chain->next = it->second;
            map[key] = chain;
        }
    }

    void BroadcastHandler::SubscribeToAspectType(IBroadcastSubscriber* sub, IAspectType& type) {
        this->Subscribe(sub, &type, byComponentDataType);
    }

    void BroadcastHandler::SubscribeToMessageType(IBroadcastSubscriber* sub, intptr_t type) {
        this->Subscribe(sub, type, byMessageType);
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
