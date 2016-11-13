
#include "gameui.hpp"

namespace gameui
{
    StaticText::StaticText(UIResManager* res, const char* label, size_t font)
            : Widget(res), painter(res, label, font)
    {
    }

    StaticText::StaticText(UIResManager* res, const char* label, glm::ivec2 pos, glm::ivec2 size, size_t font)
            : Widget(res, pos, size), painter(res, label, font)
    {
    }

    void StaticText::DrawAt(const Int2& pos)
    {
        painter.Draw(pos, size);
    }

    void StaticText::ReloadMedia()
    {
        painter.GetMinSize(minSize);
    }
}