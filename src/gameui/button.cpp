
#include <gameui/gameui.hpp>

#include <framework/event.hpp>
#include <framework/framework.hpp>

namespace gameui
{
    Button::Button(UIThemer* themer, const char* label)
    {
        thm.reset(themer->CreateButtonThemer(pos, size, label));
    }

    Button::Button(UIThemer* themer, const char* label, Int3 pos, Int2 size)
            : Widget(pos, size)
    {
        thm.reset(themer->CreateButtonThemer(pos, size, label));
    }

    Button::~Button()
    {
    }

    bool Button::AcquireResources()
    {
        if (!thm->AcquireResources())
            return false;

        minSize = thm->GetMinSize();
        return true;
    }

    void Button::Draw()
    {
        if (!visible)
            return;

        thm->Draw();
    }

    void Button::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    void Button::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);
    }

    void Button::OnFrame(double delta)
    {
        //painter.Update(delta);
    }

    int Button::OnMouseMove(int h, int x, int y)
    {
        if (h < 0 && visible && IsInsideMe(x, y))
        {
            thm->SetMouseOver(true);
            return 0;
        }
        else
            thm->SetMouseOver(false);
        
        return h;
    }

    int Button::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0 && visible && IsInsideMe(x, y))
        {
            if (pressed)
                FireMouseDownEvent(MOUSEBTN_LEFT, x, y);
            else
                FireClickEvent(x, y);

            return 1;
        }

        return h;
    }

    void Button::SetLabel(const char* label)
    {
        thm->SetLabel(label);

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);
    }
}
