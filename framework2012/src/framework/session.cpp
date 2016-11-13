
#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/session.hpp>

#include <littl/TcpSocket.hpp>
#include <littl/Thread.hpp>
#include <littl/UdpSocket.hpp>

// Uncomment to enable debug messages in this unit
#define DEBUG_SESSION

#ifdef DEBUG_SESSION
#define Debug_printf(text) Sys::printf text
#else
#define Debug_printf(text)
#endif

namespace zfw
{
    enum {
        // C->S
        OP_QUERY_SERVER_INFO = 0x01,
        OP_JOIN_GAME = 0x10,

        // S->C

        // Errors
        OP_UNKNOWN_OPCODE = 0x800,
        OP_PLAYER_REFUSED
    };

    class ClientConnection;

    DECL_MESSAGE(SessionAddClient, SESSION_ADD_CLIENT)
    {
        ClientConnection* client;
    };

    DECL_MESSAGE(SessionConnectTo, SESSION_CONNECT_TO)
    {
        String hostname;
        uint16_t port;
    };

    DECL_MESSAGE(SessionHost, SESSION_HOST)
    {
        uint16_t port;
    };

    DECL_MESSAGE(SessionSetDiscovery, SESSION_SET_DISCOVERY)
    {
        bool enable;
        int interval;
        uint16_t port;
    };

    class ClientConnection;

    class SessionImpl: public Session, public li::Thread
    {
        // Main thread
        ArrayIOStream buffer;
        ISessionListener* listener;

        // Common
        MessageQueue* msgQueue;

        // Worker thread
        ArrayIOStream workerBuffer;
        Object<TcpSocket> socket;
        int listenPort;

        List<SessionServerInfo> knownServers;

        unsigned lastDiscovery;
        Object<UdpSocket> discoverySocket;
        SockAddress* broadcastAddr;
        int discoveryInterval;

        ClientId nextClientId;
        Array<ClientConnection*> clients;

        bool TryToAddKnownServer(const SessionServerInfo& ssi);

        public:
            SessionImpl(ISessionListener* listener);
            virtual ~SessionImpl();

            virtual void Host(int port) override;
            virtual void ConnectTo(const char *hostname, int port) override;

            virtual void SetDiscovery(bool enabled, int port, int interval) override;

            virtual void SendMessageTo(ArrayIOStream& message, int clid) override;

            virtual void run() override;

            void AddClient(ClientConnection* client);
            int OnIncomingConnection(const char* remoteHostname, const char* playerName, const char* playerPassword);
    };

    class ClientConnection: public li::Thread
    {
        friend class SessionImpl;

        SessionImpl* session;
        Object<TcpSocket> socket;

        ArrayIOStream inBuffer, outBuffer;

        String ip, playerName;

        public:
            ClientConnection(SessionImpl* session, TcpSocket* socket);

            virtual void run() override;

            void SetId(ClientId id) {}
    };

    SessionImpl::SessionImpl(ISessionListener* listener)
            : listener(listener)
    {
        msgQueue = MessageQueue::Create();

        this->start();
    }

    SessionImpl::~SessionImpl()
    {
        msgQueue->Post(SESSION_QUIT_THREAD);
        this->waitFor();

        delete msgQueue;
    }

    void SessionImpl::AddClient(ClientConnection* client)
    {
        MessageConstruction<SessionAddClient> msg(msgQueue);
        msg->client = client;
    }

    void SessionImpl::ConnectTo(const char *hostname, int port)
    {
        MessageConstruction<SessionConnectTo> msg(msgQueue);
        msg->hostname = hostname;
        msg->port = port;
    }

    void SessionImpl::Host(int port)
    {
        MessageConstruction<SessionHost> msg(msgQueue);
        msg->port = port;
    }

    int SessionImpl::OnIncomingConnection(const char* remoteHostname, const char* playerName, const char* playerPassword) 
    {
        return listener->OnIncomingConnection(remoteHostname, playerName, playerPassword);
    }

    void SessionImpl::run()
    {
        broadcastAddr = nullptr;
        discoveryInterval = 0;
        lastDiscovery = 0;

        while (!shouldEnd)
        {
            MessageHeader* msg;
            TcpSocket* incoming;

            if ((msg = msgQueue->Retrieve(Timeout(0))) != nullptr)
            {
                switch (msg->type)
                {
                    case SESSION_ADD_CLIENT:
                    {
                        auto payload = msg->Data<SessionAddClient>();

                        clients[nextClientId] = payload->client;
                        payload->client->SetId(nextClientId);
                        Debug_printf(("[Session] assigned clid %i\n", nextClientId));

                        MessageConstruction<SessionClientAccepted> msg(listener->GetMessageQueue());
                        msg->clid = nextClientId;

                        nextClientId++;
                        break;
                    }

                    case SESSION_HOST:
                    {
                        auto payload = msg->Data<SessionHost>();

                        socket = TcpSocket::create(true);
                        bool success = socket->listen(payload->port);

                        if (success)
                            listenPort = payload->port;
                        else
                            listenPort = -1;

                        nextClientId = 1;

                        Debug_printf(("[Session] listen on port %i -> success = %i\n", payload->port, (int) success));
                        MessageConstruction<SessionRes, SESSION_HOST_RES> msg(listener->GetMessageQueue());
                        msg->success = success;
                        break;
                    }

                    case SESSION_CONNECT_TO:
                    {
                        auto payload = msg->Data<SessionConnectTo>();

                        socket = TcpSocket::create(true);
                        bool success = socket->connect(payload->hostname, payload->port, true);

                        if (success)
                        {
                            ArrayIOStream outBuffer;
                            outBuffer.write<int16_t>(OP_JOIN_GAME);
                            outBuffer.writeString("$PlayerName$");
                            socket->send(outBuffer);
                        }

                        Debug_printf(("[Session] connect to '%s':%i -> success = %i\n", payload->hostname.c_str(), payload->port, (int) success));
                        MessageConstruction<SessionRes, SESSION_CONNECT_TO_RES> msg(listener->GetMessageQueue());
                        msg->success = success;
                        break;
                    }

                    case SESSION_SET_DISCOVERY:
                    {
                        auto payload = msg->Data<SessionSetDiscovery>();

                        if (payload->interval > 0)
                        {
                            discoverySocket = UdpSocket::create();
                            bool success = discoverySocket->bind(payload->port);

                            if (!success)
                                Sys::printf("[Session] discovery bind failed: %s\n", discoverySocket->getErrorDesc());
                            
                            broadcastAddr = discoverySocket->getBroadcastAddress(payload->port);
                            discoveryInterval = payload->interval;

                            Debug_printf(("[Session] set discovery on port %i each %i ms -> success = %i\n", payload->port, payload->interval, (int) success));
                            MessageConstruction<SessionRes, SESSION_SET_DISCOVERY_RES> msg(listener->GetMessageQueue());
                            msg->success = success;
                        }
                        else
                        {
                            discoveryInterval = 0;
                            discoverySocket.release();
                        }
                        break;
                    }

                    case SESSION_QUIT_THREAD:
                        end();
                        break;
                }

                msg->Release();
            }
            else if (socket != nullptr && (incoming = socket->accept(false)))
            {
                Debug_printf(("[Session] incoming TCP connection from '%s'\n", incoming->getPeerIP()));

                (new ClientConnection(this, incoming))->start();
            }
            else
            {
                if (discoverySocket != nullptr && listenPort >= 0 && relativeTime() > lastDiscovery + discoveryInterval)
                {
                    workerBuffer.clear(true);
                    workerBuffer.writeString("GameServer");
                    workerBuffer.write<uint16_t>(listenPort);
                    discoverySocket->send(broadcastAddr, workerBuffer);

                    lastDiscovery = relativeTime();

                    if (discoverySocket->receive(workerBuffer))
                    {
                        SessionServerInfo ssi;

                        ssi.serverName = workerBuffer.readString();
                        ssi.hostname = discoverySocket->getPeerIP();
                        ssi.port = workerBuffer.read<uint16_t>();

                        if (TryToAddKnownServer(ssi))
                        {
                            MessageConstruction<SessionServerDiscovered> msg(listener->GetMessageQueue());
                            msg->serverInfo = ssi;
                        }
                    }
                }

                li::pauseThread(glm::min(10, discoveryInterval));
            }
        }

        socket.release();

        if (broadcastAddr != nullptr)
            discoverySocket->releaseAddress(broadcastAddr);
        discoverySocket.release();
    }

    void SessionImpl::SendMessageTo(ArrayIOStream& message, int clid)
    {
        ZFW_ASSERT(clid > 0 && clid < 32)

        clients[clid]->socket->send(message);
    }

    void SessionImpl::SetDiscovery(bool enabled, int port, int interval)
    {
        MessageConstruction<SessionSetDiscovery> msg(msgQueue);

        msg->enable = enabled;
        msg->port = port;
        msg->interval = interval;
    }

    bool SessionImpl::TryToAddKnownServer(const SessionServerInfo& ssi)
    {
        iterate2 (i, knownServers)
        {
            auto& s = *i;

            if (s.hostname == ssi.hostname && s.port == ssi.port)
                return false;
        }

        knownServers.add(ssi);
        return true;
    }

    ClientConnection::ClientConnection(SessionImpl* session, TcpSocket* socket) : session(session), socket(socket)
    {
        destroyOnExit();
    }

    void ClientConnection::run()
    {
        if (!socket->receive(inBuffer, Timeout(10000)))
            return;

        int16_t opcode = inBuffer.read<int16_t>();

        if (opcode == OP_QUERY_SERVER_INFO)
        {
            ZFW_ASSERT(opcode != OP_QUERY_SERVER_INFO)
        }
        else if (opcode == OP_JOIN_GAME)
        {
            ip = socket->getPeerIP();
            playerName = inBuffer.readString();
            Debug_printf(("[Session] player '%s' wants to join the game!\n", playerName.c_str()));

            int letIn = session->OnIncomingConnection(ip, playerName, nullptr);

            if (letIn <= 0)
            {
                outBuffer.clear(true);
                outBuffer.write<int16_t>(OP_PLAYER_REFUSED);
                socket->send(outBuffer);
            }
            else
            {
                destroyOnExit(false);
                session->AddClient(this);
            }
        }
        else
        {
            outBuffer.clear(true);
            outBuffer.write<int16_t>(OP_UNKNOWN_OPCODE);
            socket->send(outBuffer);
        }
    }

    Session* Session::Create(ISessionListener* listener)
    {
        return new SessionImpl(listener);
    }
}