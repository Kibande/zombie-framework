
#include <framework/framework.hpp>
#include <framework/nativedialogs.hpp>

// FIXME: Unicode is a must

namespace zfw
{
    using namespace li;

    static char s_pathBuffer[4096];

    // release with Allocator<wchar_t>::release();
    static wchar_t* utf8ToWchars(const char* text_in)
    {
        // FIXME: Better error checking/handling

        const size_t len = strlen(text_in);
        int count = MultiByteToWideChar(CP_UTF8, 0, text_in, len + 1, nullptr, 0);

        if (count > 0)
        {
            wchar_t* wchars = Allocator<wchar_t>::allocate(count);
            MultiByteToWideChar(CP_UTF8, 0, text_in, len + 1, wchars, count);

            return wchars;
        }

        return nullptr;
    }

    extern "C" int NatDlg_ErrorWindow(const char* message, int is_fatal)
    {
        wchar_t* message_w = utf8ToWchars(message);

        if (message_w != nullptr)
        {
            int mb = MessageBoxW(NULL, message_w, L"Zombie Framework",
#ifdef ZOMBIE_DEBUG
                MB_ICONWARNING | MB_YESNOCANCEL
#else
                (is_fatal ? MB_ICONERROR : MB_ICONWARNING) | MB_YESNO
#endif
            );

            Allocator<wchar_t>::release(message_w);

            switch (mb)
            {
                case IDYES: return ERRWND_OPEN_REPORT;
                case IDCANCEL: return ERRWND_DEBUG_BREAK;
                default: return 0;
            }
        }
        else
            return ERRWND_OPEN_REPORT;
    }

    extern "C" const char* NatDlg_FileOpen(const char* defaultDir, const char* defaultFileName)
    {
        if (defaultFileName != nullptr)
        {
            strncpy(s_pathBuffer, defaultFileName, lengthof(s_pathBuffer) - 1);
            s_pathBuffer[lengthof(s_pathBuffer) - 1] = 0;
        }
        else
            s_pathBuffer[0] = 0;

        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);

        ofn.lpstrFile = s_pathBuffer;
        ofn.nMaxFile = lengthof(s_pathBuffer);
        ofn.lpstrInitialDir = defaultDir;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn))
            return s_pathBuffer;
        else
            return nullptr;
    }

    extern "C" const char* NatDlg_FileSaveAs(const char* defaultDir, const char* defaultFileName)
    {
        if (defaultFileName != nullptr)
        {
            strncpy(s_pathBuffer, defaultFileName, lengthof(s_pathBuffer) - 1);
            s_pathBuffer[lengthof(s_pathBuffer) - 1] = 0;
        }
        else
            s_pathBuffer[0] = 0;

        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);

        ofn.lpstrFile = s_pathBuffer;
        ofn.nMaxFile = lengthof(s_pathBuffer);
        ofn.lpstrInitialDir = defaultDir;
        ofn.Flags = OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn))
            return s_pathBuffer;
        else
            return nullptr;
    }

    extern "C" void NatDlg_ShellOpenFile(const char* path)
    {
        wchar_t* path_w = utf8ToWchars(path);

        if (path_w != nullptr)
        {
            ShellExecuteW(NULL, L"open", path_w, NULL, NULL, SW_NORMAL);
            Allocator<wchar_t>::release(path_w);
        }
    }
}
