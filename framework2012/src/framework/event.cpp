
#include "event.hpp"

// TODO: this is used only for UIEvent debugging (we're not supposed to depend on it anyway)
#include "../gameui/gameui.hpp"

#include <littl/Stack.hpp>

namespace zfw
{
    static List<Event_t> evtstack;
    static Event_t tmpevt;

    static const char* btn_names[] =
            { "Up", "Down", "Left", "Right", "A", "B", "X", "Y", "L", "R", "Menu", "Aim Up", "Aim Down", "Aim Left", "Aim Right", "Move Up", "Move Down", "Move Left", "Move Right" };

    static const char* btn_shortNames[] = { "up", "down", "left", "right", "a", "b", "x", "y", "l", "r", "menu", "aup", "adown", "aleft", "aright", "mup", "mdown", "mleft", "mright" };

    const char* Event::FormatVkey(const Vkey_t& vkey)
    {
        if (vkey.inclass == IN_ANALOG || vkey.inclass == IN_JOYBTN || vkey.inclass == IN_JOYHAT)
            return Sys::tmpsprintf(256, "%i:%s:%04X:%04X", vkey.inclass, render::Video::GetJoystickName(vkey.dev), vkey.key, vkey.subkey);

        return Sys::tmpsprintf(64, "%i:%i:%04X:%04X", vkey.inclass, vkey.dev, vkey.key, vkey.subkey);
    }

    const char* Event::GetButtonName( int index )
    {
        return btn_names[index];
    }

    const char* Event::GetButtonShortName( int index )
    {
        return btn_shortNames[index];
    }

    size_t Event::GetCount()
    {
        return evtstack.getLength();
    }

    Event_t* Event::GetCached(size_t index)
    {
        return &evtstack[index];
    }

    void Event::ParseVkey(const char* fmt, Vkey_t& vkey)
    {
        if (fmt == nullptr)
            return;

        int inclass = -1, dev = -1, key = -1, subkey = -1;

        sscanf(fmt, "%i:", &inclass );

        if (inclass == IN_ANALOG || inclass == IN_JOYBTN || inclass == IN_JOYHAT)
        {
            String f(fmt);
            int index, index2;

            if ((index = f.findChar(':')) < 0 || (index2 = f.findChar(':', index + 1)) < 0)
                return;

            dev = render::Video::GetJoystickDev( f.subString(index + 1, index2 - index - 1) );
            sscanf(f.dropLeftPart(index2), ":%X:%X", &key, &subkey );
        }
        else
            sscanf(fmt, "%*i:%i:%X:%X", &dev, &key, &subkey );

        vkey.inclass = inclass;
        vkey.dev = dev;
        vkey.key = key;
        vkey.subkey = subkey;
    }

    void Event::Push( const Event_t* event )
    {
        evtstack.add(*event);
    }

    void Event::PushMouseEvent(EventType event, int x, int y, int flags, int button)
    {
        Event_t ev;

        ev.type = event;
        ev.mouse.x = x;
        ev.mouse.y = y;
        ev.mouse.flags = flags;
        ev.mouse.button = button;

        Push( &ev );
    }

    void Event::PushUIItemEvent(EventType event, gameui::Widget* widget, int itemIndex)
    {
        Event_t ev;

        ev.type = event;
        ev.ui_item.widget = widget;
        ev.ui_item.itemIndex = itemIndex;

        Push(&ev);
    }

    void Event::PushUIMouseEvent(EventType event, gameui::Widget* widget, int x, int y)
    {
        Event_t ev;

        ev.type = event;
        ev.ui_mouse.widget = widget;
        ev.ui_mouse.x = x;
        ev.ui_mouse.y = y;

        Push(&ev);
    }

    void Event::PushUnicodeInEvent( uint32_t cp )
    {
        //printf( "PushUnicodeInEvent: %04X\n", cp );

        Event_t ev;

        ev.type = EV_UNICODE_IN;
        ev.uni_cp = cp;

        Push( &ev );
    }

    void Event::PushViewportResizeEvent(unsigned int w, unsigned int h)
    {
        printf( "PushViewportResizeEvent: %ux%u\n", w, h );

        Event_t ev;

        ev.type = EV_VIEWPORT_RESIZE;
        ev.viewresize.w = w;
        ev.viewresize.h = h;

        Push( &ev );
    }

    void Event::PushVkeyEvent( int16_t inclass, int16_t dev, int16_t key, int16_t subkey, int flags, int value )
    {
        //printf( "PushVkeyEvent: %i:%i:%04X:%04X:%i\n", inclass, dev, key, subkey, value );

        Vkey_t vk = { inclass, dev, key, subkey };
        Event_t ev;

        ev.type = EV_VKEY;
        ev.vkey.vk = vk;
        ev.vkey.flags = flags;
        ev.vkey.value = value;

        Push( &ev );
    }

    Event_t* Event::Pop()
    {
        if ( !evtstack.isEmpty() )
        {
            /*
            // this relies on li::List not releasing anything until it's rewritten!
            evtstack.removeFromEnd(0);

            // returns a pointer to the REMOVED one
            return &evtstack.getUnsafe(evtstack.getLength());
            */
            
            memcpy(&tmpevt, evtstack.getPtrUnsafe(0), sizeof(Event_t));
            evtstack.remove(0);
            return &tmpevt;
        }
        else
            return nullptr;
    }

    void Event::RemoveCached(size_t index)
    {
        evtstack.remove(index);
    }

    Event_t* Event::Top()
    {
        if ( !evtstack.isEmpty() )
            return &evtstack.getUnsafe(evtstack.getLength() - 1);
        else
            return nullptr;
    }
}
