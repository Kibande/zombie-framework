
#include "render.hpp"

#include FT_OUTLINE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_TRUETYPE_IDS_H

#define TTF_STYLE_NORMAL	0x00
#define TTF_STYLE_BOLD		0x01
#define TTF_STYLE_ITALIC	0x02

#define FxP_FLOOR(i)    ( ( (i) & -64 ) / 64 )
#define FxP_CEIL(i)     ( ( ( (i) + 63 ) & -64 ) / 64 )

// Handle a style only if the font does not already handle it
#define TTF_HANDLE_STYLE_BOLD       ((style & TTF_STYLE_BOLD) && !(face_style & TTF_STYLE_BOLD))
#define TTF_HANDLE_STYLE_ITALIC     ((style & TTF_STYLE_ITALIC) && !(face_style & TTF_STYLE_ITALIC))

#define FT_MAXTEXW      1024
#define FT_HSPACING     3
#define FT_VSPACING     2
#define FT_USE_BATCHING true
#define FT_BATCH_CHARS  100
#define FT_VPAD         2

// TODO: crashes with too small fonts on Bitmap::Blit with y<0

namespace zfw
{
    namespace render
    {
        static FT_Library library;

        static unsigned long OnStreamRead( FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count )
        {
            SeekableInputStream* input = reinterpret_cast<SeekableInputStream*>( stream->descriptor.pointer );

            input->setPos( offset );
            return input->read( buffer, count );
        }

        GLFont::GLFont( const char* name, int ptsize, int flags )
                : name( name ), ptsize( ptsize ), flags( flags ), face( nullptr )
        {
            args.stream = nullptr;
        }

        GLFont::~GLFont()
        {
            if ( face != nullptr )
    	        FT_Done_Face( face );

            if ( args.stream != nullptr )
    	        free( args.stream );

            if ( batch != nullptr )
                free( batch->vertices );
        }

        void GLFont::Debug()
        {
            R::DrawTexture(rendertex, glm::vec3(), glm::vec2(rendertex->GetSize()) );
        }

        void GLFont::DrawString( const char* text, const glm::vec3& pos_in, const glm::vec4& blend_in )
        {
            glm::vec3 pos = pos_in;
            glm::vec4 blend = blend_in;

            R::SetBlendColour(blend_in);
            //R::PushTransform( drawTransform );

            while ( *text )
            {
                if ( *text == '\n' )
                {
                    pos.x = pos_in.x;
                    pos.y += lineskip;
                }

                // TODO: range etc.
                if ( mappings[*text].isset )
                {
                    R::DrawTexture( rendertex, glm::vec2( pos.x + mappings[*text].minX, pos.y ), mappings[*text].size, mappings[*text].uv, drawTransform );
                    pos.x += mappings[*text].advance;
                }

                text++;
            }

            //R::PopTransform();
        }

        void GLFont::DrawString( const char* text, const glm::vec3& pos_in, const glm::vec4& blend, int flags )
        {
            if (text == nullptr || *text==0)
                return;

            if (flags & (TEXT_CENTER|TEXT_RIGHT|TEXT_MIDDLE|TEXT_BOTTOM))
            {
                glm::vec2 size = MeasureString( text );

                glm::vec3 pos = pos_in;

                if (flags & TEXT_CENTER)
                    pos.x -= size.x / 2;
                else if (flags & TEXT_RIGHT)
                    pos.x -= size.x;

                if (flags & TEXT_MIDDLE)
                    pos.y -= size.y / 2;
                else if (flags & TEXT_BOTTOM)
                    pos.y -= size.y;

                if (flags & TEXT_SHADOW)
                    DrawString( text, glm::floor(pos) + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.8f * blend.a) );

                DrawString( text, glm::floor(pos), blend );
            }
            else
            {
                if (flags & TEXT_SHADOW)
                    DrawString( text, pos_in + glm::vec3(1.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.65f * blend.a) );

                DrawString( text, pos_in, blend );
            }
        }

        void GLFont::Exit()
        {
            FT_Done_FreeType( library );
        }

        glm::ivec2 GLFont::GetTextSize( const char* text )
        {
            return glm::ivec2( MeasureString( text ) );
        }

        void GLFont::Init()
        {
            FT_Error error = FT_Init_FreeType( &library );

            if ( error )
                Sys::RaiseException( EX_BACKEND_ERR, "GLFont::Init", "Couldn't init FreeType engine (%i)\n", error );
        }

        glm::vec2 GLFont::MeasureString( const char* text )
        {
            glm::vec2 size(0.0f, height);

            while ( *text )
            {
                // TODO: fix as in DrawText
                if ( mappings[*text].isset )
                    size.x += mappings[*text].advance;

                text++;
            }

            return size;
        }

        void GLFont::PreloadRange( int minglyph, int maxglyph )
        {
            uint64_t begin, end;
            begin = Timer::getRelativeMicroseconds();

            // some temporary structures
            Array<Glyph> glyphs( maxglyph );

            unsigned totalWidth = 0, totalHeight = 0, x = 0, row = 0;
            unsigned rowSpacing = lineskip + FT_VSPACING;

            // align all the glyphs
            for ( int i = 0; i < maxglyph; i++ )
            {
                glyphs[i].isset = false;

                if ( i < minglyph )
                    continue;

                FT_UInt index = FT_Get_Char_Index( face, i );

                if ( index == 0 || !RenderGlyph( index, glyphs[i] ) )
                    continue;

                if ( x + glyphs[i].maxX + FT_HSPACING > FT_MAXTEXW )
                {
                    row++;
                    x = 0;
                }

                if ( glyphs[i].maxY - glyphs[i].minY > ( int ) rowSpacing )
                    rowSpacing = glyphs[i].maxY - glyphs[i].minY;

                glyphs[i].x = x + 1;
                glyphs[i].y = FT_VPAD + row * rowSpacing;

                x += glyphs[i].maxX - glyphs[i].minX + FT_HSPACING;

                if ( x > totalWidth )
                    totalWidth = x;

                glyphs[i].isset = true;
            }

            row++;
            totalHeight = FT_VPAD + row * rowSpacing + FT_VPAD;

            if ( totalWidth == 0 || totalHeight == 0 )
                return;

            // build one final surface
            //printf( " - %s@%i: font texture is %u x %u pixels\n", name, size, totalWidth, totalHeight );

            DIBitmap finalLayout;

            Bitmap::Alloc( &finalLayout, totalWidth, totalHeight, TEX_RGBA8 );

            int count = 0;

            for ( int i = 0; i < maxglyph; i++ )
            {
                GlyphMapping glyph = {};

                if ( ( glyph.isset = glyphs[i].isset ) )
                {
                    Bitmap::Blit( &finalLayout, glyphs[i].x, glyphs[i].y + ascent - glyphs[i].maxY, &glyphs[i].bmp );

                    glyph.uv[0].x = ( float )( glyphs[i].x ) / totalWidth;
                    glyph.uv[0].y = ( float )( glyphs[i].y ) / totalHeight;
                    glyph.uv[1].x = ( float )( glyphs[i].x + glyphs[i].maxX - glyphs[i].minX ) / totalWidth;
                    glyph.uv[1].y = ( float )( glyphs[i].y + ascent - glyphs[i].minY ) / totalHeight;
                    glyph.size.x = ( float )( glyphs[i].maxX - glyphs[i].minX );
                    glyph.size.y = ( float )( ascent - glyphs[i].minY );
                    glyph.advance = ( float )( glyphs[i].advance );
                    glyph.minX = ( float )( glyphs[i].minX );

                    count++;
                }

                mappings.add( glyph );
            }

            rendertex = GLTexture::CreateFinal( name + "_RENDER", nullptr, &finalLayout );

            end = Timer::getRelativeMicroseconds();

            printf( "GLFont: generated RenderTex with %i visible glyphs for\n\t'%s' @ %i in %" PRIu64 " us\n\n", count, name.c_str(), ptsize, end - begin );
        }

        void GLFont::Setup()
        {
            // Set-up the internal structures for the newly-created GLFont object

            input = Sys::OpenInput( name );

            if ( input == nullptr )
            {
                Sys::RaiseException( EX_ASSET_OPEN_ERR, "GLFont::Open", "failed to open font '%s'", name.c_str() );
                return;
            }

            // Allocate a FT_Stream structure (basically a SeekableInputStream equivalent)
            FT_Stream stream = (FT_Stream) calloc( sizeof( *stream ), 1 );

            if ( stream == nullptr )
            {
                Sys::RaiseException( EX_ALLOC_ERR, "GLFont::Open", "" );
                return;
            }

            // Fill in the stream structure
            stream->read = OnStreamRead;
            stream->descriptor.pointer = input;
            stream->pos = ( unsigned long ) input->getPos();
            stream->size = ( unsigned long )( input->getSize() - input->getPos() );

            // Set up another structure, basically telling FT to use our stream as the input
            args.flags = FT_OPEN_STREAM;
            args.stream = stream;

            // FreeType will create and init a FT_Face structure
            FT_Error error = FT_Open_Face( library, &args, 0, &face );

            if ( error )
            {
                Sys::RaiseException( EX_ASSET_OPEN_ERR, "GLFont::Open", "failed to load font '%s' (freeType error %i)", name.c_str(), error );
                return;
            }

            /*if ( !FT_IS_SCALABLE( face ) )
            {
                Sys::RaiseException( EX_ASSET_OPEN_ERR, "GLFont::Open", "failed to load font '%s'", path );
                return nullptr;
            }*/

            // Another thing we need to do is to set the face size
            // The size in points is multiplied by 64 because this is the way how FreeType's fixed point numbers work
            error = FT_Set_Char_Size( face, 0, ptsize * 64, 0, 0 );

            if ( error )
            {
                Sys::RaiseException( EX_BACKEND_ERR, "GLFont::Open", "failed to set font size %i for '%s'", ptsize, name.c_str() );
                return;
            }

            // Calculate various metrics
            FT_Fixed scale = face->size->metrics.y_scale;
            ascent = FxP_CEIL( FT_MulFix( face->ascender, scale ) );
            descent = FxP_CEIL( FT_MulFix( face->descender, scale ) );
            height = ascent - descent + 1;
            lineskip = FxP_CEIL( FT_MulFix( face->height, scale ) );

            // Initialize the face style
            face_style = TTF_STYLE_NORMAL;

            if ( face->style_flags & FT_STYLE_FLAG_BOLD )
                face_style |= TTF_STYLE_BOLD;

            if ( face->style_flags & FT_STYLE_FLAG_ITALIC )
                face_style |= TTF_STYLE_ITALIC;

            style = face_style;

            if ( flags & FONT_BOLD )
                style |= TTF_STYLE_BOLD;

            if ( flags & FONT_ITALIC )
                style |= TTF_STYLE_ITALIC;

            glyph_overhang = face->size->metrics.y_ppem / 10;
            glyph_italics = 0.207f;
            glyph_italics *= height;

            hinting = 0;

            if ( FT_USE_BATCHING )
            {
                batch = new Batch;
                batch->size = FT_BATCH_CHARS;
                batch->used = 0;

                batch->vbo = 0;

                batch->vertices = (float*) malloc( batch->size * 4 * 4 );

                /*if ( driver->features.gl3PlusOnly )
                {
                    glApi.functions.glGenBuffers( 1, &batch->vbo );
                    glApi.functions.glBindBuffer( GL_ARRAY_BUFFER, batch->vbo );
                    //glApi.functions.glBufferData( GL_ARRAY_BUFFER, batch->size * 6 * 4 * sizeof( float ), nullptr, GL_STREAM_DRAW );
                    glApi.functions.glBufferData( GL_ARRAY_BUFFER, batch->size * 4 * 4 * sizeof( float ), nullptr, GL_STREAM_DRAW );
                }*/
            }
        }

        bool GLFont::RenderGlyph( FT_UInt index, Glyph& glyph_out )
        {
            // Welcome to the ugliest function in this class. Follow us this way, please.

            // *** STEP 1: Load the glyph outline into face's internal cache ***

            FT_Error error = FT_Load_Glyph( face, index, FT_LOAD_DEFAULT | hinting );

            if ( error )
                return false;

            // Get some pointers
            FT_GlyphSlot glyph = face->glyph;
            FT_Glyph_Metrics* metrics = &glyph->metrics;
            FT_Outline* outline = &glyph->outline;

            // Get the glyph metrics and bounds
            glyph_out.minX = FxP_FLOOR(metrics->horiBearingX);
            glyph_out.maxX = glyph_out.minX + FxP_CEIL(metrics->width);
            glyph_out.maxY = FxP_FLOOR(metrics->horiBearingY);
            glyph_out.minY = glyph_out.maxY - FxP_CEIL(metrics->height);
            glyph_out.advance = FxP_CEIL(metrics->horiAdvance);

            // Adjustments for bold and italic text
            if ( TTF_HANDLE_STYLE_BOLD )
                glyph_out.maxX += glyph_overhang;

            if ( TTF_HANDLE_STYLE_ITALIC )
                glyph_out.maxX += (int) ceil( glyph_italics );

            // Handle the italic style
            if ( TTF_HANDLE_STYLE_ITALIC )
            {
                FT_Matrix shear;

                shear.xx = 1 << 16;
                shear.xy = (int) ( glyph_italics * (1 << 16) ) / height;
                shear.yx = 0;
                shear.yy = 1 << 16;

                FT_Outline_Transform( outline, &shear );
            }

            // *** STEP 2: Actually render the glyph (still using face's internal cache) ***

            error = FT_Render_Glyph( glyph, ft_render_mode_normal );

            if ( error )
                return false;

            // *** STEP 3: Copy the rendered glyph to yet another bitmap (this time a local one) ***

            FT_Bitmap* src = &glyph->bitmap;

            if ( src->pixel_mode != FT_PIXEL_MODE_GRAY )
            {
                // FIXME !!!
                *(volatile int*)(nullptr) = 0;
            }

            FT_Bitmap localPixmap;

            memcpy( &localPixmap, src, sizeof( localPixmap ) );

            // Adjust for bold and italic text
            if ( TTF_HANDLE_STYLE_BOLD )
            {
                int bump = glyph_overhang;
                localPixmap.pitch += bump;
                localPixmap.width += bump;
            }

            if ( TTF_HANDLE_STYLE_ITALIC )
            {
                int bump = (int) ceil( glyph_italics );
                localPixmap.pitch += bump;
                localPixmap.width += bump;
            }

            if ( localPixmap.rows != 0 )
            {
                localPixmap.buffer = (uint8_t*)calloc( localPixmap.pitch, localPixmap.rows );

                if ( localPixmap.buffer == nullptr )
                    return false;

                for ( int y = 0; y < src->rows; y++ )
                    memcpy( localPixmap.buffer + y * localPixmap.pitch, src->buffer + y * src->pitch, src->pitch );
            }

            // Handle the bold style
            if ( TTF_HANDLE_STYLE_BOLD )
            {
                uint8_t* pixmap;

                for ( int row = localPixmap.rows - 1; row >= 0; --row )
                {
                    pixmap = ( uint8_t* ) localPixmap.buffer + row * localPixmap.pitch;

                    for ( int offset = 1; offset <= glyph_overhang; offset++ )
                    {
                        for ( int col = localPixmap.width - 1; col > 0; --col )
                            pixmap[col] = (uint8_t) minimum( pixmap[col] + pixmap[col - 1], 0xFF );
                    }
                }
            }

		    if ( TTF_HANDLE_STYLE_BOLD )
            {
                glyph_out.maxX += glyph_overhang;
			    glyph_out.advance += glyph_overhang;
            }

            /*if ( TTF_HANDLE_STYLE_ITALIC )
                glyph_out.maxX += glyph_italics;*/

            // *** STEP 4: Convert the greyscale FT_Bitmap to an RGBA zfw::DIBitmap ***

			Bitmap::Alloc( &glyph_out.bmp, localPixmap.width, localPixmap.rows, TEX_RGBA8 );

	        for ( int y = 0; y < glyph_out.bmp.size.y; y++ )
	        {
		        const uint8_t* src = localPixmap.buffer + y * localPixmap.pitch;
                uint32_t* dst = ( uint32_t* ) glyph_out.bmp.data.getPtr() + y * glyph_out.bmp.size.x;

		        for ( int x = 0; x < glyph_out.bmp.size.x; x++ )
			        *dst++ = 0xFFFFFF | ( ( *src++ ) << 24 );
	        }

            free( localPixmap.buffer );

	        return true;
        }
    }
}
