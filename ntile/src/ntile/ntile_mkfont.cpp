
#include "ntile.hpp"

#include <framework/filesystem.hpp>
#include <framework/utility/pixmap.hpp>

#include <bleb/repository.hpp>
#include <bleb/byteio_stdio.hpp>

#include <littl+bleb/ByteIOStream.hpp>

namespace ntile
{
    int mkfont(int argc, char** argv)
    {
        if (argc < 5)
        {
            fprintf(stderr, "usage: ntile mkfont <infile> <outfile> <charset> x1,y1,x2,y2,spacing,space_width\n");
            return -1;
        }

        ErrorBuffer::Create(g_eb);
        g_sys = CreateSystem();
        g_sys->Init(g_eb, kSysNonInteractive);

        IFSUnion* fsUnion = g_sys->GetFSUnion();
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem(".", kFSAccessStat | kFSAccessRead), 100);

        g_sys->Startup();

        Pixmap_t pm;
        Pixmap::LoadFromFile(g_sys, &pm, argv[1]);
            
        if (pm.info.format != PixmapFormat_t::RGB8 && pm.info.format != PixmapFormat_t::RGBA8)
        {
            fprintf(stderr, "mkfont: %s: must be 24-/32-bit RGBA\n", argv[1]);
            return -1;
        }

        FILE* f = fopen(argv[2], "wb+");
        zombie_assert(f);

        bleb::StdioFileByteIO bio(f, true);
        bleb::Repository repo(&bio, false);

        zombie_assert(repo.open(true));
        repo.setAllocationGranularity(512);                             // only do this after contentdir is created

        unique_ptr<bleb::ByteIO> outputBIO(repo.openStream("bmf1", bleb::kStreamCreate | bleb::kStreamTruncate));
        zombie_assert(outputBIO);
        li::ByteIOStream output(outputBIO.get());

        const char* charset = argv[3];

        int x[2], y[2];
        int spacing, space_width;

        sscanf(argv[4], "%i,%i,%i,%i,%i,%i", &x[0], &y[0], &x[1], &y[1], &spacing, &space_width);

        if (x[0] < 0)   x[0] = 0;
        if (y[0] < 0)   y[0] = 0;
        if (x[1] >= pm.info.size.x) x[1] = pm.info.size.x;
        if (y[1] >= pm.info.size.y) y[1] = pm.info.size.y;

        printf("Processing %s:(%i,%i)..(%i,%i)\n", argv[1], x[0], y[0], x[1], y[1]);

        output.writeLE<uint32_t>(0x01);
        output.writeLE<uint8_t>(strlen(charset));
        output.writeLE<uint8_t>(y[1] - y[0]);
        output.writeLE<uint8_t>(spacing);
        output.writeLE<uint8_t>(space_width);

        int xx, yy, is_blank;
        unsigned int i;
        uint16_t char_x, char_w;

        i = 0;
        is_blank = 1;

        for (xx = x[0]; xx <= x[1] && i < strlen(charset); xx++)
        {
            int col_blank;
            col_blank = 1;

            for (yy = y[0]; yy < y[1]; yy++)
            {
                register int c;

                if (pm.info.format == PixmapFormat_t::RGB8)
                    c = memcmp(&pm.pixelData[(yy * pm.info.size.x + xx) * 3], "\xFF\xFF\xFF", 3);
                else
                    c = memcmp(&pm.pixelData[(yy * pm.info.size.x + xx) * 4], "\xFF\xFF\xFF\xFF", 4);

                if (c != 0)
                {
                    col_blank = 0;
                    break;
                }
            }

            if (is_blank && !col_blank)
                char_x = xx;

            if (!is_blank && col_blank)
            {
                char_w = xx - char_x;

                ZFW_ASSERT(char_w < 255)

                output.writeLE<uint8_t>(charset[i++]);
                output.writeLE<uint8_t>((uint8_t) char_w);

                for (yy = y[0]; yy < y[1]; yy++)
                {
                    uint16_t line = 0;

                    if (pm.info.format == PixmapFormat_t::RGB8)
                    {
                        for (xx = char_x + char_w; xx >= char_x; xx--)
                            line = (line << 1) | (pm.pixelData[(yy * pm.info.size.x + xx) * 3] >> 7);
                    }
                    else
                    {
                        for (xx = char_x + char_w; xx >= char_x; xx--)
                            line = (line << 1) | (pm.pixelData[(yy * pm.info.size.x + xx) * 4] >> 7);
                    }

                    output.writeLE<uint16_t>(~line);
                }

                xx = char_x + char_w;
            }

            is_blank = col_blank;
        }

        return 0;
    }
}
