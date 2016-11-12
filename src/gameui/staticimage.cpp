
#include "gameui.hpp"

#include <framework/event.hpp>
#include <framework/framework.hpp>

namespace gameui
{
    StaticImage::StaticImage(UIThemer* themer, const char* path)
    {
        thm.reset(themer->CreateStaticImageThemer(pos, size, path));
    }

    StaticImage::~StaticImage()
    {
    }

    void StaticImage::Draw()
    {
        if (!visible)
            return;

        thm->Draw();
    }

    void StaticImage::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    void StaticImage::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);
    }

    int StaticImage::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (pressed && h < 0 && IsInsideMe(x, y))
        {
            FireMouseDownEvent(MOUSEBTN_LEFT, x, y);
            return 1;
        }

        return h;
    }
}
