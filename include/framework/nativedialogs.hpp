#pragma once

#include <framework/base.hpp>

#if defined(ZOMBIE_WINNT)
#define NATDLG_GOOD_SHELL_OPEN_FILE 1
#define NATDLG_GOOD_FILE_OPEN       1
#elif defined(ZOMBIE_OSX)
#define NATDLG_GOOD_SHELL_OPEN_FILE 1
#define NATDLG_GOOD_FILE_OPEN       0
#else
#define NATDLG_GOOD_SHELL_OPEN_FILE 0
#define NATDLG_GOOD_FILE_OPEN       0
#endif

namespace zfw
{
    // extern "C" to link against ObjC on Mac (also only use built-in types pl0x)
    extern "C" int          NatDlg_ErrorWindow(const char* message, int is_fatal);
    extern "C" const char*  NatDlg_FileOpen(const char* defaultDir, const char* defaultFileName);
    extern "C" const char*  NatDlg_FileSaveAs(const char* defaultDir, const char* defaultFileName);
    extern "C" void         NatDlg_ShellOpenFile(const char* path);

    enum
    {
        ERRWND_OPEN_REPORT = 1,
        ERRWND_DEBUG_BREAK = 2
    };
    
    // IMPORTANT: These functions use NATIVE FILESYSTEM PATHS
    // Use fopen or a native IFileSystem (Sys::CreateNativeFileSystem)
    class NativeDialogs
    {
        public:
            inline static int           ErrorWindow(const char* message, int is_fatal) { return NatDlg_ErrorWindow(message, is_fatal); }
            inline static const char*   FileOpen(const char* defaultDir, const char* defaultFileName) { return NatDlg_FileOpen(defaultDir, defaultFileName); }
            inline static const char*   FileSaveAs(const char* defaultDir, const char* defaultFileName) { return NatDlg_FileSaveAs(defaultDir, defaultFileName); }
            inline static void          ShellOpenFile(const char* path) { return NatDlg_ShellOpenFile(path); }
    };
}
