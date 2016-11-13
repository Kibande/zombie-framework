#pragma once

#include "mediafile.hpp"

namespace zshared
{
    enum {IOMESH_INDEXED = 1, IOMESH_LINEPRIM = 2};
    enum {IOFMT_VERTEX3F = 1, IOFMT_NORMAL3F = 2, IOFMT_UV2F = 4};

    struct MeshDataHeader
    {
        int ioFlags;
        int fmtFlags;
        uint32_t numVertices;
        uint32_t numIndices;
    };
}