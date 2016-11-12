#pragma once

#include <framework/base.hpp>

// TODO: Document subkey

namespace zfw
{
    // Messages for zfw::Event ($2000..20FF)
    enum {
        EVENT_DUMMY_MSG = 0x2000,

        // In

        // Out
        EVENT_MOUSE_BUTTON,
        EVENT_MOUSE_MOVE,
        EVENT_UNICODE_INPUT,
        EVENT_VKEY,
        EVENT_WINDOW_CLOSE,
        EVENT_WINDOW_RESIZE,

        // Private

        EVENT_MAX_MSG
    };

    enum VkeyType_t
    {
        VKEY_NONE,          // null input
        VKEY_ANALOG,        // joystick/gamepad analog
        VKEY_JOYBTN,        // joystick/gamepad buttons, triggers
        VKEY_KEY,           // keyboard
        VKEY_MOUSEBTN,      // mouse button
        VKEY_SPECIAL        // special vkeys (see below)
    };

    enum
    {
        ANALOG_DEFAULT_THRESHOLD = 20000
    };

    // int16_t Vkey_t::key for VKEY_JOYBTN
    enum
    {
        JOYBTN_HAT_UP       = -1,
        JOYBTN_HAT_DOWN     = -2,
        JOYBTN_HAT_LEFT     = -3,
        JOYBTN_HAT_RIGHT    = -4
    };

    // int16_t Vkey_t::key for VKEY_KEY
    enum
    {
        KEY_ESCAPE = 27,
        KEY_PRINTSCREEN = 70,
    };

    // int16_t Vkey_t::key for VKEY_MOUSEBTN
    enum
    {
        MOUSEBTN_LEFT,
        MOUSEBTN_RIGHT,
        MOUSEBTN_MIDDLE,
        MOUSEBTN_WHEEL_UP,
        MOUSEBTN_WHEEL_DOWN,
        MOUSEBTN_OTHER = 10
    };

    // int16_t Vkey_t::key for VKEY_SPECIAL
    enum
    {
        SPECIAL_CLOSE_WINDOW
    };

    // int VkeyState_t::flags
    enum
    {
        VKEY_PRESSED        = 1,
        VKEY_RELEASED       = 2,
        VKEY_VALUE_CHANGED  = 8
    };

    // Vkey_t: 8 bytes
    struct Vkey_t
    {
        int16_t type,       // one of VkeyType_t
                device,     // device index (unique among same VkeyType_t) or -1 if any/unspecified/not applicable
                key,        // key index, button index, analog axis (unique among one device)
                subkey;     // sub-key index (actual meaning varies)
    };

    // VkeyState_t: 16 bytes
    struct VkeyState_t
    {
        Vkey_t  vkey;
        int     flags,      // VKEY_VALUE_CHANGED is only applicable to VKEY_ANALOG
                value;      // currently only applicable to VKEY_ANALOG
                            // always 0..32767, subkey determines actual direction (+1/-1)
    };

    DECL_MESSAGE(EventMouseMove, EVENT_MOUSE_MOVE)
    {
        int x, y;
    };

    DECL_MESSAGE(EventUnicodeInput, EVENT_UNICODE_INPUT)
    {
        VkeyState_t input;          // may be VKEY_NONE
        char32_t c;
    };

    DECL_MESSAGE(EventVkey, EVENT_VKEY)
    {
        VkeyState_t input;          // musn't be VKEY_NONE
        //Int2 mouse;
    };
    
    DECL_MESSAGE(EventWindowResize, EVENT_WINDOW_RESIZE)
    {
        unsigned int width, height;
    };

    class Vkey
    {
        public:
            static void         ClearVkey(Vkey_t* vk_out);

            static const char*  FormatVkey(const Vkey_t& vk);
            static bool         ParseVkey(Vkey_t* vk_out, const char* formatted);

            static bool         IsMouseButtonEvent(const VkeyState_t& input, int& button_out, bool& pressed_out);
            static bool         IsStrongInput(const VkeyState_t& input, int analogThreshold = ANALOG_DEFAULT_THRESHOLD);
            static bool         Test(const VkeyState_t& input, const Vkey_t& vk, int analogThreshold = ANALOG_DEFAULT_THRESHOLD);
    };
}
