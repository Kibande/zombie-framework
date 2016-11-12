
#include "gameui.hpp"

namespace gameui
{
    ScrollBarV::ScrollBarV(UIThemer* themer)
        : y(0), h(0), ydiff(0), dragging(false), yOrig(0)
    {
        thm.reset(themer->CreateScrollBarThemer(false));
    }
    
    void ScrollBarV::Draw()
    {
        thm->DrawVScrollBar(pos, size, y, h);
    }
    
    int ScrollBarV::OnMouseMove(int h, int x, int y)
    {
        if (dragging)
        {
            ydiff = this->y;
            this->y = glm::max(glm::min(this->y + y - yOrig, size.y - this->h - 2 * scrollbar_pad), 0);
            ydiff -= this->y;
            yOrig = y;
            return 0;
        }
        
        return h;
    }
    
    int ScrollBarV::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (dragging)
            dragging = false;
        else if (h < 0 && x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y)
        {
            dragging = true;
            yOrig = y;
            return 1;
        }
        
        return h;
    }
}
