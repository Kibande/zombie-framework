#pragma once

#include "world.hpp"

namespace worldcraftpro
{
    class ISessionMessageListener
    {
        public:
            virtual void OnSessionMessage(int clientId, uint32_t id, ArrayIOStream& message) = 0;
    };

    class IEditorSession
    {
        public:
            virtual ~IEditorSession() {}

            static IEditorSession* CreateClientSession(const char* host, int port);
            static IEditorSession* CreateHostSession(int port);

            virtual const char* GetHostname() = 0;
            virtual void SetMessageListener(ISessionMessageListener* listener) = 0;

            virtual void OnFrame(double delta) = 0;

            virtual ArrayIOStream& BeginMessage(uint32_t id) = 0;
            virtual void Broadcast(const ArrayIOStream& message) = 0;
    };
}
