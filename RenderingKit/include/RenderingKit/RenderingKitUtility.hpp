
#include <RenderingKit/RenderingKit.hpp>

#include <framework/utility/pixmap.hpp>

namespace RenderingKit
{
    class PixmapWrapper : public IPixmap
    {
        public:
            zfw::Pixmap_t* pm;
            bool owning;

        public:
            PixmapWrapper(zfw::Pixmap_t* pm, bool owning) : pm(pm), owning(owning) {}
            ~PixmapWrapper() { if (owning) delete pm;  }

            virtual const uint8_t* GetData() override { return zfw::Pixmap::GetPixelDataForWriting(pm); }
            virtual zfw::PixmapFormat_t GetFormat() override { return pm->info.format; }
            virtual Int2 GetSize() override { return pm->info.size; }

            virtual uint8_t* GetDataWritable() override { return zfw::Pixmap::GetPixelDataForWriting(pm); }
            virtual bool SetFormat(zfw::PixmapFormat_t fmt) override{ pm->info.format = fmt; return true; }
            virtual bool SetSize(Int2 size) override { pm->info.size = size; return true; }
    };

    class PixelAtlas
    {
        struct Node
        {
            enum { FULLY_OCCUPIED = 2, DIVIDE_HOR = 4, DIVIDE_VER = 8 };
            Int2 pos, size;
            int flags;
            Node* children[2];

            ~Node();
        };

        Int2 border, align, minCell;
        Node* root;

        bool Alloc(Node* node, Int2 size, Int2& pos_out);
        Node* HorSplit(Node* node, Int2 size);
        Node* VertSplit(Node* node, Int2 size);

        PixelAtlas(const PixelAtlas&);

        public:
            PixelAtlas(Int2 size, Int2 border, Int2 alignBits, Int2 minCell);
            ~PixelAtlas();

            bool Alloc(Int2 size, Int2& pos_out);
    };
}
