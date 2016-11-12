#pragma once

#include <framework/base.hpp>

namespace zfw
{
    class IScriptAPI
    {
        public:
            // 0 for success, >0 for error, -1 for not supported
            virtual int RegisterScriptAPI(IScriptEnv* se) = 0;
            virtual int StartupScriptAPI(IScriptHandler* scr) = 0;

        protected:
            ~IScriptAPI() {}
    };

    class IScriptEnv
    {
        public:
            virtual ~IScriptEnv() {}

            virtual bool GetContext(void** context_out, UUID_t* contextType_out,
                    const UUID_t* preferredTypes, size_t numPreferredTypes) = 0;

            virtual bool LoadScript(const char* path, int flags) = 0;

            virtual bool InvokeGlobal(const char* name, int* rc_out) = 0;
    };

    class IScriptHandler
    {
        public:
            virtual ~IScriptHandler() {}

            virtual bool Init(ISystem* sys, ErrorBuffer_t* eb) = 0;

            virtual void AddScriptAPI(shared_ptr<IScriptAPI> api) = 0;

            virtual IScriptEnv* CreateScriptEnv() = 0;
    };
}
