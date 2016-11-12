
#include <framework/event.hpp>

#include <framework/videohandler.hpp>
#include <framework/utility/essentials.hpp>

// FIXME: FormatVkey/ParseVkey musn't use numbers for devices

namespace zfw
{
    static char formattedVkey[64];

    void Vkey::ClearVkey(Vkey_t* vk_out)
    {
        vk_out->type = VKEY_NONE;
        vk_out->device = -1;
        vk_out->key = -1;
        vk_out->subkey = 0;
    }

    const char* Vkey::FormatVkey(const Vkey_t& vk)
    {
        if (vk.type == VKEY_ANALOG || vk.type == VKEY_JOYBTN)
        {
            auto ivh = g_essentials->GetVideoHandler();
            auto joyName = ivh->JoystickDevToName(vk.device);
            snprintf(formattedVkey, 64, "%i:%s:%04X:%04X", vk.type, joyName ? joyName : "", vk.key, vk.subkey);
        }
        else
            snprintf(formattedVkey, 64, "%i:%i:%04X:%04X", vk.type, vk.device, vk.key, vk.subkey);

        return formattedVkey;
    }

    bool Vkey::IsMouseButtonEvent(const VkeyState_t& input, int& button_out, bool& pressed_out)
    {
        if (input.vkey.type == VKEY_MOUSEBTN)
        {
            button_out = input.vkey.key;
            pressed_out = (input.flags & VKEY_PRESSED) ? true : false;
            return true;
        }
        else
            return false;
    }

    bool Vkey::IsStrongInput(const VkeyState_t& input, int analogThreshold)
    {
        if (input.vkey.type == VKEY_JOYBTN || input.vkey.type == VKEY_KEY || input.vkey.type == VKEY_MOUSEBTN)
            return (input.flags & VKEY_PRESSED) ? true : false;
        else if (input.vkey.type == VKEY_ANALOG)
            return input.value >= analogThreshold;
        else
            return false;
    }

    bool Vkey::ParseVkey(Vkey_t* vk_out, const char* formatted)
    {
        if (formatted == nullptr)
            return false;

        int type = -1, device = -1, key = -1, subkey = -1;

        if (!sscanf(formatted, "%i:", &type))
            return false;

        if (type == VKEY_ANALOG || type == VKEY_JOYBTN)
        {
            const char* nameStart = strchr(formatted, ':');
            if (!nameStart)
                return false;

            const char* nameEnd = strchr(nameStart + 1, ':');
            if (!nameEnd)
                return false;

            li::String joyName(nameStart + 1, nameEnd - nameStart - 1);
            if (joyName.isEmpty())
                return false;

            auto ivh = g_essentials->GetVideoHandler();
            int device = ivh->JoystickNameToDev(joyName);

            if (device < 0 || sscanf(nameEnd + 1, "%X:%X", &key, &subkey) != 2)
                return false;

            vk_out->type =      (int16_t) type;
            vk_out->device =    (int16_t) device;
            vk_out->key =       (int16_t) key;
            vk_out->subkey =    (int16_t) subkey;
        }
        else
        {
            if (sscanf(formatted, "%*i:%i:%X:%X", &device, &key, &subkey) != 3)
                return false;

            vk_out->type =      (int16_t) type;
            vk_out->device =    (int16_t) device;
            vk_out->key =       (int16_t) key;
            vk_out->subkey =    (int16_t) subkey;
        }

        return true;
    }

    bool Vkey::Test(const VkeyState_t& input, const Vkey_t& vk, int analogThreshold)
    {
        if (input.vkey.type != vk.type || input.vkey.key != vk.key)
            return false;

        if (input.vkey.type == VKEY_ANALOG)
            return input.vkey.subkey == vk.subkey && input.value >= analogThreshold;
        else
            return true;
    }
}
