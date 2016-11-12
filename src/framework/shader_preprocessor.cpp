
#include <framework/errorbuffer.hpp>
#include <framework/framework.hpp>
#include <framework/shader_preprocessor.hpp>

#include "private.hpp"

#include <littl/FileName.hpp>

#include <regex>

namespace zfw
{
    using namespace li;

    class ShaderPreprocessorImpl: public ShaderPreprocessor
    {
        ISystem* sys;

        public:
            ShaderPreprocessorImpl(ISystem* sys) : sys(sys) {}

            virtual bool LoadShader(const char* path, const char* prepend, char** preprocessed_source_out) override;
            virtual void ReleaseShader(char* preprocessed_source) override { free(preprocessed_source); }
    };

    ShaderPreprocessor* p_CreateShaderPreprocessor(ISystem* sys)
    {
        return new ShaderPreprocessorImpl(sys);
    }

    bool ShaderPreprocessorImpl::LoadShader(const char* path, const char* prepend, char** preprocessed_source_out)
    {
        unique_ptr<InputStream> input(sys->OpenInput(path));

        if (input == nullptr)
        {
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", (const char*) sprintf_t<255>("Failed to open shader source file '%s'", path),
                    "function", li_functionName
                    ), false;
        }

        String includeBase = FileName(path).getDirectory() + "/";

        String source = prepend;

        while (!input->eof())
        {
            String line = input->readLine();

            std::cmatch match;

            // match #include "..."
            if (!line.isEmpty() && std::regex_match(line.c_str(), line.c_str() + line.getNumBytes(), match,
                    std::regex("^#include \"([^\"]+)\"$")))
            {
                char* include_source;

                if (!LoadShader(includeBase + match[1].str().c_str(), nullptr, &include_source))
                    return false;

                source += include_source;
                free(include_source);
            }
            else
                source += line + '\n';
        }

        *preprocessed_source_out = Util::StrDup(source.c_str());
        return true;
    }
}
