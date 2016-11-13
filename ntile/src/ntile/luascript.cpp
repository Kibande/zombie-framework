
#include "luascript.hpp"

#include "lua_rpc_handler.hpp"
#include "ntile.hpp"

namespace ntile
{
    LuaScript::LuaScript(String&& path) : path(path), L(nullptr)
    {
    }

    bool LuaScript::Preload()
    {
        unique_ptr<InputStream> input(g_sys->OpenInput(path + ".lua"));
        zombie_assert(input);   // FIXME

        text = input->readWhole();
        return true;
    }

    bool LuaScript::Realize()
    {
        L = luaL_newstate();

        //luaL_openlibs(L);

        installScriptApi(L);

        int status = luaL_loadbuffer(L, text.c_str(), text.getNumBytes(), path);
        text.clear();

        if (status != 0) {
            return ErrorBuffer::SetError3(EX_SCRIPT_ERROR, 3,
                    "desc", sprintf_4095("Script Compilation Error: %s", lua_tostring(L, -1)),
                    "script", path.c_str(),
                    "functionName", li_functionName
                    ), false;
        }

        int result = lua_pcall(L, 0, LUA_MULTRET, 0);

        if (result)
            g_sys->Printf(kLogError, "Script Error: %s", lua_tostring(L, -1));

        return true;
    }

    void LuaScript::Unrealize()
    {
        lua_close(L);
    }

    void LuaScript::Unload()
    {
        text.clear();
    }
}
