
#include "gameui.hpp"

namespace gameui
{
    static const Int2 default_padding(12, 12);

    enum {
        DRAGGING = 1,
        RESIZING = 2
    };

    Window::Window(UIThemer* themer, const char* title, int flags)
            : Panel(themer)
    {
        wthm = themer->CreateWindowThemer(thm.get(), flags);
        wthm->SetTitle(title);

        this->flags = flags;
        state = 0;

        padding = default_padding;

        SetExpands(false);
    }

    Window::Window(UIThemer* themer, const char* title, int flags, Int3 pos, Int2 size)
            : Panel(themer, pos, size)
    {
        wthm = themer->CreateWindowThemer(thm.get(), flags);
        wthm->SetTitle(title);

        this->flags = flags;
        state = 0;

        padding = default_padding;

        SetExpands(false);
    }

    bool Window::AcquireResources()
    {
        if (!Panel::AcquireResources())
            return false;

        titleSize = wthm->GetTitleMinSize();
        return true;
    }

    void Window::Close()
    {
        if (parentContainer != nullptr)
            parentContainer->DelayedRemoveWidget(this, false);
    }

    void Window::Focus()
    {
        if (parentContainer != nullptr)
            parentContainer->DelayedFocusWidget(this);
    }

    Int2 Window::GetMinSize()
    {
        Int2 minSize = Panel::GetMinSize();

        if (minSize.x < titleSize.x)
            minSize.x = titleSize.x;

        return minSize;
    }

    int Window::OnMouseMove(int h, int x, int y)
    {
        if (state & DRAGGING)
        {
            Move(Int3(x - dragFrom.x, y - dragFrom.y, 0));
            dragFrom = Int2(x, y);
            return 0;
        }
        else if (state & RESIZING)
        {
            Int2 minSize = GetMinSize();

            if (size.x + x - dragFrom.x >= minSize.x)
            {
                size.x += x - dragFrom.x;
                dragFrom.x = x;
            }

            if (size.y + y - dragFrom.y >= minSize.y)
            {
                size.y += y - dragFrom.y;
                dragFrom.y = y;
            }

            SetArea(pos, size);
            return 0;
        }
        else
            return Panel::OnMouseMove(h, x, y);
    }

    int Window::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (!visible)
            return h;

        if (pressed)
        {
            if (x >= pos.x && y >= pos.y - titleSize.y && x < pos.x + size.x && y < pos.y)
            {
                Focus();

                //if (ev->mouse.button == MOUSE_BTN_LEFT)
                if (h <= h_indirect)
                {
                    dragFrom = Int2(x, y);
                    state |= DRAGGING;
                }

                return Panel::OnMousePrimary(0, x, y, pressed);
            }
            else if (x >= pos.x + size.x - 12 && y >= pos.y + size.y - 12 && x < pos.x + size.x - 4 && y < pos.y + size.y - 4)
            {
                Focus();

                //if (ev->mouse.button == MOUSE_BTN_LEFT)
                if (h <= h_indirect)
                {
                    dragFrom = Int2(x, y);
                    state |= RESIZING;
                }

                return Panel::OnMousePrimary(0, x, y, pressed);
            }
            else if (parentContainer != nullptr && x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y)
                Focus();
        }
        else
        {
            //if (ev->mouse.button == MOUSE_BTN_LEFT)
                state &= ~(DRAGGING | RESIZING);
        }

        return Panel::OnMousePrimary(h, x, y, pressed);
    }
}
