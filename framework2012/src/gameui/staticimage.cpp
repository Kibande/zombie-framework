
#include "gameui.hpp"

namespace gameui
{
    StaticImage::StaticImage(UIResManager* res, const char* path)
            : Widget(res), painter(res, path, IMAGEMAP_NORMAL)
    {
    }

    StaticImage::StaticImage(UIResManager* res, const char* path, glm::ivec2 pos, glm::ivec2 size)
            : Widget(res, pos, size), painter(res, path, IMAGEMAP_NORMAL)
    {
    }

    StaticImage::~StaticImage()
    {
        ReleaseMedia();
    }

    void StaticImage::Draw()
    {
        if (!visible)
            return;

        painter.Draw(pos, size);
    }

    int StaticImage::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (pressed && h < 0 && IsInsideMe(x, y))
        {
            FireMouseDownEvent(x, y);
            return 1;
        }

        return h;
    }

    void StaticImage::ReleaseMedia()
    {
        painter.ReleaseMedia();
    }

    void StaticImage::ReloadMedia()
    {
        painter.ReloadMedia();
    }
}