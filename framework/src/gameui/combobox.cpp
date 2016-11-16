
#include <gameui/gameui.hpp>

// FIXME: This whole thing kinda stinks

namespace gameui
{
    ComboBox::ComboBox(UIThemer* themer, UIContainer* uiBase)
            : uiBase(uiBase)
    {
        thm.reset(themer->CreateComboBoxThemer(pos, size));

        isExpanded = false;
        selection = -1;
        selectedItem = nullptr;

        rebuildNeeded = true;

        popup = new ComboBoxPopup(themer, this);
    }

    ComboBox::~ComboBox()
    {
        // If the popup is being modal it will be deleted by UIContainer
        if (!isExpanded)
            delete popup;
    }

    void ComboBox::Add(Widget* widget)
    {
        popup->table->Add(widget);

        // TODO: Is this the nicest way?
        rebuildNeeded = true;

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);
    }

    void ComboBox::ClosePopup()
    {
        uiBase->PopModal(false);
        isExpanded = false;

        SetSelection(popup->mouseOver);

        thm->SetExpanded(isExpanded);
        Layout();

        if (selection >= 0)
            FireItemSelectedEvent(selection);
    }

    void ComboBox::Draw()
    {
        thm->Draw();

        if (selectedItem != nullptr)
            selectedItem->Draw();
    }

    Int2 ComboBox::GetMaxSize()
    {
        GetMinSize();

        return maxSize;
    }

    Int2 ComboBox::GetMinSize()
    {
        if (rebuildNeeded)
        {
            contentsSize = Int2(0, 0);
            minSize = Int2(0, 0);

            auto& widgets = popup->table->GetWidgetList();

            for ( size_t i = 0; i < widgets.getLength(); i++ )
            {
                Int2 itemSize = widgets[i]->GetMinSize();

                if ( itemSize.x > contentsSize.x )
                    contentsSize.x = itemSize.x;

                if ( itemSize.y > minSize.y )
                    minSize.y = itemSize.y;

                contentsSize.y += itemSize.y;
            }

            contentsSize.x = popup->GetMinSize().x;

            thm->SetContentsSize(contentsSize);
            minSize = thm->GetMinSize(padding * 2 + Int2(contentsSize.x, minSize.y));

            maxSize.y = minSize.y;

            rebuildNeeded = false;
        }

        return minSize;
    }

    void ComboBox::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);

        if (!isExpanded && selectedItem != nullptr)
            selectedItem->SetArea(pos + Int3(padding, 0), Int2(contentsSize.x, minSize.y));
    }

    void ComboBox::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);

        popup->Move(vec);
    }

    int ComboBox::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0)
        {
            if (IsInsideMe(x, y) && !isExpanded)
            {
                if (pressed)
                {
                    isExpanded = true;
                    popup->SetPos(Int3(pos.x, pos.y + size.y, pos.z + 1));
                    uiBase->PushModal(popup);

                    thm->SetExpanded(isExpanded);
                    Layout();
                }

                return 1;
            }
        }

        return h;
    }

    void ComboBox::SetSelection(int selection)
    {
        selectedItem = popup->GetItemByIndex(selection);

        if (selectedItem != nullptr)
            this->selection = selection;
        else
            this->selection = -1;

        Layout();
    }

    ComboBoxPopup::ComboBoxPopup(UIThemer* themer, ComboBox* comboBox)
        : Window(themer, nullptr, 0), comboBox(comboBox)
    {
        SetFreeFloat(true);
        SetPadding(Int2(3, 3));

        mouseOver = -1;

        thm = comboBox->thm.get();
        table = new TableLayout(1);
        Add(table);
    }

    void ComboBoxPopup::Draw()
    {
        if (!visible)
            return;

        thm->DrawPopup();

        WidgetContainer::Draw();
    }

    int ComboBoxPopup::GetItemAt(int x, int y)
    {
        auto& widgets = table->GetWidgetList();

        for (size_t i = 0; i < widgets.getLength(); i++)
        {
            const Int3 pos = widgets[i]->GetPos();
            const Int2 size = widgets[i]->GetSize();

            if (x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y)
                return (int) i;
        }

        return -1;
    }

    Widget* ComboBoxPopup::GetItemByIndex(int index)
    {
        if (index >= 0 && (unsigned int) index < table->GetWidgetList().getLength())
            return table->GetWidgetList()[index];
        else
            return nullptr;
    }

    void ComboBoxPopup::Layout()
    {
        Window::Layout();

        table->InvalidateLayout();
        table->Layout();

        thm->SetPopupArea(pos, size);
    }

    int ComboBoxPopup::OnMouseMove(int h, int x, int y)
    {
        if (h < 0)
        {
            mouseOver = GetItemAt(x, y);
            WidgetContainer::OnMouseMove(0, x, y);

            if (mouseOver >= 0)
            {
                Widget* selectedWidget = table->GetWidgetList()[mouseOver];
                thm->SetPopupSelectionArea(true, selectedWidget->GetPos(), selectedWidget->GetSize());
            }
            else

            return (mouseOver != -1) ? 0 : h;
        }
        else
            mouseOver = -1;

        return h;
    }

    int ComboBoxPopup::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0)
        {
            //if (IsInsideMe(x, y))
            if (pressed)
            {
                comboBox->ClosePopup();

                if (IsInsideMe(x, y))
                    return 1;
            }
        }

        return h;
    }
}
