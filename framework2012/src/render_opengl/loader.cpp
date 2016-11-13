
#include "render.hpp"

#include <littl/FileName.hpp>

namespace zfw
{
    namespace render
    {
        ITexture* Loader::CreateTexture( const char* name, DIBitmap* bmp )
        {
            return GLTexture::CreateFinal( FileName(name).getFileName(), name, bmp );
        }

        IProgram* Loader::LoadShadingProgram( const char* path )
        {
            return GLProgram::Load( path );
        }

        ITexture* Loader::LoadTexture( const char* path, bool required )
        {
            DIBitmap bmp;

            if (!MediaLoader::LoadBitmap( path, &bmp, required ))
                return nullptr;

            return GLTexture::CreateFinal( path, path, &bmp );
        }

        IFont* Loader::OpenFont( const char* path, int size, int flags, int preloadMin, int preloadMax )
        {
            Object<GLFont> font = new GLFont( path, size, flags );

            font->Setup();
            font->PreloadRange( preloadMin, preloadMax );

            return font.detach();
        }
    }
}
