#ifndef RenderingKit_utility_ShaderGlobal_hpp
#define RenderingKit_utility_ShaderGlobal_hpp

#include "../RenderingKit.hpp"

namespace RenderingKit
{
    class ShaderGlobal {
    public:
        ShaderGlobal(const char* name) {
            this->name = name;
        }

        void Bind(IRenderingManager* rm) {
            this->rm = rm;
            this->handle = rm->RegisterShaderGlobal(name);
        }

        template <typename T>
        ShaderGlobal& operator = (const T& value) {
            rm->SetShaderGlobal(handle, value);
            return *this;
        }

    private:
        const char* name;
        IRenderingManager* rm = nullptr;
        intptr_t handle = -1;
    };
}

#endif
