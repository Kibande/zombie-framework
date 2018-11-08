#pragma once

#include "ntile_model.hpp"

#include <RenderingKit/RenderingKit.hpp>
#include <RenderingKit/gameui/RKUIThemer.hpp>
#include <RenderingKit/utility/BasicPainter.hpp>
#include <RenderingKit/utility/CameraMouseControl.hpp>

#include <gameui/gameui.hpp>

#include <framework/entity.hpp>
#include <framework/entityworld.hpp>
#include <framework/messagequeue.hpp>
#include <framework/pointentity.hpp>
#include <framework/scene.hpp>

#include <framework/studio/recentfiles.hpp>

#define APP_NAME        "rendering-kit-demo"
#define APP_TITLE       "rendering-kit-demo"
#define APP_VENDOR      "Minexew Games"
#define APP_VERSION     "1.0"

namespace client
{
	using namespace li;
    using namespace zfw;
    using namespace RenderingKit;

    class RenderingKitDemoScene;

    struct Globals
    {
        IResourceManager* res;
    };

	extern ISystem* g_sys;
    extern ErrorBuffer_t* g_eb;
    extern MessageQueue* g_msgQueue;
    extern EntityWorld* g_world;

    extern IRenderingKit* irk;
    extern IRenderingManager* irm;
    extern IWindowManager* iwm;

    extern Globals g;

    //enum { EVENT_RK_ERROR = 0x8001 };

    //DECL_MESSAGE(EventRKError, EVENT_RK_ERROR)
    //{
    //    ErrorBuffer_t* eb;
    //};

    class editor_grid : public PointEntityBase
    {
        Float3 pos;
        Float3 cellSize;
        Int2 cells;

        RenderingKit::BasicPainter3D<> bp;

        public:
            editor_grid(RenderingKitDemoScene* rkds, const Float3& pos, const Float2& cellSize, Int2 cells);
            virtual ~editor_grid() {}

            virtual void Draw(const UUID_t* modeOrNull) override;
    };

    class RenderingKitDemoScene : public IScene, public ntile_model::IOnAnimationEnding
    {
        protected:
            // Rendering-related
            shared_ptr<IGeomBuffer> geomBuffer;
            shared_ptr<ICamera> cam2D, cam3D;
            RKCameraMouseControl cam;

            // UI
            gameui::UIContainer* ui;
            unique_ptr<IRKUIThemer> uiThemer;

            // Editor
            String path;
            unique_ptr<studio::RecentFiles> recentFiles;

            unique_ptr<ntile_model::CharacterModel> mdl;

            float timeScale;

            ntile_model::CharacterModel::Animation* selAnimation, * edAnimation;
            float selAnimationTime;

            ntile_model::Joint_t* selJoint;
            ntile_model::StudioBoneAnim_t* selBoneAnim;
            int selBoneAnimKeyframeIndex;
            ntile_model::StudioBoneAnimKeyframe_t* selBoneAnimKeyframe;

            void p_ProcessEvents();

            bool p_LoadModel();
            void p_SaveModel();
            void p_UnloadModel();

            void p_JumpToKeyframe(int dir);
            void p_SelectAnimation(ntile_model::CharacterModel::Animation* animation);
            void p_SelectJoint(const char* jointName, ntile_model::Joint_t* joint);
            void p_UpdateBoneAnimInfo();
            void p_UpdatePitchYawRollSliders();
            void p_UpdateSelectedKeyframe();
            void p_UpdateTimelineLabel();
            void pr_AddJoints(gameui::TreeBox* widget, gameui::TreeItem_t* parent, ntile_model::Joint_t* joint);

        public:
            RenderingKitDemoScene();

            virtual bool Init() override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual void DrawScene() override;
            virtual void OnFrame(double delta) override;
            virtual void OnTicks(int ticks) override;

            IGeomBuffer* GetGeomBuffer() { return geomBuffer.get(); }

            virtual void OnAnimationEnding(ntile_model::CharacterModel* mdl, ntile_model::CharacterModel::Animation* anim) override;
    };
}
