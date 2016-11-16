#pragma once

#ifndef ZOMBIE_DEBUG__
#define zombie_ErrorCheck(expr_) do {\
        if (!(expr_))\
            return false;\
    } while(false)
#else
#define zombie_ErrorCheck(expr_) zombie_assert(expr_)
#endif

#define zombie_ErrorDisplayPassthru(sys_, eb_, fatal_, expr_) do {\
        if (!(expr_)) {\
            (sys_)->DisplayError((eb_), (fatal_));\
            return false;\
        }\
    } while(false)

#define zombie_ErrorLog(sys_, eb_, expr_) do {\
        if (!(expr_))\
            (sys_)->PrintError((eb_), kLogError);\
    } while(false)

#ifndef ZOMBIE_NO_MACRO_POLLUTION
#define ErrorCheck      zombie_ErrorCheck
#define ErrorPassthru   zombie_ErrorCheck
#define ErrorDisplayPassthru  zombie_ErrorDisplayPassthru

#define ErrorDisplay    zombie_ErrorDisplay

#define ErrorLog        zombie_ErrorLog
#endif
