
#include <gameui/gameui.hpp>

#include <framework/event.hpp>
#include <framework/framework.hpp>
#include <framework/videohandler.hpp>

// FIXME: minimize relayouting

namespace gameui
{
    MenuBar::MenuBar(UIThemer* themer)
            : minSizeValid(false), layoutValid(false)
    {
        thm.reset(themer->CreateMenuBarThemer());

        dragsAppWindow = false;
        barItemPadding = Int2(8, 4);
        menuItemPadding = Int2(10, 4);

        firstItem = nullptr;
        lastItem = nullptr;
        expandedItem = nullptr;
        selectedItem = nullptr;
        layoutValid = false;
        minSizeValid = false;
        isDraggingAppWindow = false;

        SetExpands(false);
    }

    MenuBar::~MenuBar()
    {
        pr_ReleaseItem(firstItem);
    }

    MenuItem_t* MenuBar::Add(MenuItem_t* parent, const char* label, const char* name, int flags)
    {
        MenuItem_t* item = new MenuItem_t;
        item->name = name;

        item->ti = thm->AllocItem(parent != nullptr ? parent->ti : nullptr, label);
        item->expanded = false;
        item->flags = flags;
        item->thmFlags = 0;

        if (item->expanded)
            item->thmFlags |= IMenuBarThemer::ITEM_EXPANDED;

        if (parent == nullptr)
        {
            item->prev = lastItem;

            if (lastItem != nullptr)
            {
                lastItem->next = item;
                lastItem = item;
            }
            else
                firstItem = lastItem = item;
        }
        else
        {
            item->prev = parent->last_child;

            item->thmFlags |= IMenuBarThemer::ITEM_HAS_PARENT;

            if (parent->last_child == nullptr)
            {
                parent->thmFlags |= IMenuBarThemer::ITEM_HAS_CHILDREN;
                thm->SetItemFlags(parent->ti, parent->thmFlags);
            }

            if (parent->last_child != nullptr)
            {
                parent->last_child->next = item;
                parent->last_child = item;
            }
            else
                parent->first_child = parent->last_child = item;
        }

        item->first_child = nullptr;
        item->last_child = nullptr;
        item->next = nullptr;

        thm->SetItemFlags(item->ti, item->thmFlags);
        item->labelSize = thm->GetItemLabelSize(item->ti);
        item->size = (parent == nullptr) ? item->labelSize + (short)2 * barItemPadding
                : item->labelSize + (short)2 * menuItemPadding;

        minSizeValid = false;
        layoutValid = false;
   
        return item;
    }

    void MenuBar::Draw()
    {
        thm->Draw();

        pr_DrawItem(firstItem, true);
    }

    const char* MenuBar::GetItemLabel(MenuItem_t* item)
    {
        return (item != nullptr) ? thm->GetItemLabel(item->ti) : nullptr;
    }

    Int2 MenuBar::GetMinSize()
    {
        if (minSizeValid)
            return minSize;

        Int2 pos_buf;
        minSize = Int2();

        for (MenuItem_t* item = firstItem; item != nullptr; item = item->next)
        {
            minSize.x += item->size.x;
            minSize.y = std::max<int>(item->size.y, minSize.y);
        }

        minSizeValid = true;

        return minSize;
    }

    void MenuBar::Layout()
    {
        if (layoutValid)
            return;

        const Int2 minSize = GetMinSize();

        Int2 posInWidgetSpace(0, 0);

        for (MenuItem_t* item = firstItem; item != nullptr; item = item->next)
        {
            item->pos = posInWidgetSpace;
            item->labelPos = item->pos + barItemPadding;

            p_LayoutMenu(Int2(posInWidgetSpace.x, posInWidgetSpace.y + minSize.y), item);

            posInWidgetSpace.x += item->size.x;
        }

        pos = areaPos;
        size = Int2(areaSize.x, minSize.y);

        thm->SetArea(pos, size);

        layoutValid = true;
    }

    void MenuBar::LayoutItem(Int2 posInWidgetSpace, MenuItem_t* item, MenuItem_t* parent)
    {
        for (; item != nullptr; item = item->next)
        {
            item->pos = posInWidgetSpace;
            item->labelPos = item->pos + menuItemPadding;

            posInWidgetSpace.y += item->size.y;

            if (parent != nullptr)
            {
                parent->menuSize.x = std::max<short>(parent->menuSize.x, item->size.x);
                parent->menuSize.y += item->size.y;
            }
        }
    }

    int MenuBar::OnMouseMove(int h, int x, int y)
    {
        if (isDraggingAppWindow)
        {
            /*IVideoHandler* ivh = sys->GetVideoHandler();

            if (ivh != nullptr)
                ivh->MoveWindow(Int2(x, y) - appWindowDragOrigin);*/

            //appWindowDragOrigin = Int2(x, y);
            return 0;
        }

        if (h < 0 && visible)
        {
            h = OnMouseMoveTo(h, Int2(x - pos.x, y - pos.y), firstItem, 0);

            if (h < 0 && selectedItem != nullptr)
            {
                selectedItem->thmFlags &= ~IMenuBarThemer::ITEM_SELECTED;
                thm->SetItemFlags(selectedItem->ti, selectedItem->thmFlags);
                selectedItem = nullptr;
            }
        }

        return h;
    }

    int MenuBar::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (isDraggingAppWindow && !pressed)
        {
            isDraggingAppWindow = false;
            return 0;
        }

        if (h < h_direct && visible)
        {
            if (pressed)
            {
                h = OnMousePress(h, Int2(x - pos.x, y - pos.y), firstItem, true);

                // FIXME: check
                //if (h < h_direct)
                //{
                    if (IsInsideMe(x, y))
                    {
                        if (dragsAppWindow)
                        {
                            appWindowDragOrigin = Int2(x, y);
                            isDraggingAppWindow = true;
                        }

                        h = h_direct;
                    }
                    else if (expandedItem != nullptr)
                    {
                        p_SetExpandedItem(nullptr);
                        h = h_indirect;
                    }
                //}
            }
        }

        return h;
    }

    int MenuBar::OnMouseMoveTo(int h, Int2 posInWidgetSpace, MenuItem_t* item, int level)
    {
        for (; item != nullptr; item = item->next)
        {
            if (h < h_direct
                    && posInWidgetSpace.x >= item->pos.x
                    && posInWidgetSpace.y >= item->pos.y
                    && posInWidgetSpace.x < item->pos.x + item->size.x
                    && posInWidgetSpace.y < item->pos.y + item->size.y)
            {
                if (level == 0)
                {
                    if (expandedItem != nullptr && expandedItem != item)
                        p_SetExpandedItem(item);
                    else if (selectedItem != nullptr)
                    {
                        selectedItem->thmFlags &= ~IMenuBarThemer::ITEM_SELECTED;
                        thm->SetItemFlags(selectedItem->ti, selectedItem->thmFlags);
                        selectedItem = nullptr;
                    }
                }
                else
                {
                    if (selectedItem != item)
                    {
                        if (selectedItem != nullptr)
                        {
                            selectedItem->thmFlags &= ~IMenuBarThemer::ITEM_SELECTED;
                            thm->SetItemFlags(selectedItem->ti, selectedItem->thmFlags);
                        }

                        if ((item->flags & MENU_ITEM_SELECTABLE) || item->first_child != nullptr)
                        {
                            selectedItem = item;

                            item->thmFlags |= IMenuBarThemer::ITEM_SELECTED;
                            thm->SetItemFlags(item->ti, item->thmFlags);

                            if (item->first_child != nullptr)
                                item->expanded = true;
                        }
                        else
                            selectedItem = nullptr;
                    }
                }

                h = h_direct;
            }

            // A dirty way perhaps, but it's cheap and reliable
            if (level >= 1 && item->expanded && selectedItem != item && selectedItem != nullptr && selectedItem->pos.x == item->pos.x)
                item->expanded = false;

            if (item->expanded && item->first_child != nullptr)
                if ((h = OnMouseMoveTo(h, posInWidgetSpace, item->first_child, level + 1)) >= h_stop)
                    return h;
        }

        return h;
    }

    int MenuBar::OnMousePress(int h, Int2 posInWidgetSpace, MenuItem_t* item, bool isTopLevel)
    {
        if (h >= 0)
            return h;

        for (; item != nullptr; item = item->next)
        {
            if (true
                    && posInWidgetSpace.x >= item->pos.x
                    && posInWidgetSpace.y >= item->pos.y
                    && posInWidgetSpace.x < item->pos.x + item->size.x
                    && posInWidgetSpace.y < item->pos.y + item->size.y)
            {
                if (isTopLevel)
                {
                    if (item == expandedItem)
                        p_SetExpandedItem(nullptr);
                    else
                        p_SetExpandedItem(item);

                    return 0;
                }
                else if ((item->flags & MENU_ITEM_SELECTABLE))
                {
                    FireMenuItemSelectedEvent(item);

                    if (expandedItem != nullptr)
                    {
                        expandedItem->expanded = false;

                        expandedItem->thmFlags &= ~IMenuBarThemer::ITEM_EXPANDED;
                        thm->SetItemFlags(expandedItem->ti, expandedItem->thmFlags);

                        expandedItem = nullptr;
                    }

                    return 0;
                }
            }

            if (item->expanded && item->first_child != nullptr)
            {
                if ((h = OnMousePress(h, posInWidgetSpace, item->first_child, false)) >= 1)
                    return h;

                item->expanded = false;
            }
        }

        return h;
    }

    void MenuBar::p_LayoutMenu(Int2 posInWidgetSpace, MenuItem_t* item)
    {
        item->menuSize = Short2();

        if (item->first_child != nullptr)
        {
            item->menuPos = Short2(posInWidgetSpace);

            LayoutItem(Int2(posInWidgetSpace.x, posInWidgetSpace.y), item->first_child, item);

            for (MenuItem_t* child = item->first_child; child != nullptr; child = child->next)
            {
                child->size.x = item->menuSize.x;
                p_LayoutMenu(Int2(child->pos.x + child->size.x, child->pos.y), child);
            }
        }
    }

    void MenuBar::p_LayoutSubMenus(int x, MenuItem_t* item, MenuItem_t* parent)
    {
        for (; item != nullptr; item = item->next)
        {
            if (item->first_child != nullptr)
                LayoutItem(Int2(x, item->pos.y), item->first_child, item);
        }
    }

    void MenuBar::p_SetExpandedItem(MenuItem_t* item)
    {
        if (item == expandedItem)
            return;

        if (expandedItem != nullptr)
        {
            expandedItem->expanded = false;

            expandedItem->thmFlags &= ~IMenuBarThemer::ITEM_EXPANDED;
            thm->SetItemFlags(expandedItem->ti, expandedItem->thmFlags);
        }

        expandedItem = item;

        if (item != nullptr)
        {
            item->expanded = true;

            item->thmFlags |= IMenuBarThemer::ITEM_EXPANDED;
            thm->SetItemFlags(item->ti, item->thmFlags);
        }
    }

    void MenuBar::pr_DrawItem(MenuItem_t* item, bool isTopLevel)
    {
        for (; item != nullptr; item = item->next)
        {
            thm->DrawItem(item->ti, pos + Int3(item->pos, 0), Int2(item->size), pos + Int3(item->labelPos, 0), Int2(item->labelSize));

            if (item->expanded && item->first_child != nullptr)
            {
                thm->DrawMenu(Int3(item->menuPos.x, item->menuPos.y, 0), Int2(item->menuSize));
                pr_DrawItem(item->first_child, false);
            }
        }
    }

    MenuItem_t* MenuBar::pr_GetItemByName(MenuItem_t* item, const char* name)
    {
        // pseudo-breadth-first search

        MenuItem_t* startItem = item;

        for (; item != nullptr; item = item->next)
        {
            if (item->name != nullptr && strcmp(item->name, name) == 0)
                return item;
        }

        MenuItem_t* found;

        for (item = startItem; item != nullptr; item = item->next)
        {
            if (item->first_child != nullptr
                    && (found = pr_GetItemByName(item->first_child, name)) != nullptr)
                return found;
        }

        return nullptr;
    }

    void MenuBar::pr_ReleaseItem(MenuItem_t* item)
    {
        while (item != nullptr)
        {
            thm->ReleaseItem(item->ti);

            if (item->first_child != nullptr)
                pr_ReleaseItem(item->first_child);

            auto next = item->next;
            delete item;
            item = next;
        }
    }

    void MenuBar::SetArea(Int3 pos, Int2 size)
    {
        if (size == areaSize)
        {
            Move(pos - areaPos);
            areaPos = pos;
        }
        else
            layoutValid = false;

        Widget::SetArea(pos, size);
    }

    void MenuBar::SetItemFlags(MenuItem_t* item, int flags)
    {
        item->flags = flags;

        if (selectedItem == item && !(flags & MENU_ITEM_SELECTABLE))
        {
            selectedItem->thmFlags &= ~IMenuBarThemer::ITEM_SELECTED;
            thm->SetItemFlags(selectedItem->ti, selectedItem->thmFlags);
            selectedItem = nullptr;
        }
    }
}
