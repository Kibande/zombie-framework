#pragma once

#include <framework/framework.hpp>

namespace zfw
{
    struct AllocChunk;

    class AllocPool
    {
        public:
            static AllocPool* Create();
            virtual ~AllocPool() {}

            virtual bool Alloc(size_t size, AllocChunk* chunk) = 0;
            virtual void Release(AllocChunk* chunk) = 0;
    };

    struct AllocChunk
    {
        AllocPool* pool;
        size_t key;

        uint8_t* buffer;
        size_t length;

        void Init() {
            pool = nullptr;
            buffer = nullptr;
            length = 0;
        }

        void Release() {
            if (pool != nullptr)
                pool->Release(this);
        }
    };
}
