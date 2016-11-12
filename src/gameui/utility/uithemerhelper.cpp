
#include <gameui/uithemer.hpp>

#include <framework/utility/params.hpp>
#include <framework/utility/util.hpp>

namespace gameui
{
    bool UIThemerUtil::TryParseFontParams(const char* font, li::String& path_out, int& size_out, int& flags_out)
    {
        if (Params::IsValid(font) < PARAMS_VALID)
            return false;

        const char *key, *value;

        flags_out = 0;
        size_out = 0;

        while (Params::Next(font, key, value))
        {
            if (strcmp(key, "path") == 0)
                path_out = value;
            else if (strcmp(key, "size") == 0)
                size_out = Util::ParseInt(value);
            else if (strcmp(key, "bold") == 0)
                flags_out = SetFlagInBitField(flags_out, FONT_BOLD, Util::ParseBool(value));
            else if (strcmp(key, "italic") == 0)
                flags_out = SetFlagInBitField(flags_out, FONT_ITALIC, Util::ParseBool(value));
        }

        return !path_out.isEmpty() && size_out > 0;
    }
}
