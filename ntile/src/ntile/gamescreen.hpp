#pragma once

#include "ntile.hpp"
#include "entities/entities.hpp"

#include <framework/entityhandler.hpp>
#include <framework/entityworld.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/system.hpp>

#ifdef ZOMBIE_STUDIO
#include <framework/bleb/studiobleb.hpp>
#endif

#include <zshared/mediafile2.hpp>

namespace ntile
{
    enum { PT_COUNT = 1 };

    namespace Controls
    {
        enum Controls
        {
            up,
            down,
            left,
            right,
            attack,
            block,

            screenshot,

            profiler,
            start,
            studioRefresh,

            max
        };
    }

    class MyOpenFile : public zshared::IOpenFile
    {
        const char* fileName;

            public:
                MyOpenFile(const char* fileName) : fileName(fileName) {}

                virtual unique_ptr<IOStream> OpenFile(bool readOnly, bool create) override;
    };

    class WorldCollisionHandler : public ICollisionHandler
    {
        public:
            virtual bool CollideMovementTo(IPointEntity* pe, const Float3& pos, Float3& newPos) override;
    };

    class GameScreen
            : public zfw::IScene,
            public zfw::IEntityClassListener,
            public ntile::IGameScreen
    {
        // TODO: some reordering could be done here, but we really don't want to break the readability

        protected:
            bool isTitle, editingMode;

            // Resource Management
            ResourceSection_t sectPrivate, sectEntities;

            // Blocks & world
            IPointEntity* playerNearestEntity;

            int daytimeIncr;
        
            // Set Controls
            Vkey_t controls[Controls::max];
#ifndef ZOMBIE_CTR
            gameui::StaticText* setControlsControl;
            int setControlsIndex;
#endif

            // Editing
            enum { TOOL_CIRCLE, TOOL_BLOCK };
            enum { TOOL_RAISE, TOOL_LOWER, TOOL_RANDOMIZE };

            struct
            {
                int tool, toolShape, toolRadius;
            }
            editing;

            Short2 mouseBlock, selectedBlock;
            Int3 mouseWorldPos;
            Int2 goodMousePos;      // determines block picking (not updated when mouse over UI)
            IEntity* mouseEntity;
            IEntityReflection* selectedEntity;
            List<NamedValuePtr> entityProperties;
            NamedValuePtr* editedProperty;

            bool edit_movingEntity;
            Float3 edit_entityInitialPos;
            Int2 edit_entityMovementOrigin;

            // Studio
#ifdef ZOMBIE_STUDIO
            StudioBlebManager studioBlebManager;
#endif

            // UI
#ifndef ZOMBIE_CTR
            //NUIThemer uiThemer;
            //unique_ptr<gameui::UIContainer> ui;
            gameui::Panel* editing_panel;
            gameui::TableLayout* editing_entity_properties;
            gameui::StaticText* editing_selected_block, * editing_selected_entity;
            gameui::ComboBox* editing_block_type, * editing_entity_class;
            gameui::StaticImage* editing_toolbar;
#endif

            // Scripting

            // Resources
            //IFont* font_h2, * font_title;
            //ITexture* worldTex, * headsUp;

            // Shaders
            /*IShaderProgram* worldShader;
            IShaderProgram* uiShader;
            int blend_colour;
            int sun_dir, sun_amb, sun_diff, sun_spec;
            int pt_pos[PT_COUNT], pt_amb[PT_COUNT], pt_diff[PT_COUNT], pt_spec[PT_COUNT];*/

            ///////////////////////////////////////
            //void InitUI();
            bool InitGameUI();
            int LoadMap(const char* map);
            bool StartGame();

            void p_LogResourceError();

            ///////////////////////////////////////
            bool LoadKeyBindings();
            bool SaveKeyBindings();
            bool SaveScreenshot();

            ///////////////////////////////////////
#ifndef ZOMBIE_CTR
            int Edit_CreateMap();
            int Edit_SaveMap();
            //void Edit_InitEditingUI();
            void Edit_InsertBlock(Short2 pos, int type);
            void Edit_SelectBlock(Short2 blockXY);
            void Edit_SelectEntity(IEntity* entity);
            void Edit_ToolEdit(Int3 worldPos);

            void OnEventControlUsed(gameui::EventControlUsed* payload);
            void OnEventItemSelected(gameui::EventItemSelected* payload);

            virtual int OnEntityClass(const char* className) override;

            template <bool randomize>
            void Edit_CircleTerrainChange(Int2 tileXY, int radius, int raise, int min);
#else
            virtual int OnEntityClass(const char* className) override { return -1; }
#endif

        public:
            GameScreen();
            virtual ~GameScreen() {}

            virtual void Release() { delete this; }

            virtual bool Init() override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override { return true; }
            virtual void DropResources() override {}

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
            virtual void OnTicks(int ticks) override;

//            virtual bool OnAddEntity(EntityWorld* world, IEntity* ent) override;
//            virtual void OnRemoveEntity(EntityWorld* world, IEntity* ent) override;

//            virtual void OnSetPos(IPointEntity* pe, const Float3& oldPos, const Float3& newPos) override;

            // ntile::IGameScreen
            virtual IResourceManager2* GetResourceManager() override { return g_res.get(); }
            virtual void ShowError() override { g_sys->DisplayError(g_eb, false); }

#ifndef ZOMBIE_CTR
            //virtual gameui::UIContainer* GetUI() override { return ui.get(); }
            //virtual gameui::UIThemer* GetUIThemer() override { return &uiThemer; }
#endif
    };
}
