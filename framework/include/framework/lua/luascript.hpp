#pragma once

struct lua_State;

namespace zfw
{
    class ILuaScriptContext
    {
        public:
            ~ILuaScriptContext() {}

            virtual bool ExecuteFile(const char* path) = 0;

            virtual lua_State* GetLuaState() = 0;
    };
}
