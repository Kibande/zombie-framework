#pragma once

#include "gameui.hpp"

namespace gameui
{
    class MessageDlg;

    enum
    {
        DLG_BTN_OK = 1,
        DLG_BTN_CANCEL = 2,
        DLG_BTN_YES = 4,
        DLG_BTN_NO = 8,
        DLG_BTN_YESNO = 12,
        DLG_TEXT_BOX = 256,
        DLG_CUSTOM_CONTROLS = 512
    };

    // Messages from gameui Dialogs ($3800..3FFF)
    enum {
        DLG_DUMMY_MSG = (0x3800 - 1),

        // In

        // Out
        EVENT_MESSAGE_DLG_CLOSED,

        // Private

        DLG_MAX_MSG
    };

    DECL_MESSAGE(EventMessageDlgClosed, EVENT_MESSAGE_DLG_CLOSED)
    {
        MessageDlg* dialog;
        int btnCode;
    };

    class MessageDlg : public Window
    {
        public:
            MessageDlg(UIThemer* themer, const char* title, const char* query, int dlgFlags);
            virtual ~MessageDlg();

            virtual void OnFrame(double delta) override;

            void AddButton(UIThemer* themer, const char* label, const char* name);
            void AddCustomControl(Widget* widget);
            const char* GetText();
            void SetText(const char* text);

        protected:

            unique_ptr<MessageQueue> dlgMsgQueue;
            int btnCode;

            Table* w_tbl;
            StaticText* w_query;
            TextBox* w_textBox;
            Table* w_custControls;
            Table* w_buttons;
    };
}
