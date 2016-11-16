#pragma once

#include "nbase.hpp"
#include "nanoui.hpp"
#include "n3d.hpp"

#include <framework/entity.hpp>
#include <framework/event.hpp>
#include <framework/interpolator.hpp>
#include <framework/messagequeue.hpp>
#include <framework/scene.hpp>

#ifndef ZOMBIE_CTR
#include <gameui/gameui.hpp>
#endif

#include <littl/List.hpp>
#include <littl/String.hpp>

#define APP_TITLE       "nanotile: Quest of Kyria"
#define APP_VENDOR      "Minexew Games"
#define APP_VERSION     "alpha (build 201)"

#undef DrawText

namespace ntile
{
    namespace entities
    {
        class char_player;
    }

    struct WorldBlock;

    extern ISystem* g_sys;
    extern ErrorBuffer_t* g_eb;

    extern unique_ptr<IPlatform> iplat;
    extern IRenderer* ir;
    extern unique_ptr<MessageQueue> g_msgQueue;
    extern unique_ptr<IResourceManager2> g_res;

    extern NanoUI nui;

    extern Int2 r_pixelRes, r_mousePos;

    extern unique_ptr<IVertexFormat> g_modelVertexFormat, g_worldVertexFormat;

    extern Int2 worldSize;
    extern WorldBlock* blocks;

    struct WorldTile
    {
        uint8_t type;
        uint8_t flags;
        int16_t elev;
        uint8_t material;
        uint8_t colour[3];
    };

    struct WorldBlock
    {
        int type;
        unique_ptr<IVertexBuffer> vertexBuf;
        uint32_t pickingColour;

        List<IPointEntity*> entities;

        // 2k
        WorldTile tiles[TILES_IN_BLOCK_V][TILES_IN_BLOCK_H];
    };

    typedef int16_t Normal_t;

#ifdef ZOMBIE_CTR
    struct WorldVertex
    {
        float x, y, z;
        float u, v;
        int16_t n[4];
        uint8_t rgba[4];
    };

    struct ModelVertex
    {
        float x, y, z;
        float u, v;
        int16_t n[4];
        uint8_t rgba[4];
    };
#else
    struct WorldVertex
    {
        int32_t x, y, z;
        int16_t n[4];
        uint8_t rgba[4];
        float u, v;
    };

    struct ModelVertex
    {
        float x, y, z;
        float u, v;
        int16_t n[4];
        uint8_t rgba[4];
    };
#endif
    
    static_assert(sizeof(ModelVertex) == 32,        "ModelVertex size");
    static_assert(sizeof(WorldTile) == 8,           "WorldTile size must be 8");
    static_assert(sizeof(WorldVertex) == 32,        "WorldVertex size");

#ifndef ZOMBIE_CTR
    class NUIThemer : public gameui::UIThemer
    {
        friend class NButtonThemer;
        friend class NCheckBoxThemer;
        friend class NComboBoxThemer;
        friend class NPanelThemer;
        friend class NStaticImageThemer;
        friend class NStaticTextThemer;
        friend class NTextBoxThemer;

        protected:
            struct FontEntry
            {
                FontEntry() {}
                FontEntry(const FontEntry& other) = delete;

                const FontEntry& operator =(FontEntry&& other)
                {
                    name = move(other.name);
                    path = move(other.path);
                    size = other.size;
                    flags = other.flags;
                    font = move(other.font);
                    return *this;
                }

                String name;
                String path;
                int size, flags;
                IFont* font;
            };

            List<FontEntry> fonts;

        public:
            ~NUIThemer() { DropResources(); }

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual intptr_t PreregisterFont(const char* name, const char* params) override;
            virtual intptr_t GetFontId(const char* font) override;

            virtual gameui::IButtonThemer*      CreateButtonThemer(Int3 pos, Int2 size, const char* label) override;
            virtual gameui::ICheckBoxThemer*    CreateCheckBoxThemer(Int3 pos, Int2 size, const char* label, bool checked) override;
            virtual gameui::IComboBoxThemer*    CreateComboBoxThemer(Int3 pos, Int2 size) override;
            virtual gameui::IGraphicsThemer*    CreateGraphicsThemer(shared_ptr<IGraphics> g) override;
            virtual gameui::IListBoxThemer*     CreateListBoxThemer() override { return nullptr; }
            virtual gameui::IMenuBarThemer*     CreateMenuBarThemer() override { return nullptr; }
            virtual gameui::IPanelThemer*       CreatePanelThemer(Int3 pos, Int2 size) override;
            virtual gameui::IScrollBarThemer*   CreateScrollBarThemer(bool horizontal) override { return nullptr; }
            virtual gameui::ISliderThemer*      CreateSliderThemer() override { return nullptr; }
            virtual gameui::IStaticImageThemer* CreateStaticImageThemer(Int3 pos, Int2 size, const char* path) override;
            virtual gameui::IStaticTextThemer*  CreateStaticTextThemer(Int3 pos, Int2 size, const char* label) override;
            virtual gameui::ITextBoxThemer*     CreateTextBoxThemer(Int3 pos, Int2 size) override;
            virtual gameui::ITreeBoxThemer*     CreateTreeBoxThemer(Int3 pos, Int2 size) override { return nullptr; }
            virtual gameui::IWindowThemer*      CreateWindowThemer(gameui::IPanelThemer* thm, int flags) override;

            IFont* GetFont(intptr_t fontIndex) { return (fontIndex >= 0 && (uintptr_t) fontIndex < fonts.getLength()) ? fonts[fontIndex].font : fonts[0].font; }
    };
#endif

    class Blocks
    {
        public:
            static void AllocBlocks(Int2 size, bool copyOld = false, Int2 copyOffset = Int2());
            static void ReleaseBlocks(WorldBlock*& blocks, Int2 worldSize);
            
            static void InitBlock(WorldBlock* block, int bx, int by);
            static void GenerateTiles(WorldBlock* block);
            static void ResetBlock(WorldBlock* block, int bx, int by);

            static void InitAllTiles(Short2 blockXY, WorldVertex* p_vertices);
            static void UpdateAllTiles(Short2 blockXY, WorldVertex* p_vertices);
            static void UpdateTile(WorldTile* tile, WorldTile* tile_east, WorldTile* tile_south, WorldVertex*& p_vertices);
    };

    class IGameScreen
    {
        public:
            virtual IResourceManager2*       GetResourceManager() = 0;
            virtual void                    ShowError() = 0;

#ifndef ZOMBIE_CTR
            virtual gameui::UIContainer*    GetUI() = 0;
            virtual gameui::UIThemer*       GetUIThemer() = 0;
#endif

        protected:
            ~IGameScreen() {}
    };

    IGameScreen* GetGameScreen();
    IScriptAPI* CreateNtileSquirrelAPI();

    // Utility functions
    inline Short2 WorldToBlockXY(Float2 worldPos)
    {
        return Short2((worldPos - Float2(TILE_SIZE_H * 0.5f, TILE_SIZE_V * 0.5f))
                * Float2(1.0f / (TILES_IN_BLOCK_H * TILE_SIZE_H), 1.0f / (TILES_IN_BLOCK_V * TILE_SIZE_V)));
    }

    inline Int2 WorldToTileXY(Float2 worldPos)
    {
        return Int2((worldPos - Float2(TILE_SIZE_H * 0.5f, TILE_SIZE_V * 0.5f))
                * Float2(1.0f / (TILES_IN_BLOCK_H * TILE_SIZE_H), 1.0f / (TILES_IN_BLOCK_V * TILE_SIZE_V)));
    }

    // Offline tools go here
    int mkfont(int argc, char** argv);
}
