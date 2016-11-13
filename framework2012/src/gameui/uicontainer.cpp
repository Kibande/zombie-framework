
#include "gameui.hpp"

namespace gameui
{
    static void messageBoxCloseButtonClickEventHandler(Widget* button, int x, int y)
    {
        Widget* sizer = button->GetParent();
        
        if (sizer == nullptr)
            return;

        Widget* messageBox = sizer->GetParent();

        if (messageBox == nullptr)
            return;

        WidgetContainer* ui = messageBox->GetParentContainer();

        li_tryCall(ui, PushCommand(Command(Command::ID_REMOVE_WIDGET, messageBox)));
    }

    UIContainer::UIContainer(UIResManager* res) : res(res)
    {
    }

    void UIContainer::Draw()
    {
        WidgetContainer::Draw();

        if (!modalStack.isEmpty())
        {
            iterate2(i, modalStack)
            {
                if(i.getIndex() == modalStack.getLength() - 1)
                {
                    zr::R::SetTexture(nullptr);
                    zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), RGBA_WHITE_A(0.8f));
                }

                i->Draw();
            }
        }
    }

    void UIContainer::OnFrame(double delta)
    {
        reverse_iterate2(i, modalStack)
            i->OnFrame(delta);

        iterate2 (i, commands)
        {
            auto cmd = *i;

            if(cmd.cmd_id == Command::ID_REMOVE_WIDGET)
            {
                if (modalStack.removeItem(cmd.data.widget_ptr))
                {
                    delete cmd.data.widget_ptr;
                    i.remove();
                }
            }
        }

        WidgetContainer::OnFrame(delta);
    }

    int UIContainer::OnMouseMove(int h, int x, int y)
    {
        if (!modalStack.isEmpty())
        {
            reverse_iterate2(i, modalStack)
            {
                if ((h = i->OnMouseMove((i.getIndex() == modalStack.getLength() - 1) ? h : 0, x, y)) >= 1)
                    return h;
            }

            return WidgetContainer::OnMouseMove(0, x, y);
        }
        else
            return WidgetContainer::OnMouseMove(h, x, y);
    }

    int UIContainer::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (!modalStack.isEmpty())
        {
            reverse_iterate2(i, modalStack)
            {
                if ((h = i->OnMousePrimary((i.getIndex() == modalStack.getLength() - 1) ? h : 0, x, y, pressed)) >= 1)
                    return h;
            }

            return WidgetContainer::OnMousePrimary(0, x, y, pressed);
        }
        else
            return WidgetContainer::OnMousePrimary(h, x, y, pressed);
    }

    void UIContainer::PushMessageBox(const char* message)
    {
        auto messageBox = new Popup(res);
        auto sizer = new TableSizer(res, 1);
        sizer->Add(new StaticText(res, message));
        sizer->Add(new Spacer(res, Int2(0, 20)));
        auto button = new Button(res, "Close");
        button->SetAlign(Widget::HCENTER | Widget::VCENTER);
        button->SetClickEventHandler(messageBoxCloseButtonClickEventHandler);
        button->SetExpands(false);
        sizer->Add(button);
        messageBox->Add(sizer);

        messageBox->ReloadMedia();
        messageBox->SetAlign(Widget::HCENTER | Widget::VCENTER);
        messageBox->SetArea(pos, size);

        PushModal(messageBox);
    }

    void UIContainer::PushModal(Widget * modal)
    {
        modal->SetParentContainer(this);
        modalStack.add(modal);
    }

    void UIContainer::SetArea(const Int2& pos, const Int2& size)
    {
        this->pos = pos;
        this->size = size;

        WidgetContainer::SetArea(pos, size);
    }
}