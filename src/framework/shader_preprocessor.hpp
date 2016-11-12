#pragma once

#include <framework/base.hpp>

namespace zfw
{
    class ShaderPreprocessor
    {
        public:
            virtual ~ShaderPreprocessor() {}

            virtual bool LoadShader(const char* path, const char* prepend, char** preprocessed_source_out) = 0;
            virtual void ReleaseShader(char* preprocessed_source) = 0;
    };
}
