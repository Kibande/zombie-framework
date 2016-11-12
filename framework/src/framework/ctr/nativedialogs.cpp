
#include <framework/framework.hpp>
#include <framework/nativedialogs.hpp>

namespace zfw
{
    extern "C" int NatDlg_ErrorWindow(const char* message, int is_fatal)
    {
        zombie_assert(false);
        return -1;
    }

    extern "C" const char* NatDlg_FileOpen(const char* defaultDir, const char* defaultFileName)
    {
        zombie_assert(false);
        return nullptr;
    }

    extern "C" const char* NatDlg_FileSaveAs(const char* defaultDir, const char* defaultFileName)
    {
        zombie_assert(false);
        return nullptr;
    }

    extern "C" void NatDlg_ShellOpenFile(const char* path)
    {
    }
}
