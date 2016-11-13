#pragma once

#include <framework/framework.hpp>

namespace zfw
{
    class ISessionListener
    {
        public:
            virtual MessageQueue* GetMessageQueue() = 0;

            virtual int OnIncomingConnection(const char* remoteHostname, const char* playerName, const char* playerPassword) = 0;
    };

    class Session
    {
        public:
            static Session* Create(ISessionListener* listener);
            virtual ~Session() {}

            virtual void Host(int port) = 0;
            virtual void ConnectTo(const char *hostname, int port) = 0;

            virtual void SetDiscovery(bool enabled, int port, int interval) = 0;
            //virtual const char* GetHostname() = 0;

            virtual void SendMessageTo(ArrayIOStream& message, int clid) = 0;
    };

    struct SessionServerInfo
    {
        String serverName;
        String hostname;
        uint16_t port;
        bool isLan, isMe;
    };

    // Messages for zfw::Session ($2100..21FF)
    enum {
        // In
        SESSION_DUMMY_MSG = 0x2100,

        // Out
        SESSION_CONNECT_TO_RES,
        SESSION_HOST_RES,
        SESSION_SET_DISCOVERY_RES,

        SESSION_CLIENT_ACCEPTED,
        SESSION_SERVER_DISCOVERED,

        // Private
        SESSION_ADD_CLIENT,
        SESSION_CONNECT_TO,
        SESSION_HOST,
        SESSION_SET_DISCOVERY,
        SESSION_QUIT_THREAD,

        SESSION_MAX_MSG
    };

    struct SessionRes
    {
        bool success;
    };

    DECL_MESSAGE(SessionClientAccepted, SESSION_CLIENT_ACCEPTED)
    {
        ClientId clid;
    };

    DECL_MESSAGE(SessionServerDiscovered, SESSION_SERVER_DISCOVERED)
    {
        SessionServerInfo serverInfo;
    };
}
