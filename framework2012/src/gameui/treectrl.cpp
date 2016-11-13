
#include "gameui.hpp"

#include "../framework/event.hpp"

namespace gameui
{
    TreeBox::TreeBox(UIResManager* res)
        : Widget(res), painter(res, 0), contents(nullptr), contents_last(nullptr), selected_node(nullptr)
        , contents_x(0), contents_y(0), contents_w(0), contents_h(0), scrollbar_x(0), scrollbar_y(0)
        , rebuildNeeded(true)
    {
    }

    TreeBoxNode* TreeBox::Add(const char* label, TreeBoxNode* parent)
    {
        TreeBoxNode* node = new TreeBoxNode;

        node->label = label;
        node->open = true;

        if (parent == nullptr)
        {
            node->prev = contents_last;

            if (contents_last != nullptr)
            {
                contents_last->next = node;
                contents_last = node;
            }
            else
                contents = contents_last = node;
        }
        else
        {
            node->prev = parent->children_last;

            if (parent->children_last != nullptr)
            {
                parent->children_last->next = node;
                parent->children_last = node;
            }
            else
                parent->children = parent->children_last = node;
        }

        node->children = nullptr;
        node->children_last = nullptr;
        node->next = nullptr;

        rebuildNeeded = true;
        return node;
    }

    void TreeBox::Draw()
    {
        //painter.Draw(pos, size, root);

        DrawNode(contents);
    }

    void TreeBox::DrawNode(TreeBoxNode* node)
    {
        while (node != nullptr)
        {
            const Int2 pos = this->pos;

            if (node->children != nullptr)
                painter.DrawArrow(pos + node->arrow_pos, node->open);

            if (node == selected_node)
                painter.DrawLabelHighlight(pos + node->label_pos, node->label_size);

            painter.DrawLabel(pos + node->label_pos, node->label);

            if (node->open && node->children != nullptr)
                DrawNode(node->children);

            node = node->next;
        }
    }

    Int2 TreeBox::GetMinSize()
    {
        if (rebuildNeeded)
        {
            /*contents_w = 0;
            contents_h = 0;
            rowHeights.resize(widgets.getLength());

            for (size_t i = 0; i < widgets.getLength(); i++)
            {
                const glm::ivec2 ms = widgets[i]->GetMinSize();
                rowHeights[i] = ms.y;

                contents_w = glm::max(contents_w, ms.x);
                contents_h += ms.y;

                if (i + 1 < widgets.getLength())
                    contents_h += spacing;
            }*/

            rebuildNeeded = false;
        }

        //return glm::max(minSize, glm::ivec2(contents_w + ((contents_h > size.y) ? (scrollbar_w + 4) : 0), 0));
        //return glm::max(minSize, glm::ivec2(contents_w + painter.GetScrollbarWidth() + 4, 0));

        return glm::max(minSize, Int2(painter.GetScrollbarWidth(), painter.GetScrollbarHeight()));
    }

    void TreeBox::Layout()
    {
        Int2 pos(4, 4);

        LayoutNode(pos, contents);
    }

    void TreeBox::LayoutNode(Int2& pos, TreeBoxNode* node)
    {
        while (node != nullptr)
        {
            node->pos = pos;

            node->arrow_size = painter.GetArrowSize();
            node->label_size = painter.GetSize(node->label);

            int height = glm::max(node->arrow_size.y, node->label_size.y) + painter.GetSpacingY();

            node->arrow_pos = Int2(node->pos.x, node->pos.y + (height - node->arrow_size.y) / 2);
            node->label_pos = Int2(node->pos.x + node->arrow_size.x + painter.GetLabelXOffset(), node->pos.y + (height - node->label_size.y) / 2);

            pos.y += height;

            if (node->open && node->children != nullptr)
            {
                pos.x += 16;
                LayoutNode(pos, node->children);
                pos.x -= 16;
            }

            node->size = Int2(0, pos.y - node->pos.y);
            node = node->next;
        }
    }

    /*void TreeBox::MoveNode(Int2& vec, TreeBoxNode* node)
    {
        while (node != nullptr)
        {
            node->pos += vec;

            if(node->open && node->children != nullptr)
                MoveNode(vec, node->children);

            node = node->next;
        }
    }*/

    void TreeBox::OnFrame(double delta)
    {
        for (int i = Event::GetCount() - 1; i >= 0; i--)
        {
            Event_t* ev = Event::GetCached(i);

            if (ev->type == EV_MOUSE_BTN)
            {
                int x = ev->mouse.x;
                int y = ev->mouse.y;

                if (x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y)
                {
                    if ((ev->mouse.flags & VKEY_PRESSED))
                    {
                        OnMousePress(Int2(ev->mouse.x, ev->mouse.y) - pos, contents);

                        Event::RemoveCached(i);
                    }
                }
            }
        }
    }

    void TreeBox::OnMousePress(glm::ivec2 mouse, TreeBoxNode *node)
    {
        while (node != nullptr)
        {
            if (node->children != nullptr
                    && mouse.x >= node->arrow_pos.x
                    && mouse.y >= node->arrow_pos.y
                    && mouse.x < node->arrow_pos.x + node->arrow_size.x
                    && mouse.y < node->arrow_pos.y + node->arrow_size.y)
            {
                node->open = !node->open;

                TreeBoxNode *node2 = node->next;

                while (node2 != nullptr)
                {
                    if (node->open)
                        node2->pos.y += node->size.y;
                    else
                        node2->pos.y -= node->size.y;

                    node2 = node2->next;
                }
            }
            else if (mouse.x >= node->label_pos.x
                    && mouse.y >= node->label_pos.y
                    && mouse.x < node->label_pos.x + node->label_size.x
                    && mouse.y < node->label_pos.y + node->label_size.y)
            {
                selected_node = node;
            }

            if (node->children != nullptr)
                OnMousePress(mouse, node->children);

            node = node->next;
        }
    }

    void TreeBox::Relayout()
    {
        rebuildNeeded = true;
        GetMinSize();
        Layout();

        WidgetContainer::Relayout();
    }

    void TreeBox::ReloadMedia()
    {
        rebuildNeeded = true;

        painter.ReloadMedia();
        WidgetContainer::ReloadMedia();
    }

    void TreeBox::SetArea(Int2 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;

        Layout();
    }
}