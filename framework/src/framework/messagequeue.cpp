
#include <framework/messagequeue.hpp>
#include <framework/utility/essentials.hpp>

#include <littl/Thread.hpp>

#define NUM_BUFFERS             2
#define WRITEBUF_MIN_ALLOC      64

// Uncomment to enable debug messages in this unit
//#define DEBUG_MESSAGEQUEUE

#ifdef DEBUG_MESSAGEQUEUE
#define Debug_printf(text) Sys::printf text
#else
#define Debug_printf(text)
#endif

#if defined(MESSAGEQUEUE_USE_FENCES) && !defined(ZOMBIE_DEBUG)
#pragma message ("WARNING: MESSAGEQUEUE_USE_FENCES enabled in RELEASE mode")
#endif

namespace zfw
{
    using namespace li;

    // MessageHeader flags
    enum
    {
        MESSAGE_UNDER_CONSTRUCTION = 1,
        MESSAGE_PAYLOAD_IS_CALLBACK = 2
    };

#ifdef MESSAGEQUEUE_USE_FENCES
    enum
    {
        MAGIC_PRE_FENCE =   0x12773455,
        MAGIC_FENCE_A =     0xABBAFEEF,
        MAGIC_FENCE_B0 =    0xB00BD00D,
        MAGIC_FENCE_B1 =    0x37957953
    };

    inline uint32_t& pre_fence(MessageHeader* header)
    {
        return ((uint32_t*)((uint8_t*)header - sizeof(uint32_t)))[0];
    }

    inline uint32_t& fence_a(MessageHeader* header)
    {
        return ((uint32_t*)((uint8_t*)header + sizeof(MessageHeader)))[0];
    }

    inline uint32_t& fence_b0(MessageHeader* header)
    {
        return ((uint32_t*)((uint8_t*)header + sizeof(MessageHeader) + sizeof(uint32_t) + header->length))[0];
    }

    inline uint32_t& fence_b1(MessageHeader* header)
    {
        return ((uint32_t*)((uint8_t*)header + sizeof(MessageHeader) + sizeof(uint32_t) + header->length))[1];
    }
#endif

    class MessageQueueImpl: public MessageQueue
    {
        li::Mutex c_mutex;
        li::ConditionVar conditionVar;

        struct
        {
            uint8_t *data;
            size_t capacity, used, index;
        }
        buffers[NUM_BUFFERS];

        int readbuf, writebuf;
        volatile int32_t incompleteMessages;

        MessageHeader* DoAllocMessage(size_t body_length);
        MessageHeader* TryRetrieve();

        public:
            MessageQueueImpl();
            ~MessageQueueImpl();

            virtual void* AllocMessage(size_t length, int type, void (*releaseFunc)(void *messageBody)) override;
            virtual void FinishMessage(void* body) override;

            virtual void Post(int messageType) override;
            virtual void PostCall(MessageQueueCallable callback) override;
            virtual MessageHeader* Retrieve(li::Timeout timeout) override;

            void InlineFinishMessage(volatile MessageHeader* header)
            {
                header->flags &= ~MESSAGE_UNDER_CONSTRUCTION;

                interlockedDecrement(&incompleteMessages);
                conditionVar.set();
                //c_mutex.leave();
            }
    };

    MessageQueueImpl::MessageQueueImpl()
    {
        for (size_t i = 0; i < NUM_BUFFERS; i++)
        {
            buffers[i].data = nullptr;
            buffers[i].capacity = 0;
            buffers[i].used = 0;
            buffers[i].index = 0;
        }

        readbuf = -1;
        writebuf = 0;

        incompleteMessages = 0;
    }

    MessageQueueImpl::~MessageQueueImpl()
    {
        li::CriticalSection cs(c_mutex);

        MessageHeader* header;

        while ((header = TryRetrieve()) != nullptr)
            header->Release();

        for (size_t i = 0; i < NUM_BUFFERS; i++)
            free(buffers[i].data);
    }

    void* MessageQueueImpl::AllocMessage(size_t length, int type, void (*releaseFunc)(void *messageBody))
    {
        MessageHeader* header = DoAllocMessage(length);

        header->type = type;
        header->releaseFunc = releaseFunc;

#ifndef MESSAGEQUEUE_USE_FENCES
        return (void*) (header + 1);
#else
        return (void*) ((uint8_t*)header + sizeof(MessageHeader) + sizeof(uint32_t));
#endif
    }

    MessageHeader* MessageQueueImpl::DoAllocMessage(size_t body_length)
    {
        li::CriticalSection cs(c_mutex);
        //c_mutex.enter();

        Debug_printf(("[MessageQueue] using writebuf %i\n", writebuf));

        // Make sure the underlying buffer is large enough
        size_t new_used = buffers[writebuf].used + sizeof(MessageHeader) + body_length;
        
#ifdef MESSAGEQUEUE_USE_FENCES
        new_used += sizeof(uint32_t) * 4;
#endif

        if (new_used > buffers[writebuf].capacity)
        {
            while (incompleteMessages > 0)
                ;

            buffers[writebuf].capacity = std::max<size_t>(buffers[writebuf].capacity * 2, new_used);
            buffers[writebuf].capacity = std::max<size_t>(buffers[writebuf].capacity, WRITEBUF_MIN_ALLOC);
            buffers[writebuf].data = (uint8_t *) realloc(buffers[writebuf].data, buffers[writebuf].capacity);

            ZFW_ASSERT(buffers[writebuf].data != nullptr)

            Debug_printf(("[MessageQueue] growing writebuf to %u bytes\n", buffers[writebuf].capacity));
        }

        interlockedIncrement(&incompleteMessages);

#ifdef MESSAGEQUEUE_USE_FENCES
        buffers[writebuf].used += sizeof(uint32_t);
#endif

        // Mark message as 'under construction'
        MessageHeader* header = (MessageHeader*) (buffers[writebuf].data + buffers[writebuf].used);
        header->length = body_length;
        header->flags = MESSAGE_UNDER_CONSTRUCTION;
        buffers[writebuf].used = new_used;

#ifdef MESSAGEQUEUE_USE_FENCES
        pre_fence(header) = MAGIC_PRE_FENCE;
        fence_a(header) =   MAGIC_FENCE_A;
        fence_b0(header) =  MAGIC_FENCE_B0;
        fence_b1(header) =  MAGIC_FENCE_B1;
#endif

        Debug_printf(("[MessageQueue] 'used' advanced to %u\n", (unsigned int) buffers[writebuf].used));

        // c_mutex will be lifted now.
        // Retrieve will refuse to return the message until 'MESSAGE_UNDER_CONSTRUCTION' is cleared. 
        return header;
    }

    void MessageQueueImpl::FinishMessage(void* body)
    {
        // Calculate original MessageHeader address
#ifndef MESSAGEQUEUE_USE_FENCES
        volatile MessageHeader* header = ((MessageHeader*) body) - 1;   
#else
        MessageHeader* header = (MessageHeader*)((uint8_t*)body - sizeof(MessageHeader) - sizeof(uint32_t));

        ZFW_ASSERT(pre_fence(header) == MAGIC_PRE_FENCE);
        ZFW_ASSERT(fence_a(header) == MAGIC_FENCE_A);
        ZFW_ASSERT(fence_b0(header) == MAGIC_FENCE_B0);
        ZFW_ASSERT(fence_b1(header) == MAGIC_FENCE_B1);
#endif

        InlineFinishMessage(header);
    }

    void MessageQueueImpl::Post(int messageType)
    {
        volatile MessageHeader* header = DoAllocMessage(0);

        header->type = messageType;
        header->releaseFunc = nullptr;

        InlineFinishMessage(header);
    }

    void MessageQueueImpl::PostCall(MessageQueueCallable callback)
    {
        volatile MessageHeader* header = DoAllocMessage(sizeof(MessageQueueCallable));

        header->type = -1;
        header->releaseFunc = nullptr;

        //*reinterpret_cast<volatile MessageQueueCallable*>(header + 1) = callback;
        *(header->Data<volatile MessageQueueCallable>()) = callback;

        header->flags |= MESSAGE_PAYLOAD_IS_CALLBACK;
        InlineFinishMessage(header);
    }

    MessageHeader* MessageQueueImpl::Retrieve(li::Timeout timeout)
    {
        for ( ; ; )
        {
            MessageHeader* header = TryRetrieve();

            if (header != nullptr)
            {
                if (header->flags & MESSAGE_PAYLOAD_IS_CALLBACK)
                {
                    // Whoa, uh...
                    (*(header->Data<MessageQueueCallable>()))();
                    continue;
                }

                return header;
            }

            if (timeout.infinite)
            {
                ZFW_ASSERT(conditionVar.waitFor())
            }
            else if (timeout.timedOut())
                break;
        }

        return nullptr;
    }

    MessageHeader* MessageQueueImpl::TryRetrieve()
    {
        // Make sure nobody tries to realloc() our readbuf!
        c_mutex.enter();

        if (readbuf == -1 || buffers[readbuf].index == buffers[readbuf].used)
        {
            // Rotate buffers
            writebuf =  (writebuf + 1) % NUM_BUFFERS;
            readbuf =   (writebuf + 1) % NUM_BUFFERS;

            Debug_printf(("[MessageQueue] rotating buffers -> r/w [%i %i]\n", readbuf, writebuf));

            buffers[readbuf].index = 0;
            buffers[writebuf].used = 0;
        }
        else
            Debug_printf(("[MessageQueue] using readbuf %i\n", readbuf));

        // At this point, we can lift the mutex.
        // Any subsequent calls to AllocMessage will use a buffer other than current readbuf
        // - that is, at least until the next call to Retrieve
        c_mutex.leave();

        // No messages (headers even) to retrieve
        if (buffers[readbuf].index == buffers[readbuf].used)
        {
            Debug_printf(("[MessageQueue] other buffer (%i) empty as well\n", readbuf));
            return nullptr;
        }

#ifndef MESSAGEQUEUE_USE_FENCES
        ZFW_DBGASSERT(buffers[readbuf].index + sizeof(MessageHeader) <= buffers[readbuf].used)

        MessageHeader* header = (MessageHeader*) (buffers[readbuf].data + buffers[readbuf].index);
#else
        ZFW_ASSERT(buffers[readbuf].index + sizeof(MessageHeader) + 4 * sizeof(uint32_t) <= buffers[readbuf].used)

        MessageHeader* header = (MessageHeader*) (buffers[readbuf].data + buffers[readbuf].index + sizeof(uint32_t));

        ZFW_ASSERT(pre_fence(header) == MAGIC_PRE_FENCE);
        ZFW_ASSERT(fence_a(header) == MAGIC_FENCE_A);
        ZFW_ASSERT(fence_b0(header) == MAGIC_FENCE_B0);
        ZFW_ASSERT(fence_b1(header) == MAGIC_FENCE_B1);
#endif

        // If this message isn't finished yet, we have to wait for it
        if (header->flags & MESSAGE_UNDER_CONSTRUCTION)
        {
            Debug_printf(("[MessageQueue] message not ready!\n", readbuf, writebuf));
            return nullptr;
        }

        // Otherwise adjust the readbuf index and return a pointer to the header
        buffers[readbuf].index += sizeof(MessageHeader) + header->length;

#ifdef MESSAGEQUEUE_USE_FENCES
        buffers[readbuf].index += 4 * sizeof(uint32_t);
#endif

        Debug_printf(("[MessageQueue] 'index' advanced to %u\n", (unsigned) buffers[readbuf].index));
        Debug_printf(("[MessageQueue] returning %u-byte message \"%i\"\n", (unsigned) header->length, header->type));

        return header;
    }

    MessageQueue* MessageQueue::Create()
    {
        return new MessageQueueImpl();
    }
}
