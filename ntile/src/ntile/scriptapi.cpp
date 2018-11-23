
#include "luascript.hpp"
#include "lua_rpc_handler.hpp"
#include "ntile.hpp"

#include <reflection/api.hpp>

#include <framework/engine.hpp>

namespace ntile
{
namespace
{
    void Print(lua_State* L)
    {
        li::String text;

        for (int i = 1; i <= lua_gettop(L); i++)
        {
            if (i > 1)
                text += " ";

            text += lua_tostring(L, i);
        }

        g_sys->Printf(kLogInfo, "%s", text.c_str());
    }

    /*void ShowMessage(const char* message, int flags)
    {
        nui.ShowMessage(message, flags);
    }*/
}

#define function(func_)\
        lua_pushcfunction(L, (&luaRpcWrap<decltype(&func_), &func_>));\
        lua_setglobal(L, #func_)\

    bool installScriptApi(lua_State* L)
    {
        function(Print);
        //function(ShowMessage);

        return true;
    }
}
