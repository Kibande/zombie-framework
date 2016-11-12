
#include "RenderingKitImpl.hpp"
#include <RenderingKit/RenderingKitUtility.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/resource.hpp>

namespace RenderingKit
{
    using namespace zfw;

    class GLTextureAtlas : public IGLTextureAtlas
    {
        public:
            GLTextureAtlas();

            virtual bool GLInitWith(shared_ptr<IGLTexture> texture, PixelAtlas* pixelAtlas) override;

            virtual shared_ptr<ITexture> GetTexture() override { return texture; }

            virtual bool AllocWithoutResizing(Int2 dimensions, Int2* pos_out) override;

        private:
            shared_ptr<IGLTexture> texture;
            unique_ptr<PixelAtlas> pixelAtlas;
    };

    // ====================================================================== //
    //  class GLTextureAtlas
    // ====================================================================== //

    shared_ptr<IGLTextureAtlas> p_CreateTextureAtlas(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
    {
        return std::make_shared<GLTextureAtlas>();
    }

    GLTextureAtlas::GLTextureAtlas()
    {
        texture = nullptr;
    }

    bool GLTextureAtlas::GLInitWith(shared_ptr<IGLTexture> texture, PixelAtlas* pixelAtlas)
    {
        this->texture = move(texture);
        this->pixelAtlas.reset(pixelAtlas);
        return true;
    }

    bool GLTextureAtlas::AllocWithoutResizing(Int2 dimensions, Int2* pos_out)
    {
        return pixelAtlas->Alloc(dimensions, *pos_out);
    }
}
