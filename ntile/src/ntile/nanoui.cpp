
#include "nanoui.hpp"
#include "ntile.hpp"

#include <framework/colorconstants.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>

namespace ntile
{
    enum { kTicksPerChar = 4 };

    static const Int2 dialogSize(240, 120);

    bool NanoUI::Init()
    {
        g_res->Resource(&font,          "path=ntile/font/thin,size=2");

        auto var = g_sys->GetVarSystem();
        var->GetVariable("bind_attack", &button, 0);

        return true;
    }

    void NanoUI::Draw()
    {
        for (size_t i = 0; i < numDialogs; i++)
        {
            Int3 pos(20 + i * 20, 20 + i * 20, 5 - i * 20);
            ir->SetColour(RGBA_BLACK_A(192));
            ir->DrawRect(pos, dialogSize);

            pos += Int3(10, 10, -10);
            font->DrawText(dialogStack[i].message, dialogStack[i].charsShown, pos, RGBA_WHITE, ALIGN_LEFT | ALIGN_TOP);
        }
    }

    int NanoUI::HandleMessage(int h, MessageHeader* msg)
    {
        if (h > h_direct)
            return h;

        if (numDialogs > 0)
        {
            if (msg->type == EVENT_VKEY)
            {
                auto payload = msg->Data<EventVkey>();

                if (Vkey::Test(payload->input, button))
                {
                    numDialogs--;
                    return h_stop;
                }
            }
        }

        return h;
    }

    void NanoUI::OnTick()
    {
        for (size_t i = 0; i < numDialogs; i++)
        {
            auto& dlg = dialogStack[i];

            if (dlg.message[dlg.charsShown] != 0)
            {
                if (--dlg.wait == 0)
                {
                    dlg.charsShown++;
                    dlg.wait = kTicksPerChar;
                }
            }
        }
    }

    void NanoUI::ShowMessage(const char* text, int flags)
    {
        auto& dlg = dialogStack[numDialogs++];
        dlg.message = text;
        dlg.charsShown = (flags & kMessageInstant) ? -1 : 0;
        dlg.flags = flags;
        dlg.wait = 1;
    }
}
