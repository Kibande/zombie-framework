
#include "gameui.hpp"

namespace gameui
{
    Popup::Popup(UIThemer* themer)
            : Window(themer, "", 0)
    {
        easyDismiss = false;
    }

    int Popup::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (easyDismiss && parentContainer != nullptr
                && (x < pos.x || y < pos.y || x >= pos.x + size.x || y >= pos.y + size.y))
        {
            parentContainer->DelayedRemoveWidget(this, true);
            return h;
        }

        return Window::OnMousePrimary(h, x, y, pressed);
    }
}
