
#include "gameui.hpp"

namespace gameui
{
    static CInt2 default_spacing(5, 5);

    TableSizer::TableSizer(UIResManager* res, size_t numColumns)
            : Widget(res), numColumns(numColumns), rebuildNeeded(true)
    {
        spacing = default_spacing;
    }

    void TableSizer::Add(Widget* widget)
    {
        if (widget == nullptr)
            return;

        widget->SetParent(this);
        WidgetContainer::Add(widget);

        widget->SetParent(this);
        rebuildNeeded = true;
    }

    Int2 TableSizer::GetMinSize()
    {
        if(rebuildNeeded)
        {
            // Determine the table height
            size_t numRows = widgets.getLength() / numColumns;

            if ( widgets.getLength() % numColumns != 0 )
                numRows++;

            rowHeights.resize(numRows, false);
            columnWidths.resize(numColumns, false);

            size_t i = 0;

            for ( size_t row = 0; i < widgets.getLength(); row++ )
            {
                for ( size_t column = 0; column < numColumns && i < widgets.getLength(); column++ )
                {
                    Int2 cellMinSize = widgets[i]->GetMinSize();

                    if ( cellMinSize.x > columnWidths[column] )
                        columnWidths[column] = cellMinSize.x;

                    if ( cellMinSize.y > rowHeights[row] )
                        rowHeights[row] = cellMinSize.y;

                    i++;
                }
            }

            minSize = Int2();

            numGrowableColumns = 0;
            numGrowableRows = 0;

            for ( size_t column = 0; column < numColumns; column++ )
            {
                minSize.x += columnWidths[column];
                
                if ( columnsGrowable[column] )
                    numGrowableColumns++;
            }

            for ( size_t row = 0; row < numRows; row++ )
            {
                minSize.y += rowHeights[row];

                if ( rowsGrowable[row] )
                    numGrowableRows++;
            }

            minSize.x += (numColumns - 1) * spacing.x;
            minSize.y += (numRows - 1) * spacing.y;

            rebuildNeeded = false;
        }

        return minSize;
    }

    void TableSizer::Layout()
    {
        GetMinSize();

        int cellWidth = 0;
        int cellHeight = 0;

        if (numGrowableColumns > 0)
            cellWidth = (size.x - minSize.x) / numGrowableColumns;
        else
        {
            if (align & HCENTER)
                this->pos.x = pos.x + (size.x - minSize.x) / 2;
            else if (align & RIGHT)
                this->pos.x = pos.x + size.x - minSize.x;
            else
                this->pos.x = pos.x;
        }

        if (numGrowableRows > 0)
            cellHeight = (size.y - minSize.y) / numGrowableRows;
        else
        {
            if (align & VCENTER)
                this->pos.y = pos.y + (size.y - minSize.y) / 2;
            else if (align & BOTTOM)
                this->pos.y = pos.y + size.y - minSize.y;
            else
                this->pos.y = pos.y;
        }

        int y = 0;

        size_t i = 0;

        for ( size_t row = 0; i < widgets.getLength(); row++ )
        {
            int rowHeight = rowHeights[row];
            
            if (rowsGrowable[row])
                rowHeight += cellHeight;

            int x = 0;

            for ( size_t column = 0; column < numColumns && i < widgets.getLength(); column++ )
            {
                int columnWidth = columnWidths[column];

                if (columnsGrowable[column])
                    columnWidth += cellWidth;

                widgets[i]->SetArea( pos + Int2( x, y ), Int2( columnWidth, rowHeight ) );

                x += columnWidth + spacing.x;
                i++;
            }

            y += rowHeight + spacing.y;
        }
    }

    void TableSizer::Move(Int2 vec)
    {
        Widget::Move(vec);
        WidgetContainer::Move(vec);
    }

    void TableSizer::Relayout()
    {
        rebuildNeeded = true;
        Layout();

        WidgetContainer::Relayout();
    }

    void TableSizer::ReloadMedia()
    {
        rebuildNeeded = true;
        WidgetContainer::ReloadMedia();
    }

    void TableSizer::SetArea(Int2 pos, Int2 size)
    {
        if ( widgets.isEmpty() || numColumns <= 0 )
            return;

        this->pos = pos;
        this->size = size;

        Layout();
    }
}