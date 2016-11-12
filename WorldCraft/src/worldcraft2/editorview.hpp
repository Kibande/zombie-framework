#pragma once

#include "session.hpp"
#include "world.hpp"

#include <framework/pointentity.hpp>

namespace worldcraftpro
{
    using namespace RenderingKit;
    using namespace zfw;

    enum
    {
        MS_PAN_VIEW,
        MS_ROTATE_VIEW,
        MS_ZOOM_VIEW
    };

    enum EditorMode
    {
        MODE_IDLE,
        MODE_DRAW_BRUSH
    };

    enum ViewSetup_t
    {
        kViewSetup1,
        kViewSetup2_2,
    };

    /*
    void CharacterEntity::GetViewCamera(world::Camera& cam)
    {
        //cam = Camera();
        cam.eye = pos + Float3(0.0f, 0.0f, height);
        cam.center = cam.eye + orientation;
        cam.up = Float3(0.0f, 0.0f, 1.0f);      //FIXME
    }
    */

    class Actor : public PointEntityBase
    {
        protected:
            float height;
    };

    class player_sandbox : public Actor
    {
        public:
            static IEntity* Create() { return new player_sandbox; }

            player_sandbox();
    };

    class IEditorView
    {
        public:
            virtual ~IEditorView() {}

            virtual bool Init(ISystem* sys, IRenderingKit* rk, IRenderingManager* rm,
                    shared_ptr<IResourceManager> brushTexMgrRef) = 0;
            virtual void Shutdown() = 0;

            virtual bool AcquireResources() = 0;

            virtual void Draw() = 0;
            virtual void OnFrame(double delta) = 0;
            virtual int OnMouseButton(int h, int button, bool pressed, int x, int y) = 0;
            virtual int OnMouseMove(int h, int x, int y) = 0;

            virtual bool Load(InputStream* stream) = 0;
            virtual bool Serialize(OutputStream* stream) = 0;

            // must be followed by OnMouseButton
            virtual void BeginDrawBrush(const char* material) = 0;

            virtual void SelectAt(int x, int y, int mode) = 0;

            virtual void SetViewSetup(ViewSetup_t viewSetup) = 0;

            /*
            void DropResources();

            void BeginDrawBrush(const char* texture);*/
            
            //bool Load(InputStream* stream);
            /*int OnMouseButton(int button, int flags, int x, int y);
            void OnMouseMove(int x, int y);
            void SelectAt(int x, int y, int mode);
            bool Serialize(OutputStream* stream, int format = 0);

            void SetSession(IEditorSession *session);

            virtual void OnSessionMessage(int clientId, uint32_t id, ArrayIOStream& message) override;

            void AddBrush(int clientId, const char* name, const Float3& pos, const Float3& size, const char* material);
            void AddEntity(int clientId, const char* name, const char* entdef, const Float3& pos);
            void AddStaticModel(int clientId, const char* name, const Float3& pos, const char* path);

            IEntity* FindEntity(const char* entdef);
            void TransitionToView(ViewType viewType, IEntity* controlEnt);*/
    };

    IEditorView* CreateEditorView();
}
