
#include "RenderingKitImpl.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/resource.hpp>
#include <framework/shader_preprocessor.hpp>

namespace RenderingKit
{
    using namespace zfw;

    class GLShader : public IGLShaderProgram
    {
        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;
        String name;

        GLuint handle;

        GLuint p_LoadShader(const char* filename, const char* source, GLuint type);

        public:
            GLShader(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name);
            ~GLShader();

            virtual const char* GetName() override { return name.c_str(); }

            virtual intptr_t GetUniformLocation(const char* name) override;
            virtual void SetUniformInt(intptr_t location, int value) override;
            virtual void SetUniformInt2(intptr_t location, Int2 value) override;
            virtual void SetUniformFloat(intptr_t location, float value) override;
            virtual void SetUniformFloat3(intptr_t location, const Float3& value) override;
            virtual void SetUniformFloat4(intptr_t location, const Float4& value) override;
            virtual void SetUniformMat4x4(intptr_t location, const glm::mat4x4& value) override;

            virtual bool GLCompile(const char* path, const char** outputNames) override;
            virtual int GLGetAttribLocation(const char* name) override;
            virtual void GLSetup() override;
    };

    shared_ptr<IGLShaderProgram> p_CreateShaderProgram(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
    {
        return std::make_shared<GLShader>(eb, rk, rm, name);
    }

    GLShader::GLShader(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        handle = 0;
    }

    GLShader::~GLShader()
    {
        glDeleteProgram(handle);
    }

    intptr_t GLShader::GetUniformLocation(const char* name)
    {
        GLStateTracker::UseProgram(handle);
        return glGetUniformLocation(handle, name);
    }

    bool GLShader::GLCompile(const char* path, const char** outputNames)
    {
        rm->CheckErrors(li_functionName);

        auto shaderPreprocessor = rk->GetShaderPreprocessor();

        String vertexPath = (String) path + ".vert";
        String pixelPath = (String) path + ".frag";

        char* vertexSource = nullptr, * pixelSource = nullptr;

        const char* vertexShaderPrepend = "";
        const char* pixelShaderPrepend = "";

        if (!shaderPreprocessor->LoadShader(vertexPath, vertexShaderPrepend, &vertexSource)
                || !shaderPreprocessor->LoadShader(pixelPath, pixelShaderPrepend, &pixelSource))
        {
            shaderPreprocessor->ReleaseShader(vertexSource);

            rk->GetSys()->Printf(kLogError, "Shader preprocessing failed - shader program '%s'", path);
            return false;
        }

        // FIXME: Check for errors
        GLuint vertexShader = p_LoadShader(vertexPath, vertexSource, GL_VERTEX_SHADER);
        GLuint pixelShader = p_LoadShader(pixelPath, pixelSource, GL_FRAGMENT_SHADER);

        shaderPreprocessor->ReleaseShader(vertexSource);
        shaderPreprocessor->ReleaseShader(pixelSource);

        if (vertexShader == 0 || pixelShader == 0)
            return false;

        handle = glCreateProgram();
        rm->CheckErrors("glCreateProgram");

        glAttachShader( handle, pixelShader );
        rm->CheckErrors("glAttachShader");

        glAttachShader( handle, vertexShader );
        rm->CheckErrors("glAttachShader");

        glBindAttribLocation(handle, kBuiltinPosition,  "in_Position");
        glBindAttribLocation(handle, kBuiltinNormal,    "in_Normal");
        glBindAttribLocation(handle, kBuiltinColor,     "in_Color");
        glBindAttribLocation(handle, kBuiltinUV,        "in_UV");
        rm->CheckErrors("glBindAttribLocation");

        if (outputNames != nullptr)
        {
            for (size_t i = 0; outputNames[i]; i++)
                glBindFragDataLocation(handle, i, outputNames[i]);
        }

        glLinkProgram( handle );

        int status = GL_FALSE;
        glGetProgramiv( handle, GL_LINK_STATUS, &status );

        if ( status == GL_FALSE )
        {
            int logLength = 0;
            int charsWritten  = 0;
            Array<GLchar> log;

            glGetProgramiv( handle, GL_INFO_LOG_LENGTH, &logLength );

            log.resize( logLength + 1 );
            glGetProgramInfoLog( handle, logLength, &charsWritten, log.getPtr() );

            glDeleteProgram(handle);
            handle = 0;

            rk->GetSys()->Printf(kLogError, "Shader linking failed - path=%s", path);
            rk->GetSys()->Printf(kLogError, "Program compilation log:\n%s", log.getPtr());

            return ErrorBuffer::SetError(eb, EX_SHADER_COMPILE_ERR,
                    "desc", (const char*) sprintf_t<255>("Failed to link shader program '%s'.", path),
                    "solution", "Please make sure your graphics adapter is supported by this application and its driver is up-to-date.",
                    "function", li_functionName,
                    nullptr),
                    false;
        }

        /*if (glGetProgramBinary != nullptr)
        {
            GLsizei length;

            glGetProgramBinary(handle, 0, &length, );
        }*/

        return true;
    }

    int GLShader::GLGetAttribLocation(const char* name)
    {
        return glGetAttribLocation(handle, name);
    }

    void GLShader::GLSetup()
    {
        GLStateTracker::UseProgram(handle);
        rm->CheckErrors(li_functionName);
    }

    /*int GLShader::GetUniform(const char* name)
    {
        return glGetUniformLocation(handle, name);
    }*/

    GLuint GLShader::p_LoadShader(const char* filename, const char* source, GLuint type)
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

            rk->GetSys()->Printf(kLogError, "Shader compilation failed - filename=%s,type=0x%04X", filename, type);
            rk->GetSys()->Printf(kLogError, "Shader compilation log:\n%s", log.getPtr());

            return ErrorBuffer::SetError(eb, EX_SHADER_COMPILE_ERR,
                    "desc", (const char*) sprintf_t<255>("Failed to compile shader '%s'.", filename),
                    "solution", "Please make sure your graphics adapter is supported by this application and its driver is up-to-date.",
                    "function", li_functionName,
                    nullptr),
                    0;
        }

        return shader;
    }

    void GLShader::SetUniformInt(intptr_t location, int value)
    {
        GLStateTracker::UseProgram(handle);
        glUniform1i(location, value);
    }

    void GLShader::SetUniformInt2(intptr_t location, Int2 value)
    {
        GLStateTracker::UseProgram(handle);
        glUniform2iv(location, 1, &value[0]);
    }

    void GLShader::SetUniformFloat(intptr_t location, float value)
    {
        GLStateTracker::UseProgram(handle);
        glUniform1f(location, value);
    }

    void GLShader::SetUniformFloat3(intptr_t location, const Float3& value)
    {
        GLStateTracker::UseProgram(handle);
        glUniform3fv(location, 1, &value[0]);
    }

    void GLShader::SetUniformFloat4(intptr_t location, const Float4& value)
    {
        GLStateTracker::UseProgram(handle);
        glUniform4fv(location, 1, &value[0]);
    }

    void GLShader::SetUniformMat4x4(intptr_t location, const glm::mat4x4& value)
    {
        GLStateTracker::UseProgram(handle);
        glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
    }
}
