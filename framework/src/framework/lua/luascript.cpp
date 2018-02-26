
#include <framework/lua/luascript.hpp>

#include <framework/system.hpp>
#include <framework/utility/errorbuffer.hpp>

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
}

#include <littl/Stream.hpp>

#include <string>

namespace zfw
{
    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class LuaScriptContext: public ILuaScriptContext
    {
        public:
            LuaScriptContext(ISystem* sys);
            ~LuaScriptContext();

            virtual bool ExecuteFile(const char* path) override;

            virtual lua_State* GetLuaState() override { return L; }

        private:
            ISystem* sys;

            lua_State* L;
    };

    // ====================================================================== //
    //  class LuaScriptContext
    // ====================================================================== //

    shared_ptr<ILuaScriptContext> p_CreateLuaScriptContext(ISystem* sys)
    {
        return std::make_shared<LuaScriptContext>(sys);
    }

    LuaScriptContext::LuaScriptContext(ISystem* sys) : sys(sys)
    {
        L = luaL_newstate();
    }

    LuaScriptContext::~LuaScriptContext()
    {
        lua_close(L);
    }

    bool LuaScriptContext::ExecuteFile(const char* path)
    {
        unique_ptr<InputStream> input(sys->OpenInput(path));
        zombie_assert(input);   // FIXME

        std::string text = input->readWhole();

        int status = luaL_loadbuffer(L, text.data(), text.size(), path);
        text.clear();

        if (status != 0) {
            return ErrorBuffer::SetError3(EX_SCRIPT_ERROR, 3,
                "desc", sprintf_4095("Script Compilation Error: %s", lua_tostring(L, -1)),
                "script", path,
                "functionName", li_functionName
            ), false;
        }

        int result = lua_pcall(L, 0, LUA_MULTRET, 0);

        if (result)
            sys->Printf(kLogError, "Script Error: %s", lua_tostring(L, -1));

        return true;
    }
}
