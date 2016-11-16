#pragma once

#include "editorview.hpp"
#include "session.hpp"

#include <framework/event.hpp>
#include <framework/graphics.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/scene.hpp>

#include <framework/studio/recentfiles.hpp>

#include <gameui/gameui.hpp>

#include <RenderingKit/RenderingKit.hpp>
#include <RenderingKit/gameui/RKUIThemer.hpp>

#include <StudioKit/texturecache.hpp>

#define APP_NAME        "worldcraft2"
#define APP_TITLE       "worldcraft2"
#define APP_VENDOR      "Minexew Games"
#define APP_VERSION     "2.0"

namespace worldcraftpro
{
    using namespace RenderingKit;
    using namespace zfw;

    using StudioKit::ITexturePreviewCache;

    enum {
        TOOLBAR_SELECT          = 0,
        TOOLBAR_MOVE            = 1,
        TOOLBAR_BUILD_ADD       = 4,
        TOOLBAR_STATICMDL_ADD   = 6
    };

    static const size_t CACHED_THUMBNAILS_MAX = 1000;

    struct Globals
    {
        ErrorBuffer_t*      eb;

        shared_ptr<MessageQueue> msgQueue;
        unique_ptr<IResourceManager> res;
        World*              world;

        unique_ptr<IRenderingKit> rk;
        IRenderingManager*  rm;
        IWindowManager*     wm;
    };

    extern Globals g;

    extern ISystem*        g_sys;

    /*
    class ModelPreviewCache
    {
        public:
            struct CompletedModelPreview
            {
                zr::ITexture* texture;
            };

            class RetrieveCallback
            {
                public:
                    virtual void ReceiveModelPreview(int request, CompletedModelPreview* preview) = 0;
            };

        public:
            static ModelPreviewCache* Create();
            virtual ~ModelPreviewCache() {}

            // Any thread
            virtual void AddSize(int a) = 0;
            virtual int RequestModelPreview(const char* modelFileName) = 0;
            virtual void RetrieveModelPreviews(RetrieveCallback* cb) = 0;

            // Main (rendering) thread
            virtual void OnFrameDraw(int timelimitMsec) = 0;
    };
    */

    struct BuiltInBrush_t
    {
        const char* name;
        const char* label;
    };

    class WorldcraftScene: public IScene, public IResourceProvider,
            public std::enable_shared_from_this<WorldcraftScene>
    {
        ITexturePreviewCache* texPreviewCache;

        // UI
        unique_ptr<RenderingKit::IRKUIThemer> uiThemer;
        unique_ptr<gameui::UIContainer> ui;
        
        gameui::Window *contentMgr, *materialPanel, *menu;
        gameui::Window *materialMenu, *exportMenu;
        unique_ptr<studio::RecentFiles> recentFiles;

        int toolbarSelection;

        //
        unique_ptr<IEditorView> editorView;
        
        List<BuiltInBrush_t> builtInBrushes;
        String currentMaterialName;

        unique_ptr<IEditorSession> session;

        protected:
            void CreateUI();
            String GetTextureForMaterial(const String& materialName);
            void PositionToolbarSelectionShadow();
            void SetMaterial(const String& name);
            int TryUIEvent(const MessageHeader* event);

            shared_ptr<ITexture> p_CreateBrushTexture(IResourceManager* res, const char* label);

            bool p_LoadDocument();
            void p_NewDocument();
            bool p_SaveDocument();
            void p_UnloadDocument();

            bool p_ExportWorld();

        public:
            WorldcraftScene(ITexturePreviewCache* texPreviewCache);

            virtual bool Init() override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;

            // zfw::IResourceProvider
            virtual shared_ptr<IResource>   CreateResource(IResourceManager* res, const std::type_index& resourceClass, const char* normparams, int flags) override;
            virtual bool        DoParamsAlias(const std::type_index& resourceClass, const char* params1, const char* params2) override { return false; }
            virtual const char* TryGetResourceClassName(const std::type_index& resourceClass) override;

        private:
            shared_ptr<ITexture> errorTex;
            shared_ptr<IResourceManager> brushTexMgr;

            String path;
    };

    //
    void compileMain(int argc, char** argv);
}
