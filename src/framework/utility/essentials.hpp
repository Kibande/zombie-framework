#pragma once

#include <framework/base.hpp>

#define ZFW_ASSERT(expr_) if (!(expr_))\
{\
    zfw::g_essentials->AssertionFail(#expr_, li_functionName, __FILE__, __LINE__, false);\
}

#define zombie_assert(expr_) do {\
    if (!(expr_))\
        zfw::g_essentials->AssertionFail(#expr_, li_functionName, __FILE__, __LINE__, false);\
} while (false)

#ifdef _DEBUG
#define ZFW_DBGASSERT(expr_) if (!(expr_))\
{\
    zfw::g_essentials->AssertionFail(#expr_, li_functionName, __FILE__, __LINE__, true);\
}

#define zombie_debug_assert(expr_) do {\
    if (!(expr_))\
        zfw::g_essentials->AssertionFail(#expr_, li_functionName, __FILE__, __LINE__, true);\
} while (false)

#else
#define ZFW_DBGASSERT(expr_)
#define zombie_debug_assert(expr_) do {} while (false)
#endif

namespace zfw
{
    class IEssentials
    {
        public:
            virtual void            AssertionFail(const char* expr, const char* functionName, const char* file,
                    int line, bool isDebugAssertion) = 0;
            virtual void            ErrorAbort() = 0;
            virtual ErrorBuffer_t*  GetErrorBuffer() = 0;

            virtual IVideoHandler*  GetVideoHandler() = 0;      // this really doesn't belong here;
                                                                // we need it for joyIndex => name in Vkey serialization
                                                                // how to fix?
    };

    // defined in framework/utility/essentials.cpp
    extern IEssentials* g_essentials;

    inline void SetEssentials(IEssentials* essentials)
    {
        g_essentials = essentials;
    }

    inline void ErrorAbort()
    {
        g_essentials->ErrorAbort();
    }

    inline ErrorBuffer_t* GetErrorBuffer()
    {
        return g_essentials->GetErrorBuffer();
    }
}
