
#include "gameui.hpp"

namespace gameui
{
    static CInt2 default_padding(2, 2);

    Panel::Panel(UIResManager* res)
        : Widget(res), painter(res), minSizeValid(false)
    {
        padding = default_padding;
        transparent = false;
    }
    
    Panel::Panel(UIResManager* res, Int2 pos, Int2 size)
        : Widget(res, pos, size), painter(res), minSizeValid(false)
    {
        padding = default_padding;
        transparent = false;
    }

    void Panel::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        widget->SetParent(this);
        WidgetContainer::Add(widget);

        widget->SetArea(pos + padding, size - padding * 2);
        minSizeValid = false;
    }

    void Panel::Draw()
    {
        if (!visible)
            return;

        if (!transparent)
            painter.Draw(pos, size);

        WidgetContainer::Draw();
    }

    Int2 Panel::GetMinSize()
    {
        if (!minSizeValid)
        {
            minSize = Int2();

            iterate2 (widget, widgets)
                if (!widget->GetFreeFloat())
                    minSize = glm::max(minSize, widget->GetMinSize());

            minSize += padding * 2;
            minSizeValid = true;
        }

        return minSize;
    }

    void Panel::Layout()
    {
        WidgetContainer::SetArea(pos + padding, size - padding * 2);
        minSizeValid = false;
    }

    void Panel::Move(Int2 vec)
    {
        Widget::Move(vec);
        WidgetContainer::Move(vec);
    }

    int Panel::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (!visible)
            return h;

        if (IsInsideMe(x, y))
        {
            h = WidgetContainer::OnMousePrimary(h, x, y, pressed);
            return transparent ? h : 1;
        }
        else
        {
            WidgetContainer::OnMousePrimary(0, x, y, pressed);
            return h;
        }
    }

    void Panel::SetArea(Int2 pos, Int2 size)
    {
        Widget::SetArea(pos, size);
        Layout();
    }
}