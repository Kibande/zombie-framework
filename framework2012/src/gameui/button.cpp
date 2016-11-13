
#include "gameui.hpp"

namespace gameui
{
    Button::Button(UIResManager* res, const char* label)
            : Widget(res), painter(res, label, 0)
    {
    }

    Button::Button(UIResManager* res, const char* label, glm::ivec2 pos, glm::ivec2 size)
            : Widget(res, pos, size), painter(res, label, 0)
    {
    }

    void Button::Draw()
    {
        painter.Draw(pos, size);
    }

    void Button::OnFrame(double delta)
    {
        painter.Update(delta);
    }

    int Button::OnMouseMove(int h, int x, int y)
    {
        if (h < 0 && IsInsideMe(x, y))
        {
            painter.SetMouseOver(true);
            h = 0;
        }
        else
            painter.SetMouseOver(false);
        
        return h;
    }

    int Button::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0 && IsInsideMe(x, y))
        {
            if (pressed)
                FireMouseDownEvent(x, y);
            else
                FireClickEvent(x, y);

            return 1;
        }

        return h;
    }

    void Button::ReloadMedia()
    {
        painter.GetMinSize(minSize);
    }
}