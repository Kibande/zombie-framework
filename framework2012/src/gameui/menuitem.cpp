
#include "gameui.hpp"

#include "../framework/event.hpp"

namespace gameui
{
    static const int spacing = 10;
    static const glm::ivec2 padding(5, 5);

    MenuItem::MenuItem(UIResManager* res, const char* label, const char* iconAtlas, int iconIndex)
            : Widget(res), label(label), iconMode(ICON_ATLAS), iconIndex(iconIndex), iconPath(iconAtlas), mouseOver(false), mouseOverAmt(0.0f)
    {
        colour = res->GetTextColour() * 255.0f;
        tex = nullptr;

        font = 1;
    }
    
    MenuItem::MenuItem(UIResManager* res, const char* label)
            : Widget(res), label(label), iconMode(ICON_NONE), mouseOver(false), mouseOverAmt(0.0f)
    {
        colour = res->GetTextColour() * 255.0f;
        tex = nullptr;
        
        font = 1;
    }

    MenuItem::~MenuItem()
    {
        ReleaseMedia();
    }

    void MenuItem::DrawAt(const Int2& pos)
    {
        res->DrawRoundRect(Short2(pos), Short2(pos + size), RGBA_GREY(1.0f, mouseOverAmt * 0.2f));

        int x = pos.x + padding.x;

        if (tex != nullptr)
        {
            static const int shadow = 1;
            
            zr::R::SetTexture(tex);
            
            zr::R::DrawFilledRect(Short2(x + shadow, pos.y + padding.y + shadow),
                    Short2(x + iconSize.x + shadow, pos.y + padding.y + iconSize.y + shadow), uv[0], uv[1], RGBA_BLACK_A(0.5f));
            
            zr::R::DrawFilledRect(Short2(x, pos.y + padding.y), Short2(x + iconSize.x, pos.y + padding.y + iconSize.y), uv[0], uv[1], RGBA_WHITE);
            
            x += iconSize.x + spacing;
        }

        res->GetFont(font)->DrawAsciiString(label, Int2(x, pos.y + size.y / 2), colour, ALIGN_LEFT | ALIGN_MIDDLE);
    }

    void MenuItem::OnFrame(double delta)
    {
        if (mouseOver)
            mouseOverAmt = (float)glm::min(mouseOverAmt + delta / 0.1f, 1.0);
        else
            mouseOverAmt = (float)glm::max(mouseOverAmt - delta / 0.1f, 0.0);
    }

    int MenuItem::OnMouseMove(int h, int x, int y)
    {
        mouseOver = (h < 0 && IsInsideMe(x, y));
        return mouseOver ? 0 : h;
    }

    int MenuItem::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0 && IsInsideMe(x, y))
        {
            if (pressed)
                FireMouseDownEvent(x, y);
            else
                FireClickEvent(x, y);

            return 1;
        }

        return h;
    }

    void MenuItem::ReleaseMedia()
    {
        res->ReleaseTexture(tex);
    }

    void MenuItem::ReloadMedia()
    {
        minSize = glm::ivec2(0, 0);

        if (res->GetFont(font) != nullptr)
            minSize = res->GetFont(font)->MeasureAsciiString(label);

        switch (iconMode)
        {
            case ICON_ATLAS:
                if (tex == nullptr && !iconPath.isEmpty())
                {
                    tex = res->GetTexture(iconPath, true);
					
                    Int2 size = tex->GetSize();

                    iconSize = Float2(size.y, size.y);
                    uv[0] = Float2((float)iconIndex * size.y / size.x, 0.0f);
                    uv[1] = Float2((float)(iconIndex + 1) * size.y / size.x, 1.0f);

                    minSize.x += spacing + iconSize.x;
                    minSize.y = glm::max(minSize.y, (int)iconSize.y);
                }
                break;
                
            case ICON_SCALE:
                if (tex == nullptr && !iconPath.isEmpty())
                {
                    tex = res->GetTexture(iconPath, true);
                    
                    Int2 size = tex->GetSize();
                    
                    float scale = glm::min((float)iconMaxSize.x / size.x, (float)iconMaxSize.y / size.y);
                    
                    iconSize = Float2(size) * scale;
                    uv[0] = Float2(0.0f, 0.0f);
                    uv[1] = Float2(1.0f, 1.0f);
                    
                    minSize.x += spacing + iconSize.x;
                    minSize.y = glm::max(minSize.y, (int)iconSize.y);
                }
                break;
        }

        minSize += padding * 2;
        size = glm::max(size, minSize);
    }
    
    void MenuItem::SetIconScale(const char* path, CInt2& maxSize)
    {
        iconMode = ICON_SCALE;
        iconPath = path;
        iconMaxSize = maxSize;
    }
}