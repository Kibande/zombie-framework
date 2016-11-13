
#include "gameui.hpp"

#include "../framework/event.hpp"

namespace gameui
{
    static CInt2 default_padding(12, 12);

    Window::Window( UIResManager* res, const char* title, int flags)
        : Panel(res), painter(res, 1), title(title), flags(flags)
    {
        padding = default_padding;
    }

    Window::Window( UIResManager* res, const char* title, int flags, glm::ivec2 pos, glm::ivec2 size )
        : Panel(res, pos, size), painter(res, 1), title(title), flags(flags)
    {
        padding = default_padding;
    }

    void Window::Close()
    {
        li_tryCall(parentContainer, PushCommand(Command(Command::ID_REMOVE_WIDGET, this)));
    }

    void Window::Draw()
    {
        if (!visible)
            return;

        painter.DrawWindow(pos, size);

        WidgetContainer::Draw();

        //if ( closeButton )
        //    graphicsDriver->drawRectangle( realPos + Vector2<float>( realSize.x - closeButtonSize.x - closeButtonPadding.x, - closeButtonSize.y - closeButtonPadding.y ),
        //            closeButtonSize, Colour( 0.9f, 0.0f, 0.0f, 0.9f ), nullptr );

        if (flags & Window::RESIZABLE)
        {
            zr::R::SetTexture(nullptr);
            render::R::SetBlendColour(COLOUR_GREY(0.6f, 0.6f));
            render::R::DrawRectangle(glm::vec2(pos.x + size.x - 12, pos.y + size.y - 12), glm::vec2(pos.x + size.x - 4, pos.y + size.y - 4), 0.0f);
        }

        if (flags & Window::HEADER)
            painter.DrawWindowTitle(pos, size, title);
    }
    
    void Window::Focus()
    {
        li_tryCall(parentContainer, PushCommand(Command(Command::ID_BUMP_WIDGET, this)));
    }

    void Window::OnFrame(double delta)
    {
        if (visible)
        {
        for (int i = Event::GetCount() - 1; i >= 0; i--)
        {
            Event_t* ev = Event::GetCached(i);

            if (ev->type == EV_MOUSE_BTN && (ev->mouse.flags & VKEY_PRESSED))
            {
                int x = ev->mouse.x;
                int y = ev->mouse.y;

                if (x >= pos.x && y >= pos.y - painter.GetTitleHeight() && x < pos.x + size.x && y < pos.y)
                {
                    Focus();

                    if (ev->mouse.button == MOUSE_BTN_LEFT)
                    {
                        dragFrom = glm::ivec2(x, y);
                        flags |= DRAG;
                        Event::RemoveCached(i);
                    }
                }
                else if (x >= pos.x + size.x - 12 && y >= pos.y + size.y - 12 && x < pos.x + size.x - 4 && y < pos.y + size.y - 4)
                {
                    Focus();

                    if (ev->mouse.button == MOUSE_BTN_LEFT)
                    {
                        dragFrom = glm::ivec2(x, y);
                        flags |= RESIZE;
                        Event::RemoveCached(i);
                    }
                }
                else if (parent != nullptr && x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y)
                    Focus();
            }
            else if (ev->type == EV_MOUSE_BTN && (ev->mouse.flags & VKEY_RELEASED))
            {
                if (ev->mouse.button == MOUSE_BTN_LEFT)
                    flags &= ~(DRAG | RESIZE);
            }
            else if (ev->type == EV_MOUSE_MOVE)
            {
                glm::ivec2 mouse(ev->mouse.x, ev->mouse.y);

                if (flags & DRAG)
                {
                    Move(mouse - dragFrom );
                    dragFrom = mouse;
                }
                else if (flags & RESIZE)
                {
                    glm::ivec2 minSize = GetMinSize();

                    if (size.x + mouse.x - dragFrom.x >= minSize.x)
                    {
                        size.x += mouse.x - dragFrom.x;
                        dragFrom.x = mouse.x;
                    }

                    if (size.y + mouse.y - dragFrom.y >= minSize.y)
                    {
                        size.y += mouse.y - dragFrom.y;
                        dragFrom.y = mouse.y;
                    }

                    Layout();
                }
            }
        }
        }

        Panel::OnFrame(delta);
    }

    void Window::ReloadMedia()
    {
        painter.AcquireResources();

        Panel::ReloadMedia();
    }
}