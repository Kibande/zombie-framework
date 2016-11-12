
#include "gameui.hpp"

#include <framework/framework.hpp>

namespace gameui
{
    static const Int2 default_padding(2, 2);

    Panel::Panel(UIThemer* themer)
        : minSizeValid(false), layoutValid(false)
    {
        thm.reset(themer->CreatePanelThemer(pos, size));

        padding = default_padding;
        transparent = false;
    }
    
    Panel::Panel(UIThemer* themer, Int3 pos, Int2 size)
        : Widget(pos, size), minSizeValid(false), layoutValid(false)
    {
        thm.reset(themer->CreatePanelThemer(pos, size));

        padding = default_padding;
        transparent = false;
    }

    Panel::~Panel()
    {
    }

    bool Panel::AcquireResources()
    {
        minSizeValid = false;

        return thm->AcquireResources() && WidgetContainer::AcquireResources();
    }

    void Panel::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        widget->SetParent(this);
        WidgetContainer::Add(widget);

        if (ctnOnlineUpdate)
        {
            if (minSizeValid)
                OnlineUpdateForWidget(widget);
        }
        else
            minSizeValid = false;
    }

    void Panel::Draw()
    {
        if (!visible)
            return;

        if (!transparent)
            thm->Draw();

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
        if (layoutValid)
            return;

        Widget::Layout();
        thm->SetArea(pos, size);

        CtnSetArea(pos + Int3(padding, 0), size - 2 * padding);

        layoutValid = true;
    }

    void Panel::Move(Int3 vec)
    {
        Widget::Move(vec);
        WidgetContainer::Move(vec);

        thm->SetArea(pos, size);
    }

    void Panel::OnlineUpdateForWidget(Widget* widget)
    {
        if (!widget->GetFreeFloat())
        {
            const Int2 widgetMinSize = widget->GetMinSize();

            if (!expands
                    || (widgetMinSize.x + padding.x * 2 > minSize.x || widgetMinSize.y + padding.y * 2 > minSize.y))
            {
                minSize = widgetMinSize + padding * 2;

                layoutValid = false;

                // Notify the parent, as it might need to resize itself
                if (parentContainer != nullptr)
                    parentContainer->OnWidgetMinSizeChange(this);
            }
        }
    }

    int Panel::OnCharacter(int h, UnicodeChar c)
    {
        if (!visible)
            return h;

        return WidgetContainer::OnCharacter(h, c);
    }

    int Panel::OnMouseMove(int h, int x, int y)
    {
        if (!visible)
            return h;

        if (IsInsideMe(x, y))
        {
            h = WidgetContainer::OnMouseMove(h, x, y);
            return transparent ? h : 1;
        }
        else
        {
            WidgetContainer::OnMouseMove(0, x, y);
            return h;
        }
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

    void Panel::OnWidgetMinSizeChange(Widget* widget)
    {
        if (ctnOnlineUpdate)
        {
            if (minSizeValid)
                OnlineUpdateForWidget(widget);
        }
        else
            minSizeValid = false;
    }

    void Panel::SetArea(Int3 pos, Int2 size)
    {
        if (size == areaSize)
        {
            Move(pos - areaPos);
            areaPos = pos;
        }
        else
            layoutValid = false;

        Widget::SetArea(pos, size);
    }

    void Panel::SetColour(const Float4& colour)
    {
        thm->SetColour(colour);
    }

    void Panel::SetSize(Int2 size)
    {
        this->size = size;

        if (size.x != areaSize.x || size.y != areaSize.y)
        {
            layoutValid = false;
            Layout();
        }
    }
}
