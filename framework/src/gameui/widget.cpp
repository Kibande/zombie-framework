
#include <gameui/gameui.hpp>

#include <framework/messagequeue.hpp>
#include <framework/utility/util.hpp>

namespace gameui
{
    static MessageQueue* s_msgQueue;

    Widget::Widget()
        : parent(nullptr), parentContainer(nullptr), freeFloat(false), expands(true), visible(true), enabled(true), onlineUpdate(true), align(0)
    {
        //clickEventHandler = nullptr;
        //mouseDownEventHandler = nullptr;

        msgQueue = s_msgQueue;
    }

    Widget::Widget(Int3 pos, Int2 size)
        : parent(nullptr), parentContainer(nullptr), freeFloat(true), expands(true), visible(true), enabled(true), onlineUpdate(true), align(0), pos(pos), size(size)
    {
        //clickEventHandler = nullptr;
        //mouseDownEventHandler = nullptr;

        msgQueue = s_msgQueue;
    }

    Widget::~Widget()
    {
    }

    void Widget::FireClickEvent(int x, int y)
    {
        //if (clickEventHandler != nullptr)
        //    clickEventHandler(this, x, y);
        //else
        {
            MessageConstruction<EventControlUsed> msg(msgQueue);
            msg->widget = this;
        }
    }

    void Widget::FireItemSelectedEvent(int index)
    {
        MessageConstruction<EventItemSelected> msg(msgQueue);
        msg->widget = this;
        msg->itemIndex = index;
    }

    void Widget::FireMenuItemSelectedEvent(MenuItem_t* item)
    {
        MessageConstruction<EventMenuItemSelected> msg(msgQueue);
        msg->menuBar = static_cast<MenuBar*>(this);
        msg->item = item;
    }

    void Widget::FireMouseDownEvent(int button, int x, int y)
    {
        //if (mouseDownEventHandler != nullptr)
        //    mouseDownEventHandler(this, button, x, y);
        //else
        {
            MessageConstruction<EventMouseDown> msg(msgQueue);
            msg->widget = this;
            msg->button = button;
            msg->x = x;
            msg->y = y;
        }
    }

    void Widget::FireTreeItemSelectedEvent(TreeItem_t* item)
    {
        MessageConstruction<EventTreeItemSelected> msg(msgQueue);
        msg->treeBox = static_cast<TreeBox*>(this);
        msg->item = item;
    }

    void Widget::FireValueChangeEvent(int value)
    {
        MessageConstruction<EventValueChanged> msg(msgQueue);
        msg->widget = this;
        msg->value = value;
        msg->floatValue = (float) value;
    }

    void Widget::FireValueChangeEvent(float floatValue)
    {
        MessageConstruction<EventValueChanged> msg(msgQueue);
        msg->widget = this;
        msg->value = (int) floatValue;
        msg->floatValue = floatValue;
    }

    MessageQueue* Widget::GetMessageQueue()
    {
        return s_msgQueue;
    }

    void Widget::Layout()
    {
        if (!freeFloat)
        {
            if (expands)
            {
                // parent (or whoever the caller is) has to make sure that areaSize >= GetMinSize()
                size = areaSize;

                const Int2 maxSize = GetMaxSize();

                if (maxSize.x > 0 && size.x > maxSize.x)
                    size.x = maxSize.x;

                if (maxSize.y > 0 && size.y > maxSize.y)
                    size.y = maxSize.y;
            }
            else
                //size = glm::max(size, GetMinSize());
                size = GetMinSize();

            if (align & ALIGN_HCENTER)
                pos.x = areaPos.x + (areaSize.x - size.x) / 2;
            else if (align & ALIGN_RIGHT)
                pos.x = areaPos.x + areaSize.x - size.x;
            else
                pos.x = areaPos.x;

            if (align & ALIGN_VCENTER)
                pos.y = areaPos.y + (areaSize.y - size.y) / 2;
            else if (align & ALIGN_BOTTOM)
                pos.y = areaPos.y + areaSize.y - size.y;
            else
                pos.y = areaPos.y;
        }
        else
            size = glm::max(size, GetMinSize());
    }

    void Widget::SetArea(Int3 pos, Int2 size)
    {
        areaPos = pos;
        areaSize = size;

        Layout();
    }

    void Widget::SetEnabled(bool enabled, bool recursive)
    {
        this->enabled = enabled;
    }

    void Widget::SetMessageQueue(MessageQueue* queue)
    {
        s_msgQueue = queue;
    }

    void Widget::SetPrivateMessageQueue(MessageQueue* queue)
    {
        msgQueue = queue;
    }
}
