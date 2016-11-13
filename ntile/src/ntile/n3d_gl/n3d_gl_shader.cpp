
#include "n3d_gl_internal.hpp"

#include <framework/system.hpp>

namespace n3d
{
    GLShaderProgram::GLShaderProgram(GLRenderer* glr, String&& path)
            : glr(glr), state(CREATED), path(std::forward<String>(path))
    {
        program = 0;
    }

    bool GLShaderProgram::Realize(IResourceManager2* resMgr)
    {
        // TODO: preloading
        // TODO: leaks on error
        zombie_assert(program == 0);

        String vertexPath = (String) path + ".vert";
        String pixelPath = (String) path + ".frag";

        char* vertexSource = nullptr, * pixelSource = nullptr;

        if (!glr->LoadShader(vertexPath, &vertexSource) || !glr->LoadShader(pixelPath, &pixelSource))
        {
            free(vertexSource);

            g_sys->Printf(kLogError, "Shader preprocessing failed - shader program '%s'", path.c_str());
            return false;
        }

        GLuint vertexShader = p_CreateShader(vertexPath, vertexSource, GL_VERTEX_SHADER);
        GLuint pixelShader = p_CreateShader(pixelPath, pixelSource, GL_FRAGMENT_SHADER);

        free(vertexSource);
        free(pixelSource);

        program = glCreateProgram();

        if ( program == 0 )
            return ErrorBuffer::SetError3(EX_BACKEND_ERR, 2,
                    "desc", "failed to allocate OpenGL shader program object",
                    "function", li_functionName
                    ), false;

        glAttachShader( program, pixelShader );
        glAttachShader( program, vertexShader );
        glLinkProgram( program );

        int status = GL_FALSE;
        glGetProgramiv( program, GL_LINK_STATUS, &status );

        if ( status == GL_FALSE )
        {
            int logLength = 0;
            int charsWritten  = 0;
            Array<GLchar> log;

            glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLength );

            log.resize( logLength + 1 );
            glGetProgramInfoLog( program, logLength, &charsWritten, log.getPtr() );

            glDeleteProgram(program);
            program = 0;

            g_sys->Printf(kLogError, "Shader linking failed - path=%s", path.c_str());
            g_sys->Printf(kLogError, "Program compilation log:\n%s", log.getPtr());

            return ErrorBuffer::SetError3(EX_SHADER_COMPILE_ERR, 3,
                    "desc", sprintf_255("Failed to link shader program '%s'.", path.c_str()),
                    "solution", "Please make sure your graphics adapter is supported by this application and its driver is up-to-date.",
                    "function", li_functionName
                    ), false;
        }

        return true;
    }

    int GLShaderProgram::GetUniform(const char* name)
    {
        return glGetUniformLocation(program, name);
    }

    GLuint GLShaderProgram::p_CreateShader(const char* filename, const char* source, GLuint type)
    {
        String sourceStr;

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

            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

            log.resize(logLength + 1);
            glGetShaderInfoLog( shader, logLength, &charsWritten, log.getPtr() );

            glDeleteShader(shader);

            g_sys->Printf(kLogError, "Shader compilation failed - filename=%s,type=0x%04X", filename, type);
            g_sys->Printf(kLogError, "Shader compilation log:\n%s", log.getPtr());

            return ErrorBuffer::SetError3(EX_SHADER_COMPILE_ERR, 3,
                    "desc", sprintf_255("Failed to link shader program '%s'.", filename),
                    "solution", "Please make sure your graphics adapter is supported by this application and its driver is up-to-date.",
                    "function", li_functionName
                    ), 0;
        }

        return shader;
    }

    void GLShaderProgram::SetUniformInt(int id, int value)
    {
        glUniform1i(id, value);
    }

    void GLShaderProgram::SetUniformVec3(int id, const Float3& value)
    {
        glUniform3fv(id, 1, &value.x);
    }

    void GLShaderProgram::SetUniformVec4(int id, const Float4& value)
    {
        glUniform4fv(id, 1, &value.x);
    }

    void GLShaderProgram::Unrealize()
    {
        if (program)
        {
            glDeleteProgram(program);
            program = 0;
        }
    }
}
