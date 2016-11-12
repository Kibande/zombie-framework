
#include "gameui.hpp"

#include <framework/messagequeue.hpp>

namespace gameui
{
    WidgetContainer::WidgetContainer()
    {
        ctnOnlineUpdate = true;

        numTopLevelWidgets = 0;
    }

    WidgetContainer::~WidgetContainer()
    {
        iterate2 (widget, widgets)
            delete widget;
    }

    bool WidgetContainer::AcquireResources()
    {
        iterate2 (widget, widgets)
            if (!widget->AcquireResources())
                return false;

        return true;
    }

    void WidgetContainer::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        widget->SetParentContainer(this);
        widgets.add(widget);

        if (ctnOnlineUpdate)
            widget->SetArea(ctnAreaPos, ctnAreaSize);

        if (dynamic_cast<MenuBar*>(widget) != nullptr)
            numTopLevelWidgets++;
    }

    void WidgetContainer::CtnSetArea(Int3 pos, Int2 size)
    {
        ctnAreaPos = pos;
        ctnAreaSize = size;

        iterate2 (widget, widgets)
            widget->SetArea(pos, size);
    }

    void WidgetContainer::DelayedFocusWidget(Widget* widget)
    {
        if (widgets.getLength() != 0 && widget == widgets.getFromEnd())
            return;

        MessageQueue* msgQueue = Widget::GetMessageQueue();

        MessageConstruction<EventLoopEvent> msg(msgQueue);
        msg->type = EventLoopEvent::FOCUS_WIDGET_IN_CONTAINER;
        msg->flags = 0;
        msg->widget = widget;
        msg->container = this;
    }

    void WidgetContainer::DelayedRemoveWidget(Widget* widget, bool release)
    {
        MessageQueue* msgQueue = Widget::GetMessageQueue();

        MessageConstruction<EventLoopEvent> msg(msgQueue);
        msg->type = EventLoopEvent::REMOVE_WIDGET_FROM_CONTAINER;
        msg->flags = release ? EventLoopEvent::FLAG_RELEASE : 0;
        msg->widget = widget;
        msg->container = this;
    }

    void WidgetContainer::DoFocusWidget(Widget* widget)
    {
        if (widgets.removeItem(widget))
            widgets.insert(widget, widgets.getLength() - numTopLevelWidgets);
    }

    bool WidgetContainer::DoRemoveWidget(Widget* widget)
    {
        if (widgets.removeItem(widget))
        {
            if (dynamic_cast<MenuBar*>(widget) != nullptr)
                numTopLevelWidgets++;

            return true;
        }
        else
            return false;
    }

    void WidgetContainer::Draw()
    {
        iterate2 (widget, widgets)
            widget->Draw();
    }

    Widget* WidgetContainer::Find(const char* widgetName)
    {
        iterate2 (widget, widgets)
            if (widget->GetNameString() == widgetName)
                return widget;

        iterate2 (widget, widgets)
        {
            Widget *w;

            if ((w = widget->Find(widgetName)) != nullptr)
                return w;
        }

        return nullptr;
    }

    void WidgetContainer::Layout()
    {
        iterate2 (widget, widgets)
            widget->SetArea(ctnAreaPos, ctnAreaSize);
    }

    void WidgetContainer::Move(Int3 vec)
    {
        iterate2 (widget, widgets)
            widget->Move(vec);
    }

    void WidgetContainer::OnFrame(double delta)
    {
        reverse_iterate2 (widget, widgets)
            widget->OnFrame(delta);
    }

    int WidgetContainer::OnCharacter(int h, UnicodeChar c)
    {
        reverse_iterate2 (widget, widgets)
        {
            if ((h = widget->OnCharacter(h, c)) >= 1)
                return h;
        }

        return h;
    }

    int WidgetContainer::OnMouseMove(int h, int x, int y)
    {
        reverse_iterate2 (widget, widgets)
        {
            if ((h = widget->OnMouseMove(h, x, y)) >= 1)
                return h;
        }

        return h;
    }

    int WidgetContainer::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        reverse_iterate2 (widget, widgets)
        {
            if ((h = widget->OnMousePrimary(h, x, y, pressed)) >= 1)
                return h;
        }

        return h;
    }

    void WidgetContainer::RemoveAll()
    {
        reverse_iterate2 (widget, widgets)
        {
            delete widget;
            widgets.removeItem(widget);
        }
    }
}
