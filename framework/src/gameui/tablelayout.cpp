
#include <gameui/gameui.hpp>
#include <framework/utility/essentials.hpp>
#include <framework/utility/util.hpp>

// Uncomment to enable performance timing in this unit
//#define TABLE_PERFTIMING

#ifdef TABLE_PERFTIMING
#include <littl/PerfTiming.hpp>

#define Perf_begin() PerfTimer tm; auto begin = tm.getCurrentMicros()
#define Perf_end(text) auto end = tm.getCurrentMicros(); Sys::printk("Table: %s took %d us.", text, (int)(end - begin))
#define Perf_note(text) Sys::printk(text)
#else
#define Perf_begin()
#define Perf_end(text)
#define Perf_note(text)
#endif

namespace gameui
{
    using namespace li;

    static const Int2 default_spacing(5, 5);

    TableLayout::TableLayout(unsigned int numColumns)
            : numColumns(numColumns), minSizeValid(false), layoutValid(false)
    {
        numRows = 0;
        spacing = default_spacing;

        columnsGrowable = Allocator<bool>::allocate(numColumns);

        numGrowableColumns = 0;
        numGrowableRows = 0;

        columnMetrics.resize(numColumns, false);
    }

    TableLayout::~TableLayout()
    {
        Allocator<bool>::release(columnsGrowable);
    }

    bool TableLayout::AcquireResources()
    {
        minSizeValid = false;
        layoutValid = false;

        return WidgetContainer::AcquireResources();
    }

    void TableLayout::Add(Widget* widget)
    {
        ZFW_ASSERT(numColumns > 0)

        if (widget == nullptr)
            return;

        ZFW_ASSERT(!widget->GetFreeFloat())

        // Did you know about this? :P
        const div_t res = div((int) widgets.getLength(), (int) numColumns);

        // Not sure how good compilers are on detecting local variable renaming
#define row     ((unsigned int) res.quot)
#define column  ((unsigned int) res.rem)

        ZFW_ASSERT(row >= 0)
        ZFW_ASSERT(column >= 0)

        widget->SetParent(this);
        widget->SetParentContainer(this);
        widget->SetPosInTable(Short2(row, column));
        widgets.add(widget);

        if (ctnOnlineUpdate)
        {
            if (minSizeValid)
            {
                ZFW_ASSERT(!(row > numRows))

                // A little set-up is needed when inserting into a new row
                if (row == numRows)
                {
                    if (numRows != 0)
                        minSize.y += spacing.y;

                    numRows = row + 1;
                    rowMetrics.resize(li::align<4>(numRows), false);
                    rowMetrics[row].minSize = 0;

                    if ( rowsGrowable[row] )
                        numGrowableRows++;
                }

                // Don't miss this one!
                if (row == 0)
                {
                    if ( columnsGrowable[column] )
                        numGrowableColumns++;
                }
                
                OnlineUpdateForWidget(widget);
            }
        }
        else
            // Online update not allowed; assume worst case
            minSizeValid = false;

#undef row
#undef column
    }

    Int2 TableLayout::GetMinSize()
    {
        // "Layout(0)"

        if (minSizeValid)
            return minSize;

        minSize = Int2();

        // Determine the table height (division with rounding up)
        numRows = ((unsigned int) widgets.getLength() + numColumns - 1) / numColumns;

        numGrowableColumns = 0;
        numGrowableRows = 0;

        if (widgets.getLength() == 0)
        {
            Perf_note("Table: Layout(0): no widgets, bailing out");
            minSizeValid = true;
            return minSize;
        }

        Perf_begin();

        rowMetrics.resize(li::align<4>(numRows), false);

        size_t i = 0;

        // Go through the whole table and calculate min. sizes for all rows & columns
        // minSize.y is adjsuted after each row
        for ( unsigned int row = 0; i < widgets.getLength(); row++ )
        {
            RowCol_t* p_row = &rowMetrics[row];
            RowCol_t* p_column = &columnMetrics[0];

            p_row->minSize = 0;

            // Make sure column minSizes are initialized
            if (row == 0)
            {
                for ( unsigned int column = 0; column < numColumns; column++ )
                {
                    p_column->minSize = 0;
                    p_column++;
                }

                p_column = &columnMetrics[0];
            }

            for ( unsigned int column = 0; column < numColumns && i < widgets.getLength(); column++ )
            {
                const Int2 cellMinSize = widgets[i]->GetMinSize();

                if ( cellMinSize.x > p_column->minSize )
                    p_column->minSize = cellMinSize.x;

                if ( cellMinSize.y > p_row->minSize )
                    p_row->minSize = cellMinSize.y;

                i++;
                p_column++;
            }

            minSize.y += p_row->minSize;

            if ( rowsGrowable[row] )
                numGrowableRows++;
        }

        // One additional pass to calculate minSize.x
        RowCol_t* p_column = &columnMetrics[0];

        for ( unsigned int column = 0; column < numColumns; column++ )
        {
            minSize.x += p_column->minSize;
                
            if ( columnsGrowable[column] )
                numGrowableColumns++;

            p_column++;
        }

        // Finally add the inter-cell spacing
        minSize.x += (numColumns - 1) * spacing.x;
        minSize.y += (numRows - 1) * spacing.y;

        minSizeValid = true;

        Perf_end("Layout(0)");

        return minSize;
    }

    unsigned int TableLayout::GetNumRows() const
    {
        // Determine the table height (division with rounding up)
        return ((unsigned int) widgets.getLength() + numColumns - 1) / numColumns;
    }

    void TableLayout::Layout()
    {
        // "Layout(1)"

        if (layoutValid)
            return;

        Perf_begin();

        const Int2 minSize = GetMinSize();

        // Additional space available to each growable {column, row}
        // This will be added to the value in {columnMinWidths, rowMinHeights}
        int columnPadding = 0;
        int rowPadding = 0;

        if (numGrowableColumns > 0)
        {
            pos.x = areaPos.x;
            size.x = areaSize.x;
            columnPadding = (size.x - minSize.x) / numGrowableColumns;
        }
        else
        {
            if (align & ALIGN_HCENTER)
                pos.x = areaPos.x + (areaSize.x - minSize.x) / 2;
            else if (align & ALIGN_RIGHT)
                pos.x = areaPos.x + areaSize.x - minSize.x;
            else
                pos.x = areaPos.x;
        }

        if (numGrowableRows > 0)
        {
            pos.y = areaPos.y;
            size.y = areaSize.y;
            rowPadding = (size.y - minSize.y) / numGrowableRows;
        }
        else
        {
            if (align & ALIGN_VCENTER)
                pos.y = areaPos.y + (areaSize.y - minSize.y) / 2;
            else if (align & ALIGN_BOTTOM)
                pos.y = areaPos.y + areaSize.y - minSize.y;
            else
                pos.y = areaPos.y;
        }

        int y = 0;

        size_t i = 0;

        // Perform SetArea for all widgets, also update {column, row} metrics for later online updates
        for ( unsigned int row = 0; i < widgets.getLength(); row++ )
        {
            RowCol_t* p_row = &rowMetrics[row];
            RowCol_t* p_column = &columnMetrics[0];

            p_row->offset = y;
            p_row->actualSize = p_row->minSize;
            
            if (rowsGrowable[row])
                p_row->actualSize += rowPadding;

            if (row == 0)
            {
                // Calculate column metrics on the first row only
                int x = 0;

                for ( unsigned int column = 0; column < numColumns; column++ )
                {
                    p_column->actualSize = p_column->minSize;

                    if (columnsGrowable[column])
                        p_column->actualSize += columnPadding;

                    p_column->offset = x;
                    x += p_column->actualSize + spacing.x;

                    p_column++;
                }

                p_column = &columnMetrics[0];
            }

            for ( unsigned int column = 0; column < numColumns && i < widgets.getLength(); column++ )
            {
                widgets[i]->SetArea(Int3(pos.x + p_column->offset, pos.y + y, pos.z), Int2(p_column->actualSize, p_row->actualSize));

                i++;
                p_column++;
            }

            y += p_row->actualSize + spacing.y;
        }

        layoutValid = true;

        Perf_end("Layout(1)");
    }

    void TableLayout::Move(Int3 vec)
    {
        Widget::Move(vec);
        WidgetContainer::Move(vec);
    }

    void TableLayout::OnlineUpdateForWidget(Widget* widget)
    {
        const unsigned short int row = widget->GetPosInTable().x;
        const unsigned short int column = widget->GetPosInTable().y;

        Perf_begin();

        RowCol_t* p_row =    &rowMetrics[row];
        RowCol_t* p_column = &columnMetrics[column];

        const Int2 cellMinSize = widget->GetMinSize();

        bool minSizeChanged = false;

        // Will be implicitly triggered when inserting into a new row,
        // because Add() will set its minSize to 0
        if ( cellMinSize.y > p_row->minSize )
        {
            minSize.y += cellMinSize.y - p_row->minSize;
            minSizeChanged = true;
            layoutValid = false;

            p_row->minSize = cellMinSize.y;
        }

        if ( cellMinSize.x > p_column->minSize )
        {
            minSize.x += cellMinSize.x - p_column->minSize;
            minSizeChanged = true;
            layoutValid = false;

            p_column->minSize = cellMinSize.x;
        }
        
        // If MinSize didn't change and our layout was valid until now,
        // we can just perform SetArea on the updated widget and be done
        if (layoutValid)
            widget->SetArea(Int3(pos.x + p_column->offset, pos.y + p_row->offset, pos.z), Int2(p_column->actualSize, p_row->actualSize));

        // Else notify the parent, as it will likely need to resize itself as well
        if ((!layoutValid || minSizeChanged) && parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);

        Perf_end("OnlineUpdate");
    }

    void TableLayout::OnWidgetMinSizeChange(Widget* widget)
    {
        if (ctnOnlineUpdate)
        {
            if (minSizeValid)
                OnlineUpdateForWidget(widget);
        }
        else
            // Online update not allowed; assume worst case
            minSizeValid = false;
    }

    void TableLayout::RemoveAll()
    {
        WidgetContainer::RemoveAll();

        layoutValid = false;
        minSizeValid = false;

        for (unsigned int col = 0; col < numColumns; col++)
            columnMetrics[col].minSize = 0;

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);
    }

    void TableLayout::SetArea(Int3 pos, Int2 size)
    {
        if (size == areaSize)
        {
            Move(pos - areaPos);
            areaPos = pos;
        }
        else
            layoutValid = false;

        // We _DO_ want Layout to be always called.
        // Scenario:    this->Add() called, online update enabled
        //              minSize changed, don't position widget, notify parent
        //              parents calls back with same size
        Widget::SetArea(pos, size);
    }

    void TableLayout::SetNumColumns(unsigned int numColumns)
    {
        ZFW_ASSERT(numColumns > 0)

        Allocator<bool>::release(columnsGrowable);

        this->numColumns = numColumns;

        columnsGrowable = Allocator<bool>::allocate(numColumns);

        numGrowableColumns = 0;
        numGrowableRows = 0;

        columnMetrics.resize(numColumns, false);

        size_t i = 0;

        for ( unsigned int row = 0; i < widgets.getLength(); row++ )
            for ( unsigned int column = 0; column < numColumns; column++ )
                widgets[i++]->SetPosInTable(Short2(row, column));

        if (ctnOnlineUpdate)
        {
            if (minSizeValid)
            {
                minSizeValid = false;

                if (parentContainer != nullptr)
                    parentContainer->OnWidgetMinSizeChange(this);
            }
        }
        else
            minSizeValid = false;
    }
}
