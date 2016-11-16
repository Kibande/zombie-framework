
#include <gameui/gameui.hpp>

#include <framework/event.hpp>

namespace gameui
{
    TreeBox::TreeBox(UIThemer* themer)
            : label_x_offset(8), y_spacing(2)
    {
        thm.reset(themer->CreateTreeBoxThemer(pos, size));

        firstItem = nullptr;
        lastItem = nullptr;
        selectedItem = nullptr;
        minSizeValid = false;
        layoutValid = false;
    }

    TreeBox::~TreeBox()
    {
        pr_ReleaseItem(firstItem);
    }

    bool TreeBox::AcquireResources()
    {
        if (!thm->AcquireResources())
            return false;

        arrowSize = thm->GetArrowSize();
        return true;
    }

    TreeItem_t* TreeBox::Add(const char* label, TreeItem_t* parent)
    {
        TreeItem_t* item = new TreeItem_t;

        item->ti = thm->AllocItem(parent != nullptr ? parent->ti : nullptr, label);
        item->expanded = false;

        thm->SetItemExpanded(item->ti, item->expanded);
        thm->SetItemHasChildren(item->ti, false);
        thm->SetItemSelected(item->ti, false);

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

            if (parent->last_child != nullptr)
            {
                parent->last_child->next = item;
                parent->last_child = item;
            }
            else
                parent->first_child = parent->last_child = item;

            thm->SetItemHasChildren(parent->ti, true);
        }

        item->first_child = nullptr;
        item->last_child = nullptr;
        item->next = nullptr;

        item->labelSize = thm->GetItemLabelSize(item->ti);
        item->height = std::max<int>(arrowSize.y, item->labelSize.y);

        if (parent == nullptr || parent->expanded)
        {
            minSizeValid = false;
            layoutValid = false;

            // TODO: Allow disabling online update
            if (parentContainer != nullptr)
                parentContainer->OnWidgetMinSizeChange(this);
        }
   
        return item;
    }

    void TreeBox::Clear()
    {
        pr_ReleaseItem(firstItem);

        firstItem = nullptr;
        lastItem = nullptr;
        selectedItem = nullptr;
    }

    void TreeBox::CollapseAll()
    {
        pr_SetExpanded(firstItem, false);

        minSizeValid = false;
        layoutValid = false;

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);
    }

    void TreeBox::Draw()
    {
        DrawItem(firstItem);
    }

    void TreeBox::ExpandAll()
    {
        pr_SetExpanded(firstItem, true);

        minSizeValid = false;
        layoutValid = false;

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);
    }

    void TreeBox::DrawItem(TreeItem_t* item)
    {
        for (; item != nullptr; item = item->next)
        {
            thm->DrawItem(item->ti, pos + Int3(item->arrowPos, 0), pos + Int3(item->labelPos, 0));

            if (item->expanded && item->first_child != nullptr)
                DrawItem(item->first_child);
        }
    }

    const char* TreeBox::GetItemLabel(TreeItem_t* item)
    {
        return (item != nullptr) ? thm->GetItemLabel(item->ti) : nullptr;
    }

    Int2 TreeBox::GetMinSize()
    {
        if (minSizeValid)
            return minSize;

        Int2 pos_buf;
        contentsSize = Int2();
        MeasureItem(pos_buf, contentsSize, firstItem);

        if (contentsSize.y > 0)
            contentsSize.y -= y_spacing;

        minSize = contentsSize;
        minSizeValid = true;

        return minSize;
    }

    bool TreeBox::HasChildren(TreeItem_t* parentOrNull)
    {
        if (parentOrNull != nullptr)
            return parentOrNull->first_child != nullptr;
        else
            return firstItem != nullptr;
    }

    void TreeBox::Layout()
    {
        if (layoutValid)
            return;

        pos = areaPos;
        size = areaSize;

        Int2 pos(4, 4);

        LayoutItem(pos, firstItem);

        layoutValid = true;
    }

    void TreeBox::LayoutItem(Int2& pos, TreeItem_t* item)
    {
        for (; item != nullptr; item = item->next)
        {
            item->arrowPos = Int2(pos.x, pos.y + (item->height - arrowSize.y) / 2);
            item->labelPos = Int2(pos.x + arrowSize.x + label_x_offset, pos.y + (item->height - item->labelSize.y) / 2);

            pos.y += item->height + y_spacing;

            if (item->expanded && item->first_child != nullptr)
            {
                pos.x += 16;
                LayoutItem(pos, item->first_child);
                pos.x -= 16;
            }
        }
    }

    void TreeBox::MeasureItem(Int2& pos, Int2& size, const TreeItem_t* item)
    {
        for (; item != nullptr; item = item->next)
        {
            size.x = std::max<int>(size.x, pos.x + arrowSize.x + label_x_offset + item->labelSize.x);
            size.y += item->height + y_spacing;

            if (item->expanded && item->first_child != nullptr)
            {
                pos.x += 16;
                MeasureItem(pos, size, item->first_child);
                pos.x -= 16;
            }
        }
    }

    int TreeBox::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (!visible)
            return h;

        if (IsInsideMe(x, y))
        {
            if (pressed)
                OnMousePress(Int2(x - pos.x, y - pos.y), firstItem);

            return 0;
        }

        return h;
    }

    bool TreeBox::OnMousePress(Int2 posInWidgetSpace, TreeItem_t* item)
    {
        for (; item != nullptr; item = item->next)
        {
            if (item->first_child != nullptr
                    && posInWidgetSpace.x >= item->arrowPos.x
                    && posInWidgetSpace.y >= item->arrowPos.y
                    && posInWidgetSpace.x < item->arrowPos.x + arrowSize.x
                    && posInWidgetSpace.y < item->arrowPos.y + arrowSize.y)
            {
                item->expanded = !item->expanded;
                thm->SetItemExpanded(item->ti, item->expanded);

                // Yup, we're doing a full relayout. It's not slow enough to matter.
                minSizeValid = false;
                layoutValid = false;

                if (parentContainer != nullptr)
                    parentContainer->OnWidgetMinSizeChange(this);

                Layout();

                return false;
            }
            else if (posInWidgetSpace.x >= item->labelPos.x
                    && posInWidgetSpace.y >= item->labelPos.y
                    && posInWidgetSpace.x < item->labelPos.x + item->labelSize.x
                    && posInWidgetSpace.y < item->labelPos.y + item->labelSize.y)
            {
                SetSelection(item, true);

                return false;
            }

            if (item->first_child != nullptr)
                if (!OnMousePress(posInWidgetSpace, item->first_child))
                    return false;
        }

        return true;
    }

    void TreeBox::pr_ReleaseItem(TreeItem_t* item)
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

    bool TreeBox::pr_RemoveItem(TreeItem_t* lookFor, TreeItem_t* item, TreeItem_t* parent)
    {
        while (item != nullptr)
        {
            if (item == lookFor)
            {
                if (parent != nullptr)
                {
                    if (parent->first_child == lookFor)
                        parent->first_child = lookFor->next;

                    if (parent->last_child == lookFor)
                        parent->last_child = lookFor->prev;
                }

                if (item->prev != nullptr)
                    item->prev->next = item->next;

                if (item->next != nullptr)
                    item->next->prev = item->prev;

                return true;
            }

            if (item->first_child != nullptr)
                if (pr_RemoveItem(lookFor, item->first_child, item))
                    return true;

            item = item->next;
        }

        return false;
    }

    void TreeBox::pr_SetExpanded(TreeItem_t* item, bool expanded)
    {
        for (; item != nullptr; item = item->next)
        {
            item->expanded = expanded;
            thm->SetItemExpanded(item->ti, item->expanded);

            if (item->first_child != nullptr)
                pr_SetExpanded(item->first_child, expanded);
        }
    }

    void TreeBox::RemoveItem(TreeItem_t* item, bool selectNext)
    {
        if (item == selectedItem)
            SetSelection(selectNext ? item->next : nullptr, true);

        pr_RemoveItem(item, firstItem, nullptr);

        if (item == firstItem)
            firstItem = item->next;

        if (item == lastItem)
            lastItem = item->prev;

        // release item
        thm->ReleaseItem(item->ti);

        if (item->first_child != nullptr)
            pr_ReleaseItem(item->first_child);

        delete item;

        minSizeValid = false;
        layoutValid = false;

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);

        Layout();
    }

    void TreeBox::SetItemLabel(TreeItem_t* item, const char* label)
    {
        thm->SetItemLabel(item->ti, label);
    }

    void TreeBox::SetSelection(TreeItem_t* item, bool fireEvents)
    {
        if (selectedItem == item)
            return;

        if (selectedItem != nullptr)
            thm->SetItemSelected(selectedItem->ti, false);

        selectedItem = item;

        if (item != nullptr)
            thm->SetItemSelected(item->ti, true);

        FireTreeItemSelectedEvent(item);
    }
}
