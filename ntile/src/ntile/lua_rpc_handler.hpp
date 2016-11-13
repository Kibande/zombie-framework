#pragma once

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <reflection/rpc.hpp>

#include <string>
#include <vector>

namespace ntile
{
class LuaRpcHandler
{
public:
    lua_State* L;
    int nextArg;
    bool hasReturnValue_;

    LuaRpcHandler(lua_State* L) : L(L) {
    }

    bool begin() {
        nextArg = 1;
        hasReturnValue_ = false;
        return true;
    }

    bool getArgument(const char*& arg) {
        arg = luaL_checkstring(L, nextArg++);
        return true;
    }

    bool getArgument(int& arg) {
        arg = luaL_checkint(L, nextArg++);
        return true;
    }

    bool getArgument(float& arg) {
        arg = (float)luaL_checknumber(L, nextArg++);
        return true;
    }

    bool getArgument(lua_State*& arg) {
        arg = L;
        return true;
    }

    template <typename T>
    bool getArgument(T*& arg) {
        arg = (T*)lua_touserdata(L, nextArg++);
        return true;
    }

    bool invoke() {
        return true;
    }

    bool end() {
        return true;
    }

    bool end(int result) {
        lua_pushinteger(L, result);

        hasReturnValue_ = true;
        return true;
    }

    bool end(const std::string& result) {
        lua_pushstring(L, result.c_str());

        hasReturnValue_ = true;
        return true;
    }

    bool hasReturnValue() {
        return hasReturnValue_;
    }
};

template <typename F, F func>
int luaRpcWrap(lua_State *L)
{
    LuaRpcHandler handler(L);
    //GET_RPC_EXECUTE(LuaRpcHandler, func)(handler);
    ::rpc::getRpcExecute<F, func, LuaRpcHandler>(func)(handler);
    return handler.hasReturnValue() ? 1 : 0;
}
}
