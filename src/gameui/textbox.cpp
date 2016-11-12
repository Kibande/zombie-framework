
#include "gameui.hpp"

#include <framework/event.hpp>
#include <framework/framework.hpp>

namespace gameui
{
    TextBox::TextBox(UIThemer* themer)
    {
        thm.reset(themer->CreateTextBoxThemer(pos, size));

        active = false;
        cursorPos = 0;
    }

    TextBox::~TextBox()
    {
    }
    
    void TextBox::Draw()
    {
        if (!visible)
            return;

        thm->Draw();
    }

    void TextBox::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    void TextBox::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);
    }

    int TextBox::OnCharacter(int h, UnicodeChar c)
    {
        if (active && h < 0)
        {
            cursorPos = thm->InsertCharacter(cursorPos, c);

            if (thm->GetMinSize() != minSize && parentContainer != nullptr)
                parentContainer->OnWidgetMinSizeChange(this);

            return 0;
        }

        return h;
    }

    int TextBox::OnMouseMove(int h, int x, int y)
    {
        if (h < 0 && IsInsideMe(x, y))
        {
            thm->SetMouseOver(true);
            return 0;
        }
        else
            thm->SetMouseOver(false);
        
        return h;
    }

    int TextBox::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (IsInsideMe(x, y))
        {
            if (h < 0)
            {
                if (pressed)
                {
                    thm->SetActive((active = true));
                    thm->SetCursorPos(Int3(x, y, pos.z), &cursorPos);
                }

                return h_direct;
            }
        }
        else
            thm->SetActive((active = false));

        return h;
    }

    void TextBox::SetText(const char* text)
    {
        thm->SetText(text);
    }
}
