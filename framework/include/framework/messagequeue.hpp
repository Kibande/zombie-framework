#pragma once

#include <framework/base.hpp>

#include <littl/Stream.hpp>

/*
    A few more or less important notes about MessageQueue:
        - At any given time, NO MORE THAN ONE THREAD should be attempting to call Retrieve.
        - Internally, MessageQueue uses 2 buffers. Using more should be possible (in theory),
            but there doesn't seem to be any advantage in doing that.
        - MessageQueueImpl uses a single shared mutex to lock cycle+realloc operations.
            This shouldn't be a performance problem as it is not expected for the buffers
            to be realloc'd very often.
            Anyway, there seems to be no easy way of doing this "properly". The problem is
            to make sure all DoAllocMessages started before a certain TryRetrieve have ended
            before that TryRetrieve invokation returns a pointer pointing inside what used to
            be a write buffer.

    To declare your own message struct:

        enum {
            MY_MESSAGE,
            MY_SIMPLE_MESSAGE
        };

        DECL_MESSAGE(MyMessage, MY_MESSAGE)
        {
            int x, y;
            String text;
        };

    To post a message:

        MessageConstruction<MyMessage> msg(msgQueue);
        msg->x = 0;
        msg->y = 1;
        msg->text = "Hello, World!";

    To post a message without body:

        msgQueue->Post(MY_SIMPLE_MESSAGE);

    To call a function from MessageQueue recipient thread:

        static void DoSomething();
            ...
        msgQueue->PostCall(DoSomething);
*/

// Uncomment to enable checking against memory corruption
#ifdef ZOMBIE_DEBUG
#define MESSAGEQUEUE_USE_FENCES
#endif

#define MESSAGE_CONSTRUCTION

namespace zfw
{
    typedef void (*MessageQueueCallable)();

    struct MessageHeader
    {
        size_t      length;     // length of the message body not including this MessageHeader
        int         type;       // message type as specified in AllocMessage/MessageConstruction
        int         flags;      // private, do not touch
        //ClientId    sender, recipient;

        void (*releaseFunc)(void *messageBody);

        // Utility functions
        template <typename Struct> Struct* Data() volatile
        {
#ifndef MESSAGEQUEUE_USE_FENCES
            return (Struct*)(this + 1);
#else
            return (Struct*)((uint8_t*)this + sizeof(MessageHeader) + sizeof(uint32_t));
#endif
        }

        void Release()
        {
#ifndef MESSAGEQUEUE_USE_FENCES
            if (releaseFunc != nullptr)
                releaseFunc(this + 1);
#else
            if (releaseFunc != nullptr)
                releaseFunc((uint8_t*)this + sizeof(MessageHeader) + sizeof(uint32_t));
#endif
        }
    };

    class MessageQueue
    {
        public:
            static MessageQueue* Create();
            virtual ~MessageQueue() {}

            virtual void Post(int messageType) = 0;
            virtual void PostCall(MessageQueueCallable callback) = 0;
            virtual MessageHeader* Retrieve(li::Timeout timeout) = 0;

            // These shouldn't be used directly, unless you know for sure what you're doing
            virtual void* AllocMessage(size_t length, int type, void (*releaseFunc)(void *messageBody)) = 0;
            virtual void FinishMessage(void* body) = 0;
    };

    template <typename Struct, int type = Struct::msgType> class MessageConstruction
    {
        MessageQueue* msgQueue;
        void* messageBody;

        private:
            static void ReleaseFunc(void* messageBody) { li::destructPointer<>((Struct*) messageBody); }

        public:
            MessageConstruction(MessageQueue* msgQueue) : msgQueue(msgQueue)
            {
                messageBody = msgQueue->AllocMessage(sizeof(Struct), type, ReleaseFunc);
                li::constructPointer<>((Struct*) messageBody);
            }

            ~MessageConstruction() { msgQueue->FinishMessage(messageBody); }

            Struct* operator -> () { return (Struct*) messageBody; }
    };
}
