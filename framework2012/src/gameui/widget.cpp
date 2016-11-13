
#include "gameui.hpp"

namespace gameui
{
    Widget::Widget(UIResManager* res)
        : res(res), parent(nullptr), parentContainer(nullptr), freeFloat(false), expands(true), visible(true), align(0)
    {
        clickEventHandler = nullptr;
        mouseDownEventHandler = nullptr;
    }

    Widget::Widget(UIResManager* res, glm::ivec2 pos, glm::ivec2 size)
        : res(res), parent(nullptr), parentContainer(nullptr), freeFloat(true), expands(true), visible(true), align(0), pos(pos), size(size)
    {
        clickEventHandler = nullptr;
        mouseDownEventHandler = nullptr;
    }

    Widget::~Widget()
    {
    }

    void Widget::SetArea(Int2 pos, Int2 size)
    {
        if (!freeFloat)
        {
            if (expands)
            {
                // parent (or whoever is the caller) should ensure that size >= GetMinSize()
                this->size = size;

                const Int2 maxSize = GetMaxSize();

                if (maxSize.x > 0 && this->size.x > maxSize.x)
                    this->size.x = maxSize.x;

                if (maxSize.y > 0 && this->size.y > maxSize.y)
                    this->size.y = maxSize.y;
            }
            else
                this->size = glm::max(this->size, GetMinSize());

            if (align & HCENTER)
                this->pos.x = pos.x + (size.x - this->size.x) / 2;
            else if (align & RIGHT)
                this->pos.x = pos.x + size.x - this->size.x;
            else
                this->pos.x = pos.x;

            if (align & VCENTER)
                this->pos.y = pos.y + (size.y - this->size.y) / 2;
            else if (align & BOTTOM)
                this->pos.y = pos.y + size.y - this->size.y;
            else
                this->pos.y = pos.y;
        }
        else
            this->size = glm::max(this->size, GetMinSize());
    }
}