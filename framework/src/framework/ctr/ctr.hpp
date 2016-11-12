
#include <framework/framework.hpp>

namespace zfw
{
    void CtrPrintInit();
    int CtrPrints(const char* format);
    int CtrPrintfv(const char* format, va_list args);
}
