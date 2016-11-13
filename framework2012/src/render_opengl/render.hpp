#pragma once

#include <framework/event.hpp>
#include <framework/framework.hpp>

#include <GL/glew.h>

#include <littl/HashMap.hpp>
#include <littl/Exception.hpp>

#define R_MAXLIGHTS 4
#define R_MAXTEX    2

#include <ft2build.h>

#include FT_FREETYPE_H

#define GLSet( name_, enable_ )\
    if ( enable_ != zfw::render::enabled.name_ )\
    {\
        zr::FlushBatch();\
        \
        if ( enable_ )\
            glEnable( GL_##name_ );\
        else\
            glDisable( GL_##name_ );\
        \
        zfw::render::enabled.name_ = enable_;\
    }

#define GLSetClientState( name_, enable_ )\
    if ( enable_ != zfw::render::enabled.name_ )\
    {\
        if ( enable_ )\
            glEnableClientState( GL_##name_ );\
        else\
            glDisableClientState( GL_##name_ );\
        \
        zfw::render::enabled.name_ = enable_;\
    }

namespace zfw
{
    namespace render
    {
        using namespace media;

        class GLTexture;

        //extern SDL_Surface* display;

        extern Var_t* gl_allowshaders;
        extern Var_t* r_viewportw;
        extern Var_t* r_viewporth;

        extern Var_t* r_windoww;
        extern Var_t* r_windowh;

        extern int numDrawCalls, numShaderSwitches;

        extern glm::vec4 currentColour;

        struct Batch
        {
            size_t size, used;
            GLuint vbo;

            float* vertices, * uvs;
        };

        struct DynBatch_t
        {
            GLenum format;
            int flags;

            size_t numVertices;
            List<float> coords, normals, uvs, colours;

            void AddQuadUnsafe( const glm::vec2& a, const glm::vec2& b, float z, const glm::vec4& colour );
            void AddQuadUnsafe( const glm::vec2& a, const glm::vec2& b, float z, const glm::vec2 uv[2], const glm::vec4& colour );
            void AddQuadUnsafe( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const glm::vec2 uv[2], const glm::vec4& colour );
            void AddVertexUnsafe( const glm::vec3& pos, const glm::vec4& colour );
            void AddVertexUnsafe( const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& colour );
            void PreAlloc( size_t count );
        };

        struct Glyph
        {
            bool isset;
            DIBitmap bmp;

            unsigned x, y;
            int minX, maxX, minY, maxY, advance;
        };

        struct GlyphMapping
        {
            bool isset;
            glm::vec2 size, uv[2];
            float advance, minX;
        };

        extern struct GLStates
        {
            bool COLOR_ARRAY, DEPTH_TEST, NORMAL_ARRAY, SCISSOR_TEST, TEXTURE_2D, TEXTURE_COORD_ARRAY, VERTEX_ARRAY;
        }
        enabled;

        class GLFont : public IFont
        {
            String name;
            int ptsize, flags;

            /* Freetype2 maintains all sorts of useful info itself */
            FT_Face face;

            /* We'll cache these ourselves */
            int height;
            int ascent;
            int descent;
            int lineskip;

            /* The font style */
            int face_style;
            int style;

            /* Extra width in glyph bounds for text styles */
            int glyph_overhang;
            float glyph_italics;

            /* We are responsible for closing the font stream */
            Reference<SeekableInputStream> input;
            FT_Open_Args args;

            /* For non-scalable formats, we must remember which font index size */
            int font_size_family;

            /* really just flags passed into FT_Load_Glyph */
            int hinting;

            List<GlyphMapping> mappings;
            Object<GLTexture> rendertex;
            Object<Batch> batch;

            glm::mat4 drawTransform;

            public:
                GLFont( const char* path, int ptsize, int flags );
                virtual ~GLFont();

                static void Init();
                static void Exit();

                virtual void Debug() override;

                void DrawString( const char* text, const glm::vec3& pos_in, const glm::vec4& blend_in );
                virtual void DrawString( const char* text, const glm::vec3& pos_in, const glm::vec4& blend, int flags ) override;
                virtual glm::vec2 MeasureString( const char* text );
                virtual int GetLineSkip() override { return lineskip; }
                virtual glm::ivec2 GetTextSize( const char* text ) override;
                virtual void SetTransform( const glm::mat4& transform ) override { drawTransform = transform; }

                void PreloadRange( int minglyph, int maxglyph );
                bool RenderGlyph( FT_UInt cp, Glyph& glyph );
                void Setup();
        };

        class GLShader
        {
            public:
                GLuint shader;

            public:
                GLShader(GLuint shader);
                ~GLShader();

                static GLShader* Load( GLenum type, const char* path );
        };

        class GLProgram : public IProgram
        {
            public:
                GLuint prog;
                Object<GLShader> pixel, vertex;

                //uint16_t max_lights_amb;
                uint16_t max_lights_pt, num_lights_pt, num_tex;
                bool sync_pt_count;

                GLint blend_colour;
                GLint ambient;

                GLint pt_count;
                GLint pt_pos[R_MAXLIGHTS];
                GLint pt_ambient[R_MAXLIGHTS];
                GLint pt_diffuse[R_MAXLIGHTS];
                //GLuint pt_specular[MAX_LIGHTS];
                GLint pt_range[R_MAXLIGHTS];

                GLint tex[R_MAXTEX];

            public:
                GLProgram( GLuint id, GLShader* pixel, GLShader* vertex, const HashMap<String, String>& attrib );
                virtual ~GLProgram();

                void AddLightPt( const glm::vec3& pos, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float range );
                void ClearLights();
                void SetAmbient( const glm::vec3& colour );
                void SetBlendColour( const glm::vec4& colour );

                virtual int GetUniformLocation(const char *name) override;
                virtual void SetUniform(int location, CFloat3& vec) override;

                static GLProgram* Load( const char* path );
        };

        class GLTexture : public ITexture
        {
            public:
                String name, fileName;
                glm::ivec2 size;
                bool finalized;

                // preload
                Object<DIBitmap> preload;

                // finalized
                GLuint id;

            public:
                GLTexture( const char* name, const char* fileName, glm::ivec2 size, GLuint id );
                virtual ~GLTexture();

                static GLTexture* CreateFinal( const char* name, const char* fileName, DIBitmap* bmp );

                virtual bool IsFinalized() override { return finalized; }

                virtual void Download(media::DIBitmap* bmp) override;
                virtual const char* GetFileName() override { return fileName; }
                virtual glm::ivec2 GetSize() override { return size; }
        };

        class Renderer1
        {
            public:
                static void Init();
                static void Flush();
        };
    }
}
