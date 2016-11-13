
#include <framework/framework.hpp>
#include <framework/messagequeue.hpp>

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

namespace zfw
{
    // MessageHeader flags
    enum
    {
        MESSAGE_UNDER_CONSTRUCTION = 1
    };

    class MessageQueueImpl: public MessageQueue
    {
        li::Mutex c_mutex;

        struct {
            uint8_t *data;
            size_t capacity, used, index;
        } buffers[NUM_BUFFERS];

        int readbuf, writebuf;

        MessageHeader* DoAllocMessage(size_t body_length);
        MessageHeader* TryRetrieve();

        public:
            MessageQueueImpl();
            virtual ~MessageQueueImpl();

            virtual void* AllocMessage(size_t length, int type, void (*releaseFunc)(void *messageBody)) override;
            virtual void FinishMessage(void* body) override;

            virtual void Post(int messageType) override;
            virtual MessageHeader* Retrieve(li::Timeout timeout) override;
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

        header->length = length;
        header->type = type;
        header->releaseFunc = releaseFunc;

        return (void*) (header + 1);
    }

    MessageHeader* MessageQueueImpl::DoAllocMessage(size_t body_length)
    {
        li::CriticalSection cs(c_mutex);

        Debug_printf(("[MessageQueue] using writebuf %i\n", writebuf));

        // Make sure the underlying buffer is large enough
        const size_t new_used = buffers[writebuf].used + sizeof(MessageHeader) + body_length;
        
        if (new_used > buffers[writebuf].capacity)
        {
            buffers[writebuf].capacity = li::maximum<size_t>(buffers[writebuf].capacity * 2, new_used);
            buffers[writebuf].capacity = li::maximum<size_t>(buffers[writebuf].capacity, WRITEBUF_MIN_ALLOC);
            buffers[writebuf].data = (uint8_t *) realloc(buffers[writebuf].data, buffers[writebuf].capacity);

            ZFW_ASSERT(buffers[writebuf].data != nullptr)

            Debug_printf(("[MessageQueue] growing writebuf to %u bytes\n", buffers[writebuf].capacity));
        }

        // Mark message as 'under construction'
        MessageHeader* header = (MessageHeader*) (buffers[writebuf].data + buffers[writebuf].used);
        header->flags = MESSAGE_UNDER_CONSTRUCTION;
        buffers[writebuf].used = new_used;

        Debug_printf(("[MessageQueue] 'used' advanced to %u\n", (unsigned int) buffers[writebuf].used));

        // c_mutex will be lifted now.
        // Retrieve will refuse to return the message until 'MESSAGE_UNDER_CONSTRUCTION' is cleared. 
        return header;
    }

    void MessageQueueImpl::FinishMessage(void* body)
    {
        // Calculate original MessageHeader address
        volatile MessageHeader* header = ((MessageHeader*) body) - 1;
        
        header->flags &= ~MESSAGE_UNDER_CONSTRUCTION;
    }

    void MessageQueueImpl::Post(int messageType)
    {
        volatile MessageHeader* header = DoAllocMessage(0);

        header->length = 0;
        header->type = messageType;
        header->releaseFunc = nullptr;

        header->flags &= ~MESSAGE_UNDER_CONSTRUCTION;
    }

    MessageHeader* MessageQueueImpl::Retrieve(li::Timeout timeout)
    {
        while (true)
        {
            MessageHeader* header = TryRetrieve();

            if (header != nullptr)
                return header;

            if (timeout.timedOut())
                break;

            li::pauseThread(1);
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

        MessageHeader* header = (MessageHeader*) (buffers[readbuf].data + buffers[readbuf].index);
        
        // If this message isn't finished yet, we have to wait for it
        if (header->flags & MESSAGE_UNDER_CONSTRUCTION)
        {
            Debug_printf(("[MessageQueue] message not ready!\n", readbuf, writebuf));
            return nullptr;
        }

        // Otherwise adjust the readbuf index and return a pointer to the header
        buffers[readbuf].index += sizeof(MessageHeader) + header->length;

        Debug_printf(("[MessageQueue] 'index' advanced to %u\n", (unsigned) buffers[readbuf].index));
        Debug_printf(("[MessageQueue] returning %u-byte message \"%i\"\n", (unsigned) header->length, header->type));

        return header;
    }

    MessageQueue* MessageQueue::Create()
    {
        return new MessageQueueImpl();
    }
}