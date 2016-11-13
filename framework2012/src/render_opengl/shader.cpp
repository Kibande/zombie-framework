
#include "render.hpp"

// TODO: limit max_* w/ R_MAXLIGHTS

namespace zfw
{
    namespace render
    {
        GLShader::GLShader(GLuint shader)
                :shader( shader )
        {
        }

        GLShader::~GLShader()
        {
            glDeleteShader( shader );
        }

        GLShader* GLShader::Load( GLenum type, const char* path )
        {
            Reference<SeekableInputStream> input = Sys::OpenInput( path );

            if ( input == nullptr )
            {
                Sys::RaiseException( EX_SHADER_LOAD_ERR, "GLShader::Load", "failed to load shader source '%s'", path );
                return nullptr;
            }

            String sourceStr = input->readWhole();
            input.release();

            const char* source = sourceStr;

            GLuint shader = glCreateShader( type );
            glShaderSource( shader, 1, &source, 0 );
            glCompileShader( shader );

            int status = GL_FALSE;
            glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

            if ( status == GL_FALSE )
            {
                int logLength = 0;
                int charsWritten  = 0;
                Array<GLchar> log;

                glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );

                log.resize( logLength + 1 );
                glGetShaderInfoLog( shader, logLength, &charsWritten, log.getPtr() );

                Sys::RaiseException( EX_SHADER_COMPILE_ERR, "GLShader::Load", "failed to compile shader '%s'.\n\ncompilation log:\n\n%s", path, log.getPtrUnsafe() );
            }

            return new GLShader(shader);
        }

        GLProgram::GLProgram( GLuint prog, GLShader* pixel, GLShader* vertex, const HashMap<String, String>& attrib ) : prog( prog ), pixel( pixel ), vertex( vertex )
        {
            //max_lights_amb = attrib.get( (String&&) "max_lights_amb" ).toInt();
            max_lights_pt = attrib.get( (String&&) String("max_lights_pt") ).toInt();
            num_tex = attrib.get( (String&&) String("num_tex") ).toInt();

            R::SelectShadingProgram( this );

            blend_colour = glGetUniformLocation( prog, "blend_colour" );
            ambient = glGetUniformLocation( prog, "ambient" );

            pt_count = glGetUniformLocation( prog, "pt_count" );

            for ( int i = 0; i < max_lights_pt; i++ )
            {
                pt_pos[i] =         glGetUniformLocation( prog, Sys::tmpsprintf( 20, "pt_pos[%i]", i ) );
                pt_ambient[i] =     glGetUniformLocation( prog, Sys::tmpsprintf( 20, "pt_ambient[%i]", i ) );
                pt_diffuse[i] =     glGetUniformLocation( prog, Sys::tmpsprintf( 20, "pt_diffuse[%i]", i ) );
                pt_range[i] =       glGetUniformLocation( prog, Sys::tmpsprintf( 20, "pt_range[%i]", i ) );
            }

            for ( int i = 0; i < num_tex; i++ )
            {
                tex[i] =            glGetUniformLocation( prog, Sys::tmpsprintf( 20, "tex[%i]", i ) );

                if (tex[i] >= 0)
                    glUniform1i(tex[i], i);
            }
        }

        GLProgram::~GLProgram()
        {
            if ( pixel != nullptr )
            {
                glDetachShader( prog, pixel->shader );
                pixel.release();
            }

            if ( vertex != nullptr )
            {
                glDetachShader( prog, vertex->shader );
                vertex.release();
            }

            glDeleteProgram( prog );
        }

        void GLProgram::AddLightPt( const glm::vec3& pos, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float range )
        {
            if ( pt_pos[num_lights_pt] >= 0 )
                glUniform3fv( pt_pos[num_lights_pt], 1, &pos[0] );

            if ( pt_ambient[num_lights_pt] >= 0 )
                glUniform3fv( pt_ambient[num_lights_pt], 1, &ambient[0] );

            if ( pt_diffuse[num_lights_pt] >= 0 )
                glUniform3fv( pt_diffuse[num_lights_pt], 1, &diffuse[0] );

            if ( pt_range[num_lights_pt] >= 0 )
                glUniform1f( pt_range[num_lights_pt], range );

            num_lights_pt++;

            if ( pt_count >= 0 )
                glUniform1i( pt_count, num_lights_pt );
        }

        void GLProgram::ClearLights()
        {
            num_lights_pt = 0;
        }

        int GLProgram::GetUniformLocation(const char *name)
        {
            return glGetUniformLocation( prog, name );
        }

        GLProgram* GLProgram::Load( const char* path )
        {
            if ( gl_allowshaders->basictypes.i <= 0 )
            {
                Sys::RaiseException( EX_FEATURE_DISABLED, "GLProgram::Load", "the shader program functionality is disabled" );
                return nullptr;
            }

            Object<GLShader> pixel, vertex;

            pixel = GLShader::Load( GL_FRAGMENT_SHADER, (String) path + "_pixel.glsl" );
            vertex = GLShader::Load( GL_VERTEX_SHADER, (String) path + "_vertex.glsl" );

            GLuint prog = glCreateProgram();

            if ( prog == 0 )
                Sys::RaiseException( EX_BACKEND_ERR, "GLProgram::Load", "failed to allocate OpenGL shader program object" );

            glAttachShader( prog, pixel->shader );
            glAttachShader( prog, vertex->shader );
            glLinkProgram( prog );

            int status = GL_FALSE;
            glGetProgramiv( prog, GL_LINK_STATUS, &status );

            if ( status == GL_FALSE )
            {
                int logLength = 0;
                int charsWritten  = 0;
                Array<GLchar> log;

                glGetProgramiv( prog, GL_INFO_LOG_LENGTH, &logLength );

                log.resize( logLength + 1 );
                glGetProgramInfoLog( prog, logLength, &charsWritten, log.getPtr() );

                Sys::RaiseException( EX_SHADER_COMPILE_ERR, "GLProgram::Load", "failed to link shader '%s'.\n\ncompilation log:\n\n%s", path, log.getPtrUnsafe() );
            }

            HashMap<String, String> attribMap;
            List<String> tokens;

            Reference<SeekableInputStream> input = Sys::OpenInput( (String)path + ".attrib" );

            while ( isReadable( input ) )
            {
                String line = input->readLine();
                tokens.clear( true );

                if ( line.parse( tokens, ' ' ) >= 2 )
                    attribMap.set( (String&&)tokens[0], (String&&)tokens[1] );
            }

            //select();

            return new GLProgram( prog, pixel.detach(), vertex.detach(), attribMap );
        }

        void GLProgram::SetAmbient( const glm::vec3& colour )
        {
            if ( ambient >= 0 )
                glUniform3fv( ambient, 1, &colour[0] );
        }

        void GLProgram::SetBlendColour( const glm::vec4& colour )
        {
            if ( blend_colour >= 0 )
                glUniform4fv( blend_colour, 1, &colour[0] );
        }

        void GLProgram::SetUniform(int location, CFloat3& vec)
        {
            if (location != -1)
                glUniform3fv(location, 1, &vec[0]);
        }
    }
}
