
#include "RenderingKitImpl.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/resource.hpp>
#include <framework/shader_preprocessor.hpp>
#include <framework/system.hpp>

#include <string>
#include <vector>

#include <malloc.h>

namespace RenderingKit
{
    using namespace zfw;

    class GLShader : public IGLShaderProgram
    {
        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;

        GLuint handle;

        GLuint p_LoadShader(const char* filename, const char* source, GLuint type);

        public:
            GLShader(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* path);
            ~GLShader();

            virtual const char* GetName() override { return path.c_str(); }

            virtual intptr_t GetUniformLocation(const char* name) override;
            virtual void SetUniformInt(intptr_t location, int value) override;
            virtual void SetUniformInt2(intptr_t location, Int2 value) override;
            virtual void SetUniformFloat(intptr_t location, float value) override;
            virtual void SetUniformFloat3(intptr_t location, const Float3& value) override;
            virtual void SetUniformFloat4(intptr_t location, const Float4& value) override;
            virtual void SetUniformMat4x4(intptr_t location, const glm::mat4x4& value) override;

            virtual bool GLCompile(const char* path, const char** outputNames, size_t numOutputNames) override;
            virtual int GLGetAttribLocation(const char* name) override;
            virtual void GLSetup() override;
            virtual void SetOutputNames(const char** outputNames, size_t numOutputNames) override;

            // IResource2
            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr) { return true; }
            void Unload() {}
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }

            virtual State_t GetState() const final override { return state; }

            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        private:
            State_t state = CREATED;
            std::string path;

            std::string outputNames[MAX_SHADER_OUTPUTS];
            size_t numOutputNames = 0;

        friend class IResource2;
    };

    shared_ptr<IGLShaderProgram> p_CreateShaderProgram(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* path)
    {
        return std::make_shared<GLShader>(eb, rk, rm, path);
    }

    unique_ptr<IGLShaderProgram> p_CreateShaderProgramUniquePtr(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* path)
    {
        return std::make_unique<GLShader>(eb, rk, rm, path);
    }

    GLShader::GLShader(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* path)
            : path(path)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        handle = 0;
    }

    GLShader::~GLShader()
    {
        Unrealize();
        Unload();
    }

    intptr_t GLShader::GetUniformLocation(const char* name)
    {
        GLStateTracker::UseProgram(handle);
        return glGetUniformLocation(handle, name);
    }

    bool GLShader::GLCompile(const char* path, const char** outputNames, size_t numOutputNames)
    {
        rm->CheckErrors(li_functionName);

#ifdef RENDERING_KIT_USING_OPENGL_ES
		zombie_assert(outputNames == nullptr);
#endif

        auto shaderPreprocessor = rk->GetShaderPreprocessor();

        auto vertexPath = (std::string) path + ".vert";
        auto pixelPath = (std::string) path + ".frag";

        char* vertexSource = nullptr, * pixelSource = nullptr;

        const char* vertexShaderPrepend = "";
        const char* pixelShaderPrepend = "";

        if (!shaderPreprocessor->LoadShader(vertexPath.c_str(), vertexShaderPrepend, &vertexSource)
                || !shaderPreprocessor->LoadShader(pixelPath.c_str(), pixelShaderPrepend, &pixelSource))
        {
            shaderPreprocessor->ReleaseShader(vertexSource);

            rk->GetSys()->Printf(kLogError, "Shader preprocessing failed - shader program '%s'", path);
            return false;
        }

        // FIXME: Check for errors
        GLuint vertexShader = p_LoadShader(vertexPath.c_str(), vertexSource, GL_VERTEX_SHADER);
        GLuint pixelShader = p_LoadShader(pixelPath.c_str(), pixelSource, GL_FRAGMENT_SHADER);

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

#ifndef RENDERING_KIT_USING_OPENGL_ES
        if (numOutputNames > 0)
        {
            for (size_t i = 0; i < numOutputNames; i++)
                glBindFragDataLocation(handle, i, outputNames[i]);
        }
#endif

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

        // TODO: this can be removed once GLCompile isn't called externally
        this->state = REALIZED;
        return true;
    }

    int GLShader::GLGetAttribLocation(const char* name)
    {
        zombie_assert_resource_state(REALIZED, path.c_str());

        return glGetAttribLocation(handle, name);
    }

    void GLShader::GLSetup()
    {
        zombie_assert_resource_state(REALIZED, path.c_str());

        GLStateTracker::UseProgram(handle);
        rm->CheckErrors(li_functionName);
    }

    /*int GLShader::GetUniform(const char* name)
    {
        return glGetUniformLocation(handle, name);
    }*/

    GLuint GLShader::p_LoadShader(const char* filename, const char* source, GLuint type)
    {
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

    bool GLShader::Realize(IResourceManager2* resMgr)
    {
        zombie_assert_resource_state(PRELOADED, path.c_str());

        const char* pointers[MAX_SHADER_OUTPUTS];

        for (size_t i = 0; i < numOutputNames; i++)
            pointers[i] = outputNames[i].c_str();

        return GLCompile(path.c_str(), pointers, numOutputNames);
    }

    void GLShader::SetOutputNames(const char** outputNames, size_t numOutputNames)
    {
        zombie_assert_resource_state(CREATED, path.c_str());
        zombie_assert(numOutputNames < MAX_SHADER_OUTPUTS);

        for (size_t i = 0; i < numOutputNames; i++)
            this->outputNames[i] = outputNames[i];

        this->numOutputNames = numOutputNames;
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

    void GLShader::Unrealize()
    {
        if (handle != 0)
        {
            glDeleteProgram(handle);
            handle = 0;
        }
    }
}
