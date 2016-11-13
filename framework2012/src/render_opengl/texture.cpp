
#include "render.hpp"

#include "rendering.hpp"

namespace zfw
{
    namespace render
    {
        GLTexture::GLTexture( const char* name, const char* fileName, glm::ivec2 size, GLuint id )
                : name( name ), fileName(fileName), size( size ), finalized( true ), id ( id )
        {
        }

        GLTexture::~GLTexture()
        {
            zr::SetTexture2D(0);       // TODO: should be more like Unset(this->id)
            glDeleteTextures( 1, &id );
        }

        GLTexture* GLTexture::CreateFinal( const char* name, const char* fileName, media::DIBitmap* bmp )
        {
            GLuint id;

            glGenTextures( 1, &id );
            zr::SetTexture2D(id);

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );

            GLenum format;

            switch ( bmp->format )
            {
                case TEX_RGB8: format = GL_RGB; break;
                case TEX_RGBA8: format = GL_RGBA; break;

                //TODO: default: error
            }

            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, bmp->size.x, bmp->size.y, 0, format, GL_UNSIGNED_BYTE, bmp->data.getPtrUnsafe() );

            return new GLTexture( name, fileName, bmp->size, id );
        }

        void GLTexture::Download(media::DIBitmap* bmp)
        {
            media::Bitmap::Alloc(bmp, size.x, size.y, TEX_RGBA8);

            zr::SetTexture2D(id);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp->data.getPtrUnsafe());
        }
    }
}
