#pragma once

#include "nbase.hpp"

extern "C"
{
#include <lua.h>
}

#include <littl/String.hpp>

namespace ntile
{
    class LuaScript
    {
        public:
            LuaScript(String&& path);
            ~LuaScript() { Unrealize(); Unload(); }

            bool Preload();
            bool Realize();
            void Unrealize();
            void Unload();

        private:
            String path;

            String text;
            lua_State* L;
    };

    bool installScriptApi(lua_State* L);
}
