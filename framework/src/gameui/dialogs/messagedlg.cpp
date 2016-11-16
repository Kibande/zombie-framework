
#include <gameui/dialogs.hpp>

#include <framework/messagequeue.hpp>
#include <framework/utility/util.hpp>

namespace gameui
{
    using namespace li;

    static int s_Parse(const char* name)
    {
        return (name != nullptr) ? (int)strtol(name, nullptr, 10) : 0;
    }

    MessageDlg::MessageDlg(UIThemer* themer, const char* title, const char* query, int dlgFlags)
            : Window(themer, title, 0)
    {
        dlgMsgQueue = nullptr;
        w_textBox = nullptr;
        w_custControls = nullptr;

        SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);

        dlgMsgQueue.reset(MessageQueue::Create());

        w_tbl = new Table(1);
            w_query = new StaticText(themer, query);
            w_tbl->Add(w_query);

            if (dlgFlags & DLG_TEXT_BOX)
            {
                w_textBox = new TextBox(themer);
                w_tbl->Add(w_textBox);
            }

            if (dlgFlags & DLG_CUSTOM_CONTROLS)
            {
                w_custControls = new Table(1);
                w_tbl->Add(w_custControls);
            }

            w_buttons = new Table(1);
                if (dlgFlags & DLG_BTN_OK)
                    AddButton(themer, "OK", sprintf_t<15>("%i", DLG_BTN_OK));

                if (dlgFlags & DLG_BTN_CANCEL)
                    AddButton(themer, "Cancel", sprintf_t<15>("%i", DLG_BTN_CANCEL));

                if (dlgFlags & DLG_BTN_YES)
                    AddButton(themer, "Yes", sprintf_t<15>("%i", DLG_BTN_YES));

                if (dlgFlags & DLG_BTN_NO)
                    AddButton(themer, "No", sprintf_t<15>("%i", DLG_BTN_NO));

            if (w_buttons->GetNumRows() > 0)
                w_buttons->SetNumColumns(w_buttons->GetNumRows());

            w_buttons->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            w_tbl->Add(w_buttons);
        w_tbl->SetColumnGrowable(0, true);
        Add(w_tbl);
    }

    MessageDlg::~MessageDlg()
    {
    }

    void MessageDlg::AddButton(UIThemer* themer, const char* label, const char* name)
    {
        auto btn = new Button(themer, label);
        btn->SetName(name);
        btn->SetPrivateMessageQueue(dlgMsgQueue.get());
        w_buttons->Add(btn);
    }

    void MessageDlg::AddCustomControl(Widget* widget)
    {
        if (w_custControls != nullptr)
            w_custControls->Add(widget);
    }

    const char* MessageDlg::GetText()
    {
        if (w_textBox != nullptr)
            return w_textBox->GetText();
        else
            return nullptr;
    }

    void MessageDlg::OnFrame(double delta)
    {
        MessageHeader* msg;

        while ((msg = dlgMsgQueue->Retrieve(Timeout(0))) != nullptr)
        {
            switch (msg->type)
            {
                case EVENT_CONTROL_USED:
                {
                    auto payload = msg->Data<EventControlUsed>();

                    MESSAGE_CONSTRUCTION
                    {
                        MessageConstruction<EventMessageDlgClosed> msg(msgQueue);
                        msg->dialog = this;
                        msg->btnCode = s_Parse(payload->widget->GetName());
                    }

                    Close();
                    break;
                }
            }

            msg->Release();
        }
    }

    void MessageDlg::SetText(const char* text)
    {
        if (w_textBox != nullptr)
            w_textBox->SetText(text);
    }
}
