
#include "render.hpp"

#include "rendering.hpp"

#include <glm/gtc/matrix_transform.hpp>

    static void gluPerspective( float fov, float aspect, float zNear, float zFar )
    {
        float fH = ( float )( zNear * tan( fov / 360.0f * f_pi ) );
        float fW = fH * aspect;

        glFrustum( -fW, fW, -fH, fH, zNear, zFar );
    }

namespace zfw
{
    namespace render
    {
        GLStates enabled = { false, false, false, false, false, false, false };

        glm::vec4 currentColour;

        static DynBatch_t batch;
        //static bool batchingEnabled = true;

        using zr::boundProgram;

        void DynBatch_t::AddQuadUnsafe( const glm::vec2& a, const glm::vec2& b, float z, const glm::vec4& colour )
        {
            AddVertexUnsafe( glm::vec3(a.x, a.y, z), colour );
            AddVertexUnsafe( glm::vec3(a.x, b.y, z), colour );
            AddVertexUnsafe( glm::vec3(b.x, a.y, z), colour );

            AddVertexUnsafe( glm::vec3(b.x, a.y, z), colour );
            AddVertexUnsafe( glm::vec3(a.x, b.y, z), colour );
            AddVertexUnsafe( glm::vec3(b.x, b.y, z), colour );
        }

        void DynBatch_t::AddQuadUnsafe( const glm::vec2& a, const glm::vec2& b, float z, const glm::vec2 uv[2], const glm::vec4& colour )
        {
            AddVertexUnsafe( glm::vec3(a.x, a.y, z), glm::vec2(uv[0].x, uv[0].y), colour );
            AddVertexUnsafe( glm::vec3(a.x, b.y, z), glm::vec2(uv[0].x, uv[1].y), colour );
            AddVertexUnsafe( glm::vec3(b.x, a.y, z), glm::vec2(uv[1].x, uv[0].y), colour );

            AddVertexUnsafe( glm::vec3(b.x, a.y, z), glm::vec2(uv[1].x, uv[0].y), colour );
            AddVertexUnsafe( glm::vec3(a.x, b.y, z), glm::vec2(uv[0].x, uv[1].y), colour );
            AddVertexUnsafe( glm::vec3(b.x, b.y, z), glm::vec2(uv[1].x, uv[1].y), colour );
        }

        void DynBatch_t::AddQuadUnsafe( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const glm::vec2 uv[2], const glm::vec4& colour )
        {
            AddVertexUnsafe( glm::vec3(a.x, a.y, a.z), glm::vec2(uv[0].x, uv[0].y), colour );
            AddVertexUnsafe( glm::vec3(b.x, b.y, b.z), glm::vec2(uv[0].x, uv[1].y), colour );
            AddVertexUnsafe( glm::vec3(d.x, d.y, d.z), glm::vec2(uv[1].x, uv[0].y), colour );

            AddVertexUnsafe( glm::vec3(d.x, d.y, d.z), glm::vec2(uv[1].x, uv[0].y), colour );
            AddVertexUnsafe( glm::vec3(b.x, b.y, b.z), glm::vec2(uv[0].x, uv[1].y), colour );
            AddVertexUnsafe( glm::vec3(c.x, c.y, c.z), glm::vec2(uv[1].x, uv[1].y), colour );
        }

        void DynBatch_t::AddVertexUnsafe( const glm::vec3& pos, const glm::vec4& colour )
        {
            coords.addUnsafe( pos.x );
            coords.addUnsafe( pos.y );

            if ( flags & R_3D )
                coords.addUnsafe( pos.z );

            if ( flags & R_NORMALS )
            {
                normals.addUnsafe( 0.0f );
                normals.addUnsafe( 0.0f );
                normals.addUnsafe( 0.0f );
            }

            if ( flags & R_UVS )
            {
                uvs.addUnsafe( 0.0f );
                uvs.addUnsafe( 0.0f );
            }

            if ( flags & R_COLOURS )
            {
                colours.addUnsafe( colour.r );
                colours.addUnsafe( colour.g );
                colours.addUnsafe( colour.b );
                colours.addUnsafe( colour.a );
            }

            numVertices++;
        }

        void DynBatch_t::AddVertexUnsafe( const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& colour )
        {
            coords.addUnsafe( pos.x );
            coords.addUnsafe( pos.y );

            if ( flags & R_3D )
                coords.addUnsafe( pos.z );

            if ( flags & R_NORMALS )
            {
                normals.addUnsafe( 0.0f );
                normals.addUnsafe( 0.0f );
                normals.addUnsafe( 0.0f );
            }

            if ( flags & R_UVS )
            {
                uvs.addUnsafe( uv.x );
                uvs.addUnsafe( uv.y );
            }

            if ( flags & R_COLOURS )
            {
                colours.addUnsafe( colour.r );
                colours.addUnsafe( colour.g );
                colours.addUnsafe( colour.b );
                colours.addUnsafe( colour.a );
            }

            numVertices++;
        }

        void DynBatch_t::PreAlloc( size_t count )
        {
            if ( flags & R_3D )
                coords.resize( (numVertices + count) * 3, true );
            else
                coords.resize( (numVertices + count) * 2, true );

            if ( flags & R_NORMALS )
                normals.resize( (numVertices + count) * 3, true );

            if ( flags & R_UVS )
                uvs.resize( (numVertices + count) * 2, true );

            if ( flags & R_COLOURS )
                colours.resize( (numVertices + count) * 4, true );
        }

        inline void SetColour( glm::vec4 colour )
        {
            currentColour = colour;

            /*if ( colour != currentColour )
            {
                Renderer::Flush();

                //if ( boundProgram != nullptr )
                //    boundProgram->SetBlendColour( colour );
                //else
                    glColor4fv( &colour[0] );

                currentColour = colour;
            }*/
        }

        inline void SetBatchType( GLenum type )
        {
            if (type != batch.format)
            {
                Renderer1::Flush();
                batch.format = type;
            }
        }

        void Renderer1::Init()
        {
            batch.format = 0;
            batch.flags = 0;
            batch.numVertices = 0;
        }

        void Renderer1::Flush()
        {
            if ( batch.numVertices > 0 )
            {
                zr::SetArrayBuffer(0);
                
                const int d = (batch.flags & R_3D) ? 3 : 2;

                zr::boundFormat = nullptr;

                GLSetClientState( VERTEX_ARRAY, true );
                glVertexPointer( d, GL_FLOAT, d * sizeof(float), batch.coords.getPtrUnsafe() );

                if (batch.flags & R_NORMALS)
                {
                    GLSetClientState( NORMAL_ARRAY, true );
                    glNormalPointer( GL_FLOAT, 3 * sizeof(float), batch.normals.getPtrUnsafe() );
                }
                else
                    GLSetClientState( NORMAL_ARRAY, false );

                if (batch.flags & R_UVS)
                {
                    GLSetClientState( TEXTURE_COORD_ARRAY, true );
                    glTexCoordPointer( 2, GL_FLOAT, 2 * sizeof(float), batch.uvs.getPtrUnsafe() );
                }
                else
                    GLSetClientState( TEXTURE_COORD_ARRAY, false );

                if (batch.flags & R_COLOURS)
                {
                    GLSetClientState( COLOR_ARRAY, true );
                    glColorPointer( 4, GL_FLOAT, 4 * sizeof(float), batch.colours.getPtrUnsafe() );
                }
                else
                    GLSetClientState( COLOR_ARRAY, false );

                glDrawArrays( batch.format, 0, batch.numVertices );
                numDrawCalls++;

                batch.numVertices = 0;
                batch.coords.clear(true);
                batch.normals.clear(true);
                batch.uvs.clear(true);
                batch.colours.clear(true);
            }
        }

        void R::AddLightPt( const glm::vec3& pos, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float range )
        {
            if ( boundProgram != nullptr )
            {
                glm::vec3 transformPos;

                //if ( inWorldSpace )
                {
                    glm::mat4 transform;
                    glGetFloatv( GL_MODELVIEW_MATRIX, &transform[0][0] );
                    transformPos = glm::vec3( transform * glm::vec4( pos, 1.0f ) );
                }

                boundProgram->AddLightPt( transformPos, ambient, diffuse, specular, range );
            }
        }

		void R::Clear()
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

        void R::ClearLights()
        {
            if ( boundProgram != nullptr )
                boundProgram->ClearLights();
        }

        void R::DrawDIMesh( media::DIMesh* mesh, const glm::mat4& modelView )
        {
            Renderer1::Flush();

            zr::SetArrayBuffer(0);
            zr::SetElementBuffer(0);
            
            glPushMatrix();
            glMultMatrixf( &modelView[0][0] );

            glColor4fv( &currentColour[0] );

            const int d = (mesh->flags & R_3D) ? 3 : 2;

            zr::boundFormat = nullptr;

            GLSetClientState( VERTEX_ARRAY, true );
            glVertexPointer( d, GL_FLOAT, d * sizeof(float), mesh->coords.getPtrUnsafe() );

            if (mesh->flags & R_NORMALS)
            {
                GLSetClientState( NORMAL_ARRAY, true );
                glNormalPointer( GL_FLOAT, 3 * sizeof(float), mesh->normals.getPtrUnsafe() );
            }
            else
                GLSetClientState( NORMAL_ARRAY, false );

            if (mesh->flags & R_UVS)
            {
                GLSetClientState( TEXTURE_COORD_ARRAY, true );
                glTexCoordPointer( 2, GL_FLOAT, 2 * sizeof(float), mesh->uvs.getPtrUnsafe() );
            }
            else
                GLSetClientState( TEXTURE_COORD_ARRAY, false );

            /*if (mesh->flags & R_COLOURS)
            {
                GLSetClientState( COLOR_ARRAY, true );
                glColorPointer( 4, GL_FLOAT, 4 * sizeof(float), mesh->colours.getPtrUnsafe() );
            }
            else*/
                GLSetClientState( COLOR_ARRAY, false );

            if ( mesh->layout == MESH_LINEAR )
                glDrawArrays( GL_TRIANGLES, 0, mesh->numVertices );
            else
                glDrawElements( GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, mesh->indices.getPtrUnsafe() );

            numDrawCalls++;

            glPopMatrix();
        }

        void R::DrawLine( const glm::vec3& a, const glm::vec3& b )
        {
            SetBatchType( GL_LINES );

            batch.PreAlloc( 2 );

            batch.AddVertexUnsafe( a, currentColour );
            batch.AddVertexUnsafe( b, currentColour );
        }

        void R::DrawRectangle( glm::vec2 a, glm::vec2 b, float z )
        {
            GLSet( TEXTURE_2D, false );
            SetBatchType( GL_TRIANGLES );

            batch.PreAlloc( 6 );
            batch.AddQuadUnsafe( a, b, z, currentColour );
        }

        void R::DrawTexture( ITexture* tex, glm::ivec2 pos )
        {
            static const glm::vec2 uv[2] = { glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };

            GLSet( TEXTURE_2D, true );
            SetBatchType( GL_TRIANGLES );
            SetTexture( tex );

            batch.PreAlloc( 6 );
            batch.AddQuadUnsafe( glm::vec2(pos), glm::vec2(pos) + glm::vec2(tex->GetSize()), 0.0f, uv, currentColour );
        }

        void R::DrawTexture( ITexture* tex, const glm::vec3& pos, glm::vec2 size )
        {
            static const glm::vec2 uv[2] = { glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };

            GLSet( TEXTURE_2D, true );
            SetBatchType( GL_TRIANGLES );
            SetTexture( tex );

            batch.PreAlloc( 6 );
            batch.AddQuadUnsafe( glm::vec2(pos), glm::vec2(pos) + glm::vec2(size), pos.z, uv, currentColour );
        }

        void R::DrawTexture( ITexture* tex, glm::vec2 pos, glm::vec2 size, const glm::vec2 uv[2] )
        {
            GLSet( TEXTURE_2D, true );
            SetBatchType( GL_TRIANGLES );
            SetTexture( tex );

            batch.PreAlloc( 6 );
            batch.AddQuadUnsafe( pos, pos + size, 0.0f, uv, currentColour );
        }

        void R::DrawTexture( ITexture* tex, glm::vec2 pos, glm::vec2 size, const glm::vec2 uv[2], const glm::mat4& transform )
        {
            GLSet( TEXTURE_2D, true );
            SetBatchType( GL_TRIANGLES );
            SetTexture( tex );

            glm::vec3 a = glm::vec3(transform * glm::vec4(pos.x, pos.y, 0.0f, 1.0f));
            glm::vec3 b = glm::vec3(transform * glm::vec4(pos.x, pos.y + size.y, 0.0f, 1.0f));
            glm::vec3 c = glm::vec3(transform * glm::vec4(pos.x + size.x, pos.y + size.y, 0.0f, 1.0f));
            glm::vec3 d = glm::vec3(transform * glm::vec4(pos.x + size.x, pos.y, 0.0f, 1.0f));

            batch.PreAlloc( 6 );
            batch.AddQuadUnsafe( a, b, c, d, uv, currentColour );
        }

        void R::EnableDepthTest( bool enable )
        {
            GLSet( DEPTH_TEST, enable);
        }

        void R::PopTransform()
        {
            Renderer1::Flush();

            glPopMatrix();
        }

        void R::PushTransform( const glm::mat4& transform )
        {
            Renderer1::Flush();

            glPushMatrix();
            glMultMatrixf( &transform[0][0] );
        }

        void R::SelectShadingProgram( IProgram* program )
        {
            if ( boundProgram != static_cast<GLProgram*>(program) )
            {
                Renderer1::Flush();

                boundProgram = static_cast<GLProgram*>(program);

                glUseProgram( boundProgram != nullptr ? boundProgram->prog : 0 );
                numShaderSwitches++;

                zr::currentTexture2D = 0;
            }
        }

        void R::Set2DView()
        {
            Renderer1::Flush();

            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();

            glOrtho( 0.0, r_viewportw->basictypes.i, r_viewporth->basictypes.i, 0.0, 1.0f, -1.0f );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
        }

        void R::SetAmbient( const glm::vec3& colour )
        {
            if ( boundProgram != nullptr )
            {
                boundProgram->SetAmbient( colour );
            }
        }

        void R::SetBlendColour( const glm::vec4& colour )
        {
            SetColour( colour );
        }

        void R::SetBlendMode( BlendMode_t blendMode )
        {
            switch ( blendMode )
            {
                case BLEND_NORMAL:
                    if ( glBlendEquationSeparate )
                        glBlendEquationSeparate( GL_FUNC_ADD, GL_MAX );

                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    break;

                case BLEND_ADD:
                    if ( glBlendEquationSeparate )
                        glBlendEquationSeparate( GL_FUNC_ADD, GL_FUNC_ADD );

                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                    break;

                /*case subtractive:
                    if ( glApi.functions.glBlendEquationSeparate )
                        glApi.functions.glBlendEquationSeparate( GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD );

                    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                    break;*/

                /*case Blend_invert:
                    if ( glFs.glBlendEquationSeparate )
                        glFs.glBlendEquationSeparate( GL_FUNC_ADD, GL_FUNC_ADD );

                    glBlendFunc( GL_ZERO, GL_ONE_MINUS_DST_COLOR );
                    break;*/

                default:
                    ;
            }
        }

        void R::SetCamera( const world::Camera& cam )
        {
            R::SetCamera( cam.eye, cam.center, cam.up );
        }

        void R::SetCamera( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up )
        {
            glm::mat4 modelView;

            modelView = glm::lookAt( glm::vec3( eye.x, -eye.y, eye.z ), glm::vec3( center.x, -center.y, center.z ), glm::vec3( up.x, -up.y, up.z ) );
            modelView = glm::scale( modelView, glm::vec3( 1.0f, -1.0f, 1.0f ) );

            glLoadMatrixf( &modelView[0][0] );
        }

        void R::SetClearColour( const glm::vec4& colour )
        {
            glClearColor( colour.r, colour.g, colour.b, colour.a );
        }

        void R::SetPerspectiveProjection( float nearz, float farz, float fov )
        {
            Renderer1::Flush();

            glMatrixMode( GL_PROJECTION );
            glLoadIdentity();

            const GLfloat viewportRatio = ( GLfloat )( r_viewportw->basictypes.i ) / ( GLfloat )( r_viewporth->basictypes.i );

            gluPerspective( fov, viewportRatio, nearz, farz );

            glMatrixMode( GL_MODELVIEW );
            glLoadIdentity();
        }

        void R::SetRenderFlags( int flags )
        {
            if ( batch.flags != flags )
            {
                Renderer1::Flush();

                batch.flags = flags;
            }
        }

        void R::SetTexture( ITexture* tex )
        {
            render::Renderer1::Flush();

            if ( tex != nullptr )
            {
                zr::SetTexture2D( static_cast<GLTexture*>( tex )->id );
                GLSet( TEXTURE_2D, true );
            }
            else
            {
                zr::SetTexture2D( 0 );
                GLSet( TEXTURE_2D, false );
            }
        }
    }
}
