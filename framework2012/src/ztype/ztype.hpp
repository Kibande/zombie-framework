#pragma once

#include <stddef.h>
#include <stdint.h>

namespace li
{
    class InputStream;
}

namespace ztype
{
    enum FaceStyleFlags_
    {
        FACE_BOLD = 1,
        FACE_ITALIC = 2
    };

    struct CharEntry
    {
        float uv[4];
        uint16_t width, height;
        int16_t draw_x, draw_y;
        int16_t advance;
        int16_t reserved[3];
    };

    struct FaceDesc
    {
        const char *filename, *cache_filename;
        int size, flags;
        int range_min, range_max;
    };

    struct FaceMetrics
    {
        int16_t ascent;
        int16_t descent;
        int16_t lineheight;
        int16_t reserved;
    };

    struct GlyphBitmap
    {
        uint16_t width, height;
        size_t pitch;
        uint8_t *pixels;
    };

    struct GlyphMetrics
    {
        uint16_t width, height;
        int16_t x, y;
        int16_t advance;
    };

    class FaceListFile
    {
        public:
            static FaceListFile* Open(const char *file_name);
            static FaceListFile* Open(li::InputStream* stream);
            virtual ~FaceListFile() {}

            virtual bool ReadEntry(FaceDesc *desc) = 0;
    };

    class FaceFile
    {
        public:
            static FaceFile* Open(const char *file_name);
            virtual ~FaceFile() {}

            virtual bool GetRanges(size_t *num_ranges, const uint32_t **ranges) = 0;
            virtual bool ReadFTexData(uint32_t cp_min, uint32_t count, CharEntry *entries) = 0;
            virtual bool ReadHashSection(uint8_t *md5) = 0;
            virtual bool ReadMetrSection(FaceMetrics *metrics) = 0;
    };

    class FontCache
    {
        public:
            class ProgressListener
            {
                public:
                    virtual void OnFontCacheProgress(int faces_done, int faces_total, int glyphs_done, int glyphs_total) = 0;
            };

            static FontCache* Create();
            virtual ~FontCache() {}

            static const char* GetCacheFilename(FaceDesc *desc, const char *cache_dir);

            virtual int AddToUpdateList(FaceDesc *desc, const char *output_filename, bool force, bool verbose) = 0;
            virtual int BuildUpdateList(FaceListFile *face_list, const char *cache_dir, bool force, bool verbose) = 0;
            virtual void SetProgressListener(ProgressListener* listener) = 0;
            virtual int UpdateCache(bool verbose) = 0;
    };

    class Rasterizer
    {
        public:
            enum {GETCHAR_RENDER = 1, GETCHAR_METRICS = 2};

            static Rasterizer* Create(const char *ttfFile, int pxsize);
            virtual ~Rasterizer() {}

            virtual bool GetChar(uint32_t cp, int flags, const GlyphBitmap** bitmap_ptr, const GlyphMetrics** metrics_ptr) = 0;
            virtual const FaceMetrics* GetMetrics() = 0;
    };
}
