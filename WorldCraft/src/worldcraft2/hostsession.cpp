
#include "session.hpp"

#include <littl/TcpSocket.hpp>

namespace worldcraftpro
{
    using namespace li;

    class Client
    {
        unique_ptr<TcpSocket> socket;

        public:
            Client(unique_ptr<TcpSocket>&& socket) : socket(move(socket))
            {
            }

            bool receive(ArrayIOStream& msgbuf, Timeout timeout = Timeout(0))
            {
                return socket->receive(msgbuf, timeout);
            }

            bool send(const ArrayIOStream& msgbuf)
            {
                return socket->send(msgbuf);
            }
    };

    class HostSession : public IEditorSession
    {
        ISessionMessageListener* listener;

        unique_ptr<TcpSocket> socket;

        List<Client*> clients;

        ArrayIOStream msgbuf;

        public:
            HostSession(unique_ptr<TcpSocket>&& socket);
            virtual ~HostSession();

            virtual const char* GetHostname() override;
            virtual void SetMessageListener(ISessionMessageListener* listener) {this->listener = listener;}

            virtual void OnFrame(double delta) override;

            virtual ArrayIOStream& BeginMessage(uint32_t id) override;
            virtual void Broadcast(const ArrayIOStream& msgbuf) override;
    };

    HostSession::HostSession(unique_ptr<TcpSocket>&& socket)
        : listener(nullptr), socket(std::move(socket))
    {
    }

    HostSession::~HostSession()
    {
        iterate2 (i, clients)
            delete i;
    }

    ArrayIOStream& HostSession::BeginMessage(uint32_t id)
    {
        msgbuf.clear(true);
        msgbuf.writeLE<uint32_t>(333);
        msgbuf.writeLE<uint32_t>(id);
        return msgbuf;
    }

    void HostSession::Broadcast(const ArrayIOStream& msgbuf)
    {
        if (socket != nullptr && socket->getState() == TcpSocket::connected)
            socket->send(msgbuf);

        iterate2 (i, clients)
            i->send(msgbuf);
    }

    const char* HostSession::GetHostname()
    {
        if (socket != nullptr)
        {
            if (socket->getState() == TcpSocket::listening)
                return "$self";
            else if (socket->getState() == TcpSocket::connected)
                return socket->getPeerIP();
        }

        return "?";
    }

    void HostSession::OnFrame(double delta)
    {
        if (socket != nullptr)
        {
            if (socket->getState() == TcpSocket::listening)
            {
                unique_ptr<TcpSocket> client = socket->accept(false);

                if (client != nullptr)
                {
                    printf("Incoming connection from %s\n", client->getPeerIP());
                    clients.add(new Client(std::move(client)));
                }
            }
            else if (socket->getState() == TcpSocket::connected)
            {
                if (socket->receive(msgbuf))
                {
                    uint32_t sender, id;
                    if (msgbuf.readLE(&sender) && msgbuf.readLE(&id))
                        listener->OnSessionMessage(sender, id, msgbuf);
                }
            }
        }

        iterate2 (i, clients)
            if (i->receive(msgbuf))
            {
                uint32_t sender, id;
                if (msgbuf.readLE(&sender) && msgbuf.readLE(&id))
                    listener->OnSessionMessage(sender, id, msgbuf);
            }
    }

    IEditorSession* IEditorSession::CreateClientSession(const char* host, int port)
    {
        unique_ptr<TcpSocket> socket = TcpSocket::create();

        if (!socket->connect(host, port, true))
            throw (String) "Failed to connect:\n" + socket->getErrorDesc();

        return new HostSession(std::move(socket));
    }

    IEditorSession* IEditorSession::CreateHostSession(int port)
    {
        unique_ptr<TcpSocket> socket = TcpSocket::create();

        if (!socket->listen(port))
            throw (String) "Failed to create server:\n" + socket->getErrorDesc();

        return new HostSession(std::move(socket));
    }
}
