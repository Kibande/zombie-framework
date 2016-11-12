
#include "gameui.hpp"

namespace gameui
{
    ListCtrl::ListCtrl(UIThemer* themer)
        : spacing(4), rebuildNeeded(true), scrollbar(themer)
    {
        thm.reset(themer->CreateListBoxThemer());
    }

    bool ListCtrl::AcquireResources()
    {
        rebuildNeeded = true;
        return thm->AcquireResources() && WidgetContainer::AcquireResources();
    }

    void ListCtrl::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        WidgetContainer::Add(widget);
        widget->SetParent(this);
        rebuildNeeded = true;
    }

    void ListCtrl::Draw()
    {
        //painter.BeginClip(pos, size);
        
        WidgetContainer::Draw();
        
        if (contents_h > size.y)
            scrollbar.Draw();

        //painter.EndClip();
    }

    Int2 ListCtrl::GetMinSize()
    {
        if (rebuildNeeded)
        {
            contents_w = 0;
            contents_h = 0;
            rowHeights.resize(widgets.getLength());

            for (size_t i = 0; i < widgets.getLength(); i++)
            {
                const Int2 ms = widgets[i]->GetMinSize();
                rowHeights[i] = ms.y;

                contents_w = glm::max(contents_w, ms.x);
                contents_h += ms.y;

                if (i + 1 < widgets.getLength())
                    contents_h += spacing;
            }

            rebuildNeeded = false;
        }

        //return glm::max(minSize, glm::ivec2(contents_w + ((contents_h > size.y) ? (scrollbar_w + 4) : 0), 0));
        return glm::max(minSize, glm::ivec2(contents_w + scrollbar.GetWidth() + 4, 0));
    }

    void ListCtrl::Layout()
    {
        // TODO: Use a special method to perform contents_y recalc when necessary

        if (contents_h > size.y)
        {
            scrollbar.pos = Int2(pos.x + size.x - scrollbar.GetWidth(), pos.y);
            scrollbar.size = Int2(scrollbar.GetWidth(), size.y);
            scrollbar.h = (int)(glm::min((float)size.y / contents_h, 1.0f) * (size.y - 2 * scrollbar_pad));
            scrollbar.y = glm::max(glm::min(scrollbar.y, contents_h - size.y), 0);
        }

        int y = 0;

        int inside_w = size.x - ((contents_h > size.y) ? (scrollbar.GetWidth() + 4) : 0);

        for (size_t i = 0; i < widgets.getLength(); i++)
        {
            widgets[i]->SetArea(Int3(pos.x, pos.y + y, pos.z), Int2(inside_w, rowHeights[i]));
            y += rowHeights[i];

            if (i + 1 < widgets.getLength())
                y += spacing;
        }
    }

    void ListCtrl::Move(Int3 vec)
    {
        Widget::Move(vec);
        WidgetContainer::Move(vec);

        scrollbar.pos += Int2(vec);
    }

    int ListCtrl::OnMouseMove(int h, int x, int y)
    {
        if ((h = scrollbar.OnMouseMove(h, x, y)) >= 1)
            return h;
        
        if (scrollbar.ydiff != 0)
        {
            WidgetContainer::Move(Int3(0, scrollbar.ydiff * contents_h / scrollbar.size.y, 0));
        }
        
        return WidgetContainer::OnMouseMove(h, x, y);
    }
    
    int ListCtrl::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (!visible)
            return h;

        if ((h = scrollbar.OnMousePrimary(h, x, y, pressed)) >= 1)
            return h;

        if (IsInsideMe(x, y))
        {
            WidgetContainer::OnMousePrimary(h, x, y, pressed);
            return 1;
        }
        else
        {
            WidgetContainer::OnMousePrimary(0, x, y, pressed);
            return h;
        }
    }

    void ListCtrl::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;

        Layout();
    }
}
