
#include "gameui.hpp"

namespace gameui
{
    WidgetContainer::WidgetContainer()
    {
    }

    WidgetContainer::~WidgetContainer()
    {
        iterate2 (widget, widgets)
            delete widget;
    }

    void WidgetContainer::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        widget->SetParentContainer(this);
        widgets.add(widget);
    }

    void WidgetContainer::Draw()
    {
        iterate2 (widget, widgets)
            widget->Draw();
    }

    Widget* WidgetContainer::Find(const char* widgetName)
    {
        iterate2 (widget, widgets)
            if (widget->GetName() == widgetName)
                return widget;

        iterate2 (widget, widgets)
        {
            Widget *w;

            if ((w = widget->Find(widgetName)) != nullptr)
                return w;
        }

        return nullptr;
    }

    void WidgetContainer::Move(glm::ivec2 vec)
    {
        iterate2 (widget, widgets)
            widget->Move(vec);
    }

    void WidgetContainer::OnFrame(double delta)
    {
        reverse_iterate2 (widget, widgets)
            widget->OnFrame(delta);

        iterate2 (i, commands)
        {
            auto cmd = *i;

            if(cmd.cmd_id == Command::ID_REMOVE_WIDGET)
            {
                if (widgets.removeItem(cmd.data.widget_ptr))
                    delete cmd.data.widget_ptr;
            }
            else if(cmd.cmd_id == Command::ID_BUMP_WIDGET)
            {
                if (widgets.removeItem(cmd.data.widget_ptr))
                    widgets.add(cmd.data.widget_ptr);
            }
            else continue;

            i.remove();
        }
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

    void WidgetContainer::Relayout()
    {
        iterate2 (widget, widgets)
            widget->Relayout();
    }

    void WidgetContainer::ReleaseMedia()
    {
        iterate2 (widget, widgets)
            widget->ReleaseMedia();
    }

    void WidgetContainer::ReloadMedia()
    {
        iterate2 (widget, widgets)
            widget->ReloadMedia();
    }

    void WidgetContainer::SetArea(glm::ivec2 pos, glm::ivec2 size)
    {
        iterate2 (widget, widgets)
            widget->SetArea(pos, size);
    }
}