#pragma once

#include "nbase.hpp"
#include "nanoui.hpp"

#include <framework/entity.hpp>
#include <framework/entity2.hpp>
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
#define APP_VERSION     "alpha (build 301)"

#define NOT_IMPLEMENTED() zombie_assert(false)

#undef DrawText

namespace ntile
{
    namespace entities
    {
        class char_player;
    }

    struct WorldBlock;

    extern IEngine* g_sys;
    extern ErrorBuffer_t* g_eb;

    extern unique_ptr<MessageQueue> g_msgQueue;
    extern unique_ptr<IResourceManager2> g_res;

    //extern NanoUI nui;

    extern Int2 r_pixelRes, r_mousePos;


    typedef int16_t Normal_t;

    struct World {
        int daytime = 0;        // 30 ticks = 1 min (in-game),
                                // 1800 ticks = 1 hour,
                                // 18000 ticks = 10 hours = 1 day

        intptr_t playerEntity = kInvalidEntity;
    };

    extern World g_world;

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

    class IGameScreen
    {
        public:
            virtual IResourceManager2*       GetResourceManager() = 0;
            virtual void                    ShowError() = 0;

#ifndef ZOMBIE_CTR
            //virtual gameui::UIContainer*    GetUI() = 0;
            //virtual gameui::UIThemer*       GetUIThemer() = 0;
#endif

        protected:
            ~IGameScreen() {}
    };

    IGameScreen* GetGameScreen();
    IScriptAPI* CreateNtileSquirrelAPI();

    // Offline tools go here
    int mkfont(int argc, char** argv);
}
