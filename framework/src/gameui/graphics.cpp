
#include <gameui/gameui.hpp>

#include <framework/graphics.hpp>

namespace gameui
{
    Graphics::Graphics(UIThemer* themer, shared_ptr<IGraphics> g)
            : g(g)
    {
        thm.reset(themer->CreateGraphicsThemer(g));

        wasMouseInsideMe = false;

        SetExpands(false);
    }

    Graphics::~Graphics()
    {
    }

    void Graphics::Draw()
    {
        thm->Draw();
    }

    Int2 Graphics::GetMinSize()
    {
        return Int2(g->GetSize());
    }

    void Graphics::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    int Graphics::OnMouseMove(int h, int x, int y)
    {
        if (visible)
        {
            bool isMouseInsideMe = (h <= h_indirect) && IsInsideMe(x, y);

            if (!wasMouseInsideMe && isMouseInsideMe)
                g->SelectState("hover");
            else if (wasMouseInsideMe && !isMouseInsideMe)
                g->SelectState(nullptr);

            wasMouseInsideMe = isMouseInsideMe;
        }

        return h;
    }

    int Graphics::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < h_direct && visible)
        {
            if (pressed)
            {
            }
        }

        return h;
    }
}
