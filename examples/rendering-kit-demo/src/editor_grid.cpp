
#include "RenderingKitDemo.hpp"

#include <framework/colorconstants.hpp>

namespace client
{
    editor_grid::editor_grid(RenderingKitDemoScene* rkds, const Float3& pos, const Float2& cellSize, Int2 cells)
            : pos(pos), cellSize(cellSize.x, cellSize.y, 0), cells(cells)
    {
        bp.Init(irm);
    }

    void editor_grid::Draw(const UUID_t* modeOrNull)
    {
        bp.DrawGridAround(pos, cellSize, cells, RGBA_WHITE);
    }
}
