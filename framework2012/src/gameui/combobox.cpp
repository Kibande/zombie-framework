
#include "gameui.hpp"

namespace gameui
{
    ComboBox::ComboBox(UIResManager* res)
        : Widget(res), painter(res), rebuildNeeded(true)
    {
        is_open = false;
        selection = 0;
        mouseOver = -1;

        padding = painter.GetPadding();
    }

    void ComboBox::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        WidgetContainer::Add(widget);
        widget->SetParent(this);
        rebuildNeeded = true;
    }

    void ComboBox::Draw()
    {
        painter.DrawMainBg(pos, size, is_open);
        widgets[selection]->DrawAt(pos + padding);
        painter.DrawIcon(Int2(pos.x + size.x - padding.x - iconSize, pos.y + padding.y), Int2(iconSize, iconSize), is_open);

        if (is_open)
        {
            painter.DrawContentsBg(Int2(pos.x, pos.y + size.y), Int2(size.x, contentsSize.y + 2 * padding.y));

            if (mouseOver >= 0)
            {
                int y = 0;

                for (size_t i = 0; i < (unsigned int) mouseOver; i++)
                    y += rowHeights[i];

                painter.DrawSelectionBg(Int2(pos.x, pos.y + size.y + padding.y + y), Int2(size.x, rowHeights[mouseOver]));
            }

            WidgetContainer::Draw();
        }
    }

    int ComboBox::GetItemAt(int x, int y)
    {
        int xx = pos.x + padding.x;
        int yy = pos.y + size.y + padding.y;

        for (size_t i = 0; i < widgets.getLength(); i++)
        {
            if (x >= xx && y >= yy && x < xx + contentsSize.x && y < yy + rowHeights[i])
                return i;

            yy += rowHeights[i];
        }

        return -1;
    }

    Int2 ComboBox::GetMaxSize()
    {
        GetMinSize();

        return maxSize;
    }

    Int2 ComboBox::GetMinSize()
    {
        if(rebuildNeeded)
        {
            contentsSize = Int2(0, 0);
            minSize = Int2(0, 0);

            for ( size_t i = 0; i < widgets.getLength(); i++ )
            {
                Int2 itemSize = widgets[i]->GetMinSize();
                rowHeights[i] = itemSize.y;

                if ( itemSize.x > contentsSize.x )
                    contentsSize.x = itemSize.x;

                if ( itemSize.y > minSize.y )
                    minSize.y = itemSize.y;

                contentsSize.y += itemSize.y;
            }

            iconSize = glm::max(minSize.y, painter.GetIconMinSize());
            minSize.y = iconSize;
            minSize.x = contentsSize.x + iconSize;

            minSize += Int2(3 * padding.x, 2 * padding.y);

            maxSize.y = minSize.y;

            rebuildNeeded = false;
        }

        return minSize;
    }

    void ComboBox::Layout()
    {
        // Make sure minSize is up-to-date
        GetMinSize();

        // TODO: Should we care about aligning here?
        //
        //

        int x = pos.x + padding.x;
        int y = pos.y + size.y + padding.y;

        for ( size_t i = 0; i < widgets.getLength(); i++ )
        {
            widgets[i]->SetArea( Int2(x, y), Int2(contentsSize.x, rowHeights[i]) );

            y += rowHeights[i];
        }
    }

    void ComboBox::Move(Int2 vec)
    {
        Widget::Move(vec);
        WidgetContainer::Move(vec);
    }

    int ComboBox::OnMouseMove(int h, int x, int y)
    {
        if (h < 0)
        {
            mouseOver = GetItemAt(x, y);
            WidgetContainer::OnMouseMove(0, x, y);
            return (mouseOver != -1) ? 0 : h;
        }
        else
            mouseOver = -1;

        return h;
    }

    int ComboBox::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0)
            if (IsInsideMe(x, y))
            {
                if (pressed)
                {
                    FireMouseDownEvent(x, y);
                    is_open = !is_open;
                }
                else
                    FireClickEvent(x, y);

                return 1;
            }
            else
            {
                mouseOver = GetItemAt(x, y);
                is_open = false;

                if (mouseOver >= 0)
                {
                    selection = mouseOver;
                    FireItemSelectedEvent(selection);
                    return 1;
                }
            }

        return h;
    }

    void ComboBox::Relayout()
    {
        rebuildNeeded = true;
        Layout();

        WidgetContainer::Relayout();
    }

    void ComboBox::ReloadMedia()
    {
        rebuildNeeded = true;
        WidgetContainer::ReloadMedia();
    }

    void ComboBox::SetArea(Int2 pos, Int2 size)
    {
        Widget::SetArea(pos, size);

        Layout();
    }
}