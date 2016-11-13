
#include "gameui.hpp"

#include "../framework/event.hpp"

namespace gameui
{
    TextBox::TextBox(UIResManager* res) : Widget(res), painter(res, 0)
    {
    }

    void TextBox::Draw()
    {
    }
}