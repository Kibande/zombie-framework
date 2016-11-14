
#include "RenderingKitImpl.hpp"

#include <vector>

#ifndef RENDERING_KIT_USING_OPENGL_ES
#define USE_MAPPED_BUFFER
#endif

namespace RenderingKit
{
    using namespace zfw;

    class GLVertexCache : public IGLVertexCache
    {
        protected:
            IVertexCacheOwner* owner;
            size_t bytesUsed, size;

            GLuint vbo;

			void* mapped;
			std::vector<uint8_t> storage;

            bool Recreate(size_t size);

        public:
            GLVertexCache(size_t initSize);

            virtual void Release() override { delete this; }

            virtual void* Alloc(IVertexCacheOwner* owner, size_t sizeInBytes) override;
            virtual void Flush() override;

            virtual GLuint GetVBO() override { return vbo; }
    };

    IGLVertexCache* p_CreateVertexCache(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, size_t size)
    {
        return new GLVertexCache(size);
    }

    GLVertexCache::GLVertexCache(size_t initSize)
    {
        owner = nullptr;
        bytesUsed = 0;
        mapped = nullptr;

        glGenBuffers(1, &vbo);

        Recreate(initSize);
    }

    void* GLVertexCache::Alloc(IVertexCacheOwner* owner, size_t sizeInBytes)
    {
        if (owner != this->owner)
        {
            Flush();

            this->owner = owner;
        }

        if (bytesUsed + sizeInBytes > size)
        {
            Flush();

            // TODO: Grow VBO
        }

        ZFW_ASSERT(bytesUsed + sizeInBytes <= size)

#ifdef USE_MAPPED_BUFFER
        if (mapped == nullptr)
        {
            //Recreate(size);
            GLStateTracker::BindArrayBuffer(vbo);
			mapped = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            ZFW_ASSERT(mapped != nullptr)
        }

        void* ptr = reinterpret_cast<uint8_t*>(mapped) + bytesUsed;
        bytesUsed += sizeInBytes;
        return ptr;
#else
		void* ptr = &storage[bytesUsed];
		bytesUsed += sizeInBytes;
		return ptr;
#endif
    }

    void GLVertexCache::Flush()
    {
#ifdef ZOMBIE_DEBUG
        static bool reentrant = false;

        ZFW_ASSERT(reentrant == false)
        reentrant = true;
#endif

        if (this->owner != nullptr)
        {
            GLStateTracker::BindArrayBuffer(vbo);

#ifdef USE_MAPPED_BUFFER
            glUnmapBuffer(GL_ARRAY_BUFFER);

			mapped = nullptr;
#else
			glBufferData(GL_ARRAY_BUFFER, bytesUsed, &storage[0], GL_DYNAMIC_DRAW);
#endif

            this->owner->OnVertexCacheFlush(this, bytesUsed);
            bytesUsed = 0;

            this->owner = nullptr;

            //Recreate(size);
        }

#ifdef ZOMBIE_DEBUG
        reentrant = false;
#endif
    }

    bool GLVertexCache::Recreate(size_t size)
    {
        GLStateTracker::BindArrayBuffer(vbo);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

#ifndef USE_MAPPED_BUFFER
		storage.resize(size);
#endif

        this->size = size;
        return true;
    }
}