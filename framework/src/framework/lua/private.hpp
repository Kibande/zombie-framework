#pragma once

#include <framework/base.hpp>

namespace zfw
{
    shared_ptr<ILuaScriptContext> p_CreateLuaScriptContext(IEngine* sys);
}
