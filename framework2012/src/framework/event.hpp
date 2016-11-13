#pragma once

#include "framework.hpp"

namespace gameui
{
    class Widget;
}

namespace zfw
{
    class Session;

    enum EventType
    {
        EV_DUMMY, EV_UNICODE_IN,
        EV_MOUSE_BTN, EV_MOUSE_MOVE, EV_MOUSE_WHEEL,
        EV_UICLICK, EV_UIITEMSELECTED, EV_UIMOUSEDOWN, EV_VIEWPORT_RESIZE, EV_VKEY,
    };

    enum
    {
        VKEY_PRESSED        = 1,
        VKEY_RELEASED       = 2,
        VKEY_TRIG           = 4,
        VKEY_VALUECHANGE    = 8
    };

    /*enum VKEY_Fixed
    {
        V_NONE = 0,
        V_LEFTMOUSE,
        V_RIGHTMOUSE,
        V_ENTER,
        V_UP,
        V_DOWN,
        V_LEFT,
        V_RIGHT,
    };*/

    enum { IN_KEY, IN_ANALOG, IN_JOYBTN, IN_JOYHAT, IN_MOUSEBTN, IN_OTHER };
    enum { HAT_NONE = 0, HAT_UP = 1, HAT_DOWN = 2, HAT_LEFT = 4, HAT_RIGHT = 8 };
    enum { OTHER_CLOSE };
    enum { UNI_BACKSP = 8, UNI_TAB = 9, UNI_ENTER = 0x0D };

    struct Vkey_t
    {
        int16_t inclass, dev, key, subkey;

        void clear() { inclass = -1; dev = -1; key = 0; subkey = 0; }
    };

    struct VkeyState_t
    {
        Vkey_t vk;
        int flags, value;

        bool triggered() { return (flags & (VKEY_PRESSED | VKEY_TRIG | VKEY_VALUECHANGE)) != 0; }
    };

    enum { MOUSE_BTN_LEFT, MOUSE_BTN_RIGHT, MOUSE_BTN_MIDDLE, MOUSE_BTN_WHEELUP, MOUSE_BTN_WHEELDOWN, MOUSE_BTN_OTHER = 10 };
    enum { MOUSE_WHEEL_UP, MOUSE_WHEEL_DOWN };

    struct MouseEvent_t
    {
        int x, y, button, flags;
    };

    struct UIItemEvent_t
    {
        gameui::Widget* widget;
        int itemIndex;
    };

    struct UIMouseEvent_t
    {
        gameui::Widget* widget;
        int x, y;
    };

    struct ViewportResizeEvent_t
    {
        unsigned int w, h;
    };

    struct SessionResult
    {
        Session* session;
        int result;
    };

    static const int NUM_BUTTONS = 19;

    union GamepadCfg_t
    {
        struct
        {
            Vkey_t up, down, left, right, a, b, x, y, l, r, menu, aup, adown, aleft, aright, mup, mdown, mleft, mright;
        };

        Vkey_t buttons[NUM_BUTTONS];
    };

    //
    // Local event structure
    //
    struct Event_t
    {
        EventType type;

        union
        {
            VkeyState_t vkey;
            uint32_t uni_cp;
            MouseEvent_t mouse;
            UIItemEvent_t ui_item;
            UIMouseEvent_t ui_mouse;
            ViewportResizeEvent_t viewresize;
            SessionResult session;
        };
    };

    class Event
    {
        public:
            static const char* GetButtonName( int index );
            static const char* GetButtonShortName( int index );

            static const char* FormatVkey(const Vkey_t& vkey);
            static void ParseVkey(const char* fmt, Vkey_t& vkey);

            static void Push( const Event_t* event );
            static void PushMouseEvent(EventType event, int x, int y, int flags, int button);
            static void PushUIItemEvent(EventType event, gameui::Widget* widget, int itemIndex);
            static void PushUIMouseEvent(EventType event, gameui::Widget* widget, int x, int y);
            static void PushUnicodeInEvent( uint32_t cp );
            static void PushVkeyEvent( int16_t inclass, int16_t dev, int16_t key, int16_t subkey, int flags, int value );
            static void PushViewportResizeEvent(unsigned int w, unsigned int h);

            static Event_t* Pop();
            static Event_t* Top();

            static size_t GetCount();
            static Event_t* GetCached(size_t index);
            static void RemoveCached(size_t index);
    };
}
