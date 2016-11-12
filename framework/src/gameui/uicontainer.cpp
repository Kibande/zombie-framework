
#include <gameui/gameui.hpp>

#include <framework/event.hpp>
#include <framework/framework.hpp>
#include <framework/messagequeue.hpp>

namespace gameui
{
    using namespace li;

    UIContainer::~UIContainer()
    {
        iterate2 (i, modalStack)
            delete i;
    }

    void UIContainer::CenterWidget(Widget* widget)
    {
        const Int2 size = widget->GetMinSize();
        const Int3 pos = ctnAreaPos + Int3((ctnAreaSize - size) / 2, 0);

        widget->SetArea(pos, size);
    }

    bool UIContainer::DoRemoveWidget(Widget* widget)
    {
        if (!modalStack.isEmpty())
        {
            iterate2 (i, modalStack)
            {
                if (i == widget)
                {
                    modalStack.remove(i.getIndex());
                    return true;
                }
            }
        }

        return WidgetContainer::DoRemoveWidget(widget);
    }

    void UIContainer::Draw()
    {
        WidgetContainer::Draw();

        if (!modalStack.isEmpty())
        {
            iterate2 (i, modalStack)
            {
                if (i.getIndex() == modalStack.getLength() - 1)
                {
                    // TODO

                    //zr::R::SetTexture(nullptr);
                    //zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), RGBA_WHITE_A(0.8f));
                }

                i->Draw();
            }
        }
    }

    Widget* UIContainer::Find(const char* widgetName)
    {
        if (!modalStack.isEmpty())
        {
            reverse_iterate2 (i, modalStack)
            {
                Widget* widget = i->Find(widgetName);

                if (widget != nullptr)
                    return widget;
            }
        }

        return WidgetContainer::Find(widgetName);
    }

    int UIContainer::HandleMessage(int h, MessageHeader* msg)
    {
        switch (msg->type)
        {
            case EVENT_MOUSE_MOVE:
            {
                auto payload = msg->Data<EventMouseMove>();

                return this->OnMouseMove(h, payload->x, payload->y);
            }

            case EVENT_UNICODE_INPUT:
            {
                auto payload = msg->Data<EventUnicodeInput>();

                return this->OnCharacter(h, payload->c);
            }

            case EVENT_VKEY:
            {
                auto payload = msg->Data<EventVkey>();
                int button;
                bool pressed;

                if (Vkey::IsMouseButtonEvent(payload->input, button, pressed))
                {
                    int x = mousePos.x;
                    int y = mousePos.y;

                    return this->OnMouseButton(h, button, pressed, x, y);
                }

                break;
            }

            case EVENT_WINDOW_RESIZE:
            {
                auto payload = msg->Data<EventWindowResize>();

                this->SetArea(Int3(), Int2(payload->width, payload->height));
                break;
            }

            case gameui::EVENT_LOOP_EVENT:
                this->OnLoopEvent(msg->Data<gameui::EventLoopEvent>());
                break;

            default:
                ;
        }

        return h;
    }

    int UIContainer::OnCharacter(int h, UnicodeChar c)
    {
        if (!modalStack.isEmpty())
        {
            reverse_iterate2 (i, modalStack)
            {
                if ((h = i->OnCharacter((i.getIndex() == modalStack.getLength() - 1) ? h : 0, c)) >= 1)
                    return h;
            }

            return WidgetContainer::OnCharacter(0, c);
        }
        else
            return WidgetContainer::OnCharacter(h, c);
    }

    void UIContainer::OnFrame(double delta)
    {
        reverse_iterate2 (i, modalStack)
            i->OnFrame(delta);

        WidgetContainer::OnFrame(delta);
    }

    void UIContainer::OnLoopEvent(EventLoopEvent* payload)
    {
        switch (payload->type)
        {
            case EventLoopEvent::FOCUS_WIDGET_IN_CONTAINER:
                payload->container->DoFocusWidget(payload->widget);
                break;

            case EventLoopEvent::POP_MODAL:
                if (payload->flags & EventLoopEvent::FLAG_RELEASE)
                    delete modalStack.getFromEnd();

                modalStack.removeFromEnd(0, 1);
                break;

            case EventLoopEvent::REMOVE_WIDGET_FROM_CONTAINER:
                payload->container->DoRemoveWidget(payload->widget);

                if (payload->flags & EventLoopEvent::FLAG_RELEASE)
                    delete payload->widget;
                break;
        }
    }

    int UIContainer::OnMouseButton(int h, int button, bool pressed, int x, int y)
    {
        switch (button)
        {
            case MOUSEBTN_LEFT:     return OnMousePrimary(h, x, y, pressed);
            default:                return h;
        }
    }

    int UIContainer::OnMouseMove(int h, int x, int y)
    {
        mousePos = Int2(x, y);

        if (!modalStack.isEmpty())
        {
            reverse_iterate2 (i, modalStack)
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
            reverse_iterate2 (i, modalStack)
            {
                if ((h = i->OnMousePrimary((i.getIndex() == modalStack.getLength() - 1) ? h : 0, x, y, pressed)) >= 1)
                    return h;
            }

            return WidgetContainer::OnMousePrimary(0, x, y, pressed);
        }
        else
            return WidgetContainer::OnMousePrimary(h, x, y, pressed);
    }

    void UIContainer::PopModal(bool release)
    {
        MessageQueue* msgQueue = Widget::GetMessageQueue();

        MessageConstruction<EventLoopEvent> msg(msgQueue);
        msg->type = EventLoopEvent::POP_MODAL;
        msg->flags = release ? EventLoopEvent::FLAG_RELEASE : 0;
        msg->container = this;
    }

    /*void UIContainer::PushMessageBox(const char* message)
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
    */
    void UIContainer::PushModal(Widget* modal)
    {
        modal->SetArea(ctnAreaPos, ctnAreaSize);
        modal->SetParentContainer(this);
        modalStack.add(modal);
    }

    bool UIContainer::RestoreUIState(const char* path)
    {
        unique_ptr<InputStream> input(sys->OpenInput(path));

        if (input == nullptr)
            return false;

        for (;;)
        {
            String name = input->readString();

            if (name.isEmpty())
                break;

            Int3 pos;
            Int2 size;

            input->read(&pos, sizeof(pos));
            input->read(&size, sizeof(size));

            auto wnd = FindWidget<Window>(name);

            if (wnd != nullptr)
            {
                wnd->SetPos(pos);

                if (size.x >= wnd->GetMinSize().x && size.y >= wnd->GetMinSize().y)
                {
                    // TODO: This may not be enough without an ui->SetArea() following it
                    wnd->SetSize(size);
                }
            }
        }

        return true;
    }

    bool UIContainer::SaveUIState(const char* path)
    {
        unique_ptr<OutputStream> output(sys->OpenOutput(path));

        if (output == nullptr)
            return false;

        iterate2 (i, widgets)
        {
            if (dynamic_cast<Window*>(*i) != nullptr)
            {
                auto wnd = static_cast<Window*>(*i);

                if (Util::StrEmpty(wnd->GetName()))
                    continue;

                output->writeString(wnd->GetName());

                const Int3 pos = wnd->GetPos();
                const Int2 size = wnd->GetSize();

                output->write(&pos, sizeof(pos));
                output->write(&size, sizeof(size));
            }
        }

        return true;
    }

    void UIContainer::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;

        WidgetContainer::CtnSetArea(pos, size);
    }
}
