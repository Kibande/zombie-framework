
#include "gameui.hpp"

#include <framework/framework.hpp>

namespace gameui
{
    CheckBox::CheckBox(UIThemer* themer, const char* label, bool checked)
            : checked(checked)
    {
        thm.reset(themer->CreateCheckBoxThemer(pos, size, label, checked));
    }

    CheckBox::CheckBox(UIThemer* themer, const char* label, bool checked, Int3 pos, Int2 size)
            : Widget(pos, size), checked(checked)
    {
        thm.reset(themer->CreateCheckBoxThemer(pos, size, label, checked));
    }

    CheckBox::~CheckBox()
    {
    }

    bool CheckBox::AcquireResources()
    {
        if (!thm->AcquireResources())
            return false;

        minSize = thm->GetMinSize();
        return true;
    }

    void CheckBox::Draw()
    {
        if (!visible)
            return;

        thm->Draw();
    }

    void CheckBox::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    void CheckBox::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);
    }

    void CheckBox::OnFrame(double delta)
    {
        //painter.Update(delta);
    }

    int CheckBox::OnMouseMove(int h, int x, int y)
    {
        if (h < 0 && IsInsideMe(x, y))
        {
            thm->SetMouseOver(true);
            h = 0;
        }
        else
            thm->SetMouseOver(false);
        
        return h;
    }

    int CheckBox::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0 && IsInsideMe(x, y))
        {
            if (pressed)
            {
                checked = !checked;

                thm->SetChecked(checked);
                FireValueChangeEvent(checked ? 1 : 0);
            }
            else
                FireClickEvent(x, y);

            return 1;
        }

        return h;
    }

    void CheckBox::SetChecked(bool checked)
    {
        this->checked = checked;

        thm->SetChecked(checked);
    }
}
