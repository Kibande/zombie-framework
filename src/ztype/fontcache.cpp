
#include "ztype.hpp"

#include <littl/File.hpp>
#include <littl/String.hpp>

extern "C" {
#include "../zshared/md5.h"
}

#include <memory>

#include <png.h>

#pragma warning( disable : 4127 )

#define FONT_COLOUR 0xFFFFFF

#define MAX_TEXTURE_SIZE 2048

namespace ztype
{
    using namespace li;

    struct SourceFileHash
    {
        String src_filename;
        uint8_t md5[16];
    };

    struct UpdateEntry
    {
        String cache_filename, texture_filename, src_filename;
        FaceDesc desc;
    };

    class FontCacheImpl : public FontCache
    {
        List<SourceFileHash> sourceFileHashes;
        List<UpdateEntry> updateList;

        int padding, xspacing, yspacing;

        ProgressListener* progressListener;

        public:
            FontCacheImpl();
            virtual ~FontCacheImpl() {}

            virtual int AddToUpdateList(FaceDesc *desc, const char *output_filename, bool force, bool verbose) override;
            virtual int BuildUpdateList(FaceListFile *face_list, const char *cache_dir, bool force, bool verbose) override;
            virtual void SetProgressListener(ProgressListener* listener) override {progressListener = listener;}
            virtual int UpdateCache(bool verbose) override;

            bool CalculateFontTextureSize(const List<GlyphMetrics>& glyphMetrics, int& width, int& height, List<int>& lineHeights);
            bool IsUpToDate(const String& cache_filename, const FaceDesc* entry);
            bool RetrieveHash(const String& src_filename, uint8_t **md5);
    };

    static bool Md5File(const char *fileName, uint8_t *md5)
    {
        MD5_CTX ctx;
        char buffer[4000];

        FILE* f = fopen(fileName, "rb");

        if (f == nullptr)
            return false;

        size_t nread;

        MD5_Init(&ctx);
        while ((nread = fread(buffer, 1, sizeof(buffer), f)) > 0)
            MD5_Update(&ctx, buffer, nread);
        MD5_Final(md5, &ctx);

        fclose(f);
        return true;
    }

    static String Md5String(const char *text)
    {
        MD5_CTX ctx;
        uint8_t md5[16];
        static char string[33];

        MD5_Init(&ctx);
        MD5_Update(&ctx, (void*) text, strlen(text));
        MD5_Final(md5, &ctx);

        snprintf(string, 33, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
                md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);

        return string;
    }

    static bool SavePNG(const char *filename, const uint32_t *pixels, int width, int height)
    {
        png_infop info_ptr;

        FILE *file = fopen(filename, "wb");

        if (file == nullptr)
            return false;


        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (png_ptr == nullptr)
            return false;

        info_ptr = png_create_info_struct(png_ptr);

        if (info_ptr == nullptr)
        {
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
            png_destroy_info_struct(png_ptr, &info_ptr);
            png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
            fclose(file);
            return false;
        }

        png_init_io(png_ptr, file);
        png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        for (int y = 0; y < height; y++)
            png_write_row(png_ptr, (png_byte*) (pixels + width * y));

        png_write_end(png_ptr, NULL);

        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        png_destroy_info_struct(png_ptr, &info_ptr);
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(file);

        return true;
    }

    FontCacheImpl::FontCacheImpl()
    {
        padding = 2;
        xspacing = 2;
        yspacing = 2;
        progressListener = nullptr;
    }

    int FontCacheImpl::AddToUpdateList(FaceDesc *desc, const char *output_filename, bool force, bool verbose)
    {
        if (desc->range_max < desc->range_min)
        {
            fprintf(stderr, "[FontCache::AddToUpdateList]\tError: invalid glyph range '%i..%i' for '%s' @ %i\n", desc->range_min, desc->range_max, desc->filename, desc->size);
            return -1;
        }

        String cache_filename = (String) output_filename + ".ztype";

        if (!force && IsUpToDate(cache_filename, desc))
        {
            if (verbose)
                printf("[FontCache::AddToUpdateList]\tSkipping '%s' @ %i (%i; %i..%i)\n", desc->filename, desc->size, desc->flags, desc->range_min, desc->range_max);

            return 0;
        }

        UpdateEntry entry;
        entry.cache_filename = (String&&) cache_filename;
        entry.texture_filename = (String) output_filename + ".tex";
        entry.src_filename = desc->filename;
        entry.desc = *desc;
        entry.desc.filename = nullptr;

        if (verbose)
            printf("[FontCache::AddToUpdateList]\tAdding '%s' @ %i (%i; %i..%i)\n", desc->filename, desc->size, desc->flags, desc->range_min, desc->range_max);

        updateList.add(entry);
        return 1;
    }

    int FontCacheImpl::BuildUpdateList(FaceListFile *face_list, const char *cache_dir, bool force, bool verbose)
    {
        FaceDesc entry;

        int n = 0;

        while (face_list->ReadEntry(&entry))
            if (AddToUpdateList(&entry, GetCacheFilename(&entry, cache_dir), force, verbose) == 1)
                n++;

        return n;
    }

    bool FontCacheImpl::CalculateFontTextureSize(const List<GlyphMetrics>& glyphMetrics, int& width, int& height, List<int>& lineHeights)
    {
        width = 64;
        height = 64;

        while (true)
        {
            if (width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE)
            {
                fprintf(stderr, "[FontCache::CalculateFontTextureSize]\tError: can't find big enough texture\n");
                return false;
            }

            int x = padding;
            int y = padding;
            int line_height = 0;
            lineHeights.clear(true);

            size_t i;

            for (i = 0; i < glyphMetrics.getLength(); i++)
            {
                const GlyphMetrics& met = glyphMetrics[i];

                if (x + met.width + padding > width)
                {
                    x = padding;
                    y += line_height + yspacing;
                    lineHeights.add(line_height);
                    line_height = 0;
                }

                if (met.height > line_height)
                    line_height = met.height;

                if (y + line_height > height)
                {
                    if (width == height)
                        width *= 2;
                    else
                        height *= 2;

                    break;
                }

                x += met.width + xspacing;
            }

            if (i == glyphMetrics.getLength())
            {
                lineHeights.add(line_height);
                return true;
            }
        }
    }

    bool FontCacheImpl::IsUpToDate(const String& cache_filename, const FaceDesc *entry)
    {
        // FIXME: check ranges!!!

        std::unique_ptr<FaceFile> faceFile(FaceFile::Open(cache_filename));

        if (faceFile == nullptr)
            return false;

        uint8_t md5[16], *expected;

        if (!faceFile->ReadHashSection(md5) || !RetrieveHash(entry->filename, &expected))
            return false;

        return (memcmp(md5, expected, 16) == 0);
    }

    bool FontCacheImpl::RetrieveHash(const String& src_filename, uint8_t **md5)
    {
        iterate2 (i, sourceFileHashes)
        {
            SourceFileHash& s = i;

            if (s.src_filename == src_filename)
            {
                *md5 = s.md5;
                return true;
            }
        }

        SourceFileHash new_s;

        if (!Md5File(src_filename, new_s.md5))
            return false;

        new_s.src_filename = src_filename;

        size_t index = sourceFileHashes.add(new_s);
        *md5 = sourceFileHashes[index].md5;
        return true;
    }

    int FontCacheImpl::UpdateCache(bool verbose)
    {
        int n = 0;

        size_t num_faces = updateList.getLength();

        iterate2 (iter, updateList)
        {
            UpdateEntry& entry = iter;

            size_t curr_face = iter.getIndex();

            if (progressListener != nullptr)
                progressListener->OnFontCacheProgress((int) curr_face, (int) num_faces, 0, entry.desc.range_max - entry.desc.range_min);

            uint8_t md5[16];

            if (!Md5File(entry.src_filename, md5))
            {
                fprintf(stderr, "[FontCache::UpdateCache]\tError: failed to open '%s' @ %i\n", entry.src_filename.c_str(), entry.desc.size);
                continue;
            }

            List<GlyphMetrics> glyphMetrics;

            Rasterizer *rasterizer = Rasterizer::Create(entry.src_filename, entry.desc.size);

            if (rasterizer == nullptr)
            {
                fprintf(stderr, "[FontCache::UpdateCache]\tError: failed to create rasterizer for '%s' @ %i\n", entry.src_filename.c_str(), entry.desc.size);
                continue;
            }

            uint32_t numGlyphs = entry.desc.range_max - entry.desc.range_min + 1;

            for (uint32_t i = 0; i < numGlyphs; i++)
            {
                const GlyphMetrics *met;

                if (!rasterizer->GetChar(entry.desc.range_min + i, Rasterizer::GETCHAR_METRICS, nullptr, &met))
                {
                    if (verbose)
                        fprintf(stderr, "[FontCache::UpdateCache]\tFailed to render glyph U+%X (%c) for '%s' @ %i\n", entry.desc.range_min + i, entry.desc.range_min + i,
                                entry.src_filename.c_str(), entry.desc.size);
                }
                else
                    glyphMetrics.add(*met);
            }

            int texWidth, texHeight;
            List<int> lineHeights;

            if (!CalculateFontTextureSize(glyphMetrics, texWidth, texHeight, lineHeights))
                continue;

            if (verbose)
                fprintf(stderr, "[FontCache::UpdateCache]\t'%s' @ %i: using %ix%i texture (%lu raw bytes)\n", entry.src_filename.c_str(), entry.desc.size,
                        texWidth, texHeight, (unsigned long)(texWidth * texHeight * sizeof(uint32_t)));

            File output(entry.cache_filename, true);

            if (!output)
            {
                fprintf(stderr, "[FontCache::UpdateCache]\tError: failed to open file '%s' for writing\n", entry.cache_filename.c_str());
                continue;
            }

            output.writeLE<uint32_t>(0xCECA87F0);

            // 3 sections at fixed positions (Hash: 8 + 24, Metr: 8 + 24 + 16, FTex: 8 + 24 + 16 + 8)
            output.writeLE<uint32_t>(3);

            output.write("Hash", 4);
            output.writeLE<uint32_t>(32);

            output.write("Metr", 4);
            output.writeLE<uint32_t>(48);

            output.write("FTex", 4);
            output.writeLE<uint32_t>(56);

            // Hash section
            output.write(md5, 16);

            // Metr section
            output.write(rasterizer->GetMetrics(), sizeof(FaceMetrics));

            // FTex section
            // only 1 range is currently supported
            // this will need to be changed eventually
            output.writeLE<uint32_t>(1);
            output.writeLE<uint32_t>(entry.desc.range_min);
            output.writeLE<uint32_t>(numGlyphs);

            // Build font texture in memory
            uint32_t *pixels = (uint32_t *) malloc(texWidth * texHeight * sizeof(uint32_t));
            memset(pixels, 0, texWidth * texHeight * sizeof(uint32_t));

            // Layout all the characters
            int x = padding;
            int y = padding;

            size_t line = 0;

            CharEntry charEntry;
            memset(&charEntry, 0, sizeof(charEntry));

            for (uint32_t i = 0; i < numGlyphs; i++)
            {
                const GlyphBitmap *bmp;
                const GlyphMetrics *met;

                if (progressListener != nullptr && !(i & 0x1F))
                    progressListener->OnFontCacheProgress((int) curr_face, (int) num_faces, i, numGlyphs);

                if (rasterizer->GetChar(entry.desc.range_min + i, Rasterizer::GETCHAR_METRICS | Rasterizer::GETCHAR_RENDER, &bmp, &met))
                {
                    if (x + met->width + padding > texWidth)
                    {
                        x = padding;
                        y += lineHeights[line] + yspacing;
                        line++;
                    }

                    for (int yy = 0; yy < bmp->height; yy++)
                    {
                        uint8_t *in = bmp->pixels + bmp->pitch * yy;
                        uint32_t *out = pixels + texWidth * (y + lineHeights[line] - bmp->height + yy) + x;

                        for (int xx = 0; xx < bmp->width; xx++)
                            *out++ = FONT_COLOUR | ((*in++) << 24);
                    }

                    charEntry.uv[0] = (float) x / texWidth;
                    charEntry.uv[1] = (float) (y + lineHeights[line] - bmp->height) / texHeight;
                    charEntry.uv[2] = (float) (x + bmp->width) / texWidth;
                    charEntry.uv[3] = (float) (y + lineHeights[line]) / texHeight;

                    charEntry.width = met->width;
                    charEntry.height = met->height;
                    charEntry.draw_x = met->x;
                    charEntry.draw_y = met->y;
                    charEntry.advance = met->advance;

                    x += met->width + xspacing;
                }
                else
                {
                    memset(&charEntry, 0, sizeof(charEntry));
                }

                output.write(&charEntry, sizeof(charEntry));
            }

            delete rasterizer;

            if (!SavePNG(entry.texture_filename, pixels, texWidth, texHeight))
            {
                fprintf(stderr, "[FontCache::UpdateCache]\tError: failed to save file '%s'\n", entry.texture_filename.c_str());
                free(pixels);
                continue;
            }

            free(pixels);

            n++;
        }

        return n;
    }

    FontCache* FontCache::Create()
    {
        return new FontCacheImpl();
    }

    const char* FontCache::GetCacheFilename(FaceDesc *desc, const char *cache_dir)
    {
        static String filename;

        if (desc->cache_filename != nullptr)
            return desc->cache_filename;

        filename = ((cache_dir != nullptr) ? ((String) cache_dir + "/") : String())
                + Md5String(desc->filename)+ "_" + desc->size + "_" + desc->flags;

        return filename.c_str();
    }
}
