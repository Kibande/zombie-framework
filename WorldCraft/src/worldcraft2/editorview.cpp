
#include "editorview.hpp"

#include <RenderingKit/utility/BasicPainter.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

#include <framework/colorconstants.hpp>
#include <framework/event.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/system.hpp>
#include <framework/utility/algebrahelpers.hpp>

#include <littl/File.hpp>

#undef DrawText

namespace worldcraftpro
{
    enum { kMaxViewports = 4 };

    class EntityNodeSearch : public INodeSearch
    {
        public:
            const char* entdef;
            EntityNode* entNode;

        public:
            EntityNodeSearch(const char* entdef) : entdef(entdef), entNode(nullptr)
            {
            }

            virtual bool TestNode(Node* node) override
            {
                entNode = dynamic_cast<EntityNode*>(node);

                if (entNode != nullptr && strcmp(entNode->GetEntdef(), entdef) != 0)
                    entNode = nullptr;

                return (entNode != nullptr);
            }
    };

    struct Viewport_t
    {
        unsigned int index;
        Int2 pos, size;

        bool ready;
        glm::mat4x4 projection, unprojection;
    };

    class EditorView : public IEditorView, public ISessionMessageListener
    {
        // local assets
        unique_ptr<World> world;

        EditorMode m;
        int mouseMode;
        int draggedViewport;
        Int2 mouseMoveOrigin/*, viewportSize*/;
        
        String buildTexName;
        shared_ptr<ITexture> buildTex;
        int buildPhase;
        Float3 buildOrigin, buildEndpoint;

        bool viewTransition;
        float viewTransitionProgress;

        IEditorSession      *session;
        //player_sandbox      *sandboxPlayerEnt;
        //Object<control_1stperson> controlEnt;

        friend class GroupNode;

        static int log_class;       // FIXMEL : wat do with this

        protected:
            uint32_t DrawAllViews( bool picking, const Int2& mousePos );
            void DrawToolTip(Int2 mousePos, const char* text);
            void DrawView(unsigned int view, bool picking);
            void DrawWorldCursor(const Float3& world_pos);
            void Repaint() {}
            void selectCamera( unsigned viewport );

            template <typename T> T Snap(const T& value)
            {
                const float snap = 0.5f;

                return glm::round(value / snap) * snap;
            }

        public:
            EditorView();
            ~EditorView();

            virtual bool Init(ISystem* sys, IRenderingKit* rk, IRenderingManager* rm,
                    shared_ptr<IResourceManager> brushTexMgrRef) override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override;
            void DropResources();

            virtual void Draw() override;
            virtual void OnFrame(double delta) override;
            virtual int OnMouseButton(int h, int button, bool pressed, int x, int y) override;
            virtual int OnMouseMove(int h, int x, int y) override;

            virtual bool Load(InputStream* stream) override;
            virtual bool Serialize(OutputStream* stream) override;

            virtual void BeginDrawBrush(const char* material) override;
            virtual void SelectAt(int x, int y, int mode) override;
            virtual void SetViewSetup(ViewSetup_t viewSetup) override;

            void SetSession(IEditorSession *session)
            {
                if ((this->session = session) != nullptr)
                    session->SetMessageListener(this);
            }

            virtual void OnSessionMessage(int clientId, uint32_t id, ArrayIOStream& message) override;

            void AddBrush(int clientId, const char* name, const Float3& pos, const Float3& size, const char* material);
            void AddEntity(int clientId, const char* name, const char* entdef, const Float3& pos);
            void AddStaticModel(int clientId, const char* name, const Float3& pos, const char* path);

            IEntity* FindEntity(const char* entdef);
            //void TransitionToView(ViewType viewType, IEntity* controlEnt);

        private:
            bool p_ScreenToWorld(const Viewport_t& viewport, Int2 mouseInEV, const Float2& xyInWorld, Float3& worldPos_out);
            bool p_ScreenToWorld(const Viewport_t& viewport, Int2 mouseInEV, float zInWorld, bool tryDepth, Float3& worldPos_out);
            bool p_ScreenToWorldDelta(const Viewport_t& viewport, Int2 mouseInEVOrig, Int2 mouseDelta, Float3& worldDelta_out);
            int p_ViewportAtPos(Int2 mousePos);

            ISystem* sys;

            IRenderingKit* rk;
            IRenderingManager* rm;
            unique_ptr<IResourceManager> res;

            // RK resources
            BasicPainter3D<> bp;
            Picking<> picking;
            TexturedPainter3D<> tp;
            shared_ptr<IResourceManager> brushTexMgr;

            Int2 windowSize;
            Int2 mousePos;
            Viewport_t viewports[kMaxViewports];
            unsigned int numViewports;

            bool posInWorldValid;
            Float3 posInWorld;

            shared_ptr<IFontFace> ui_small;
            shared_ptr<IGeomChunk> wireframeCube;
    };

    player_sandbox::player_sandbox()
    {
        pos = Float3();
        orientation = Float3(1.0f, 0.0f, 0.0f);
        height = 1.5f;
    }

    static bool PlaneRayIntersect(const Float3& rayPos, const Float3& rayDir, const Float3& normal, float offset, Float3& point_out, float& t_out)
    {
        const float epsilon = 1e-4f;
        const float normalRayDot = glm::dot(normal, rayDir);

        // ray is parallel to the plane
        if (fabs(normalRayDot) < epsilon)
            return false;

        const float t = -(glm::dot(normal, rayPos) + offset) / normalRayDot;
    
        // ray is parallel to plane
        //if (t < 0)
        //    return false;
        //ZFW_DBGASSERT(t > -epsilon)

        point_out = rayPos + t * rayDir;

        //const Float3 pointToRayPos = rayPos - point_out;
        //const float dot = glm::dot(pointToRayPos, normal);    -- < 0 if ray comes from behind

        t_out = t;
        return true;
    }
    
    // ====================================================================== //
    //  class EditorView
    // ====================================================================== //

    IEditorView* CreateEditorView()
    {
        return new EditorView();
    }

    EditorView::EditorView()
    {
        res = nullptr;

        m = MODE_IDLE;
        mouseMode = -1;

        buildTex = nullptr;
        buildPhase = 0;

        viewTransition = false;
        session = nullptr;

        posInWorldValid = false;

        ui_small = nullptr;
        wireframeCube = nullptr;
    }

    EditorView::~EditorView()
    {
        Shutdown();
    }

    bool EditorView::Init(ISystem* sys, IRenderingKit* rk, IRenderingManager* rm, shared_ptr<IResourceManager> brushTexMgr)
    {
        this->sys = sys;
        this->rk = rk;
        this->rm = rm;
        this->brushTexMgr = brushTexMgr;

        windowSize = rm->GetViewportSize();

        res.reset(sys->CreateResourceManager("EditorView::res"));
        rm->RegisterResourceProviders(res.get());

        world.reset(new World());
        world->Init(brushTexMgr, "untitled");
        
        SetViewSetup(kViewSetup2_2);
        return true;
    }

    void EditorView::Shutdown()
    {
        DropResources();

        world.reset();
    }

    bool EditorView::AcquireResources()
    {
        if (!bp.Init(rm) || !picking.Init(rm) || !tp.Init(rm))
            return false;

        ui_small = rm->CreateFontFace("worldcraft_ui_small");

        if (!ui_small->Open("media/font/DejaVuSans.ttf", 10, IFontFace::FLAG_SHADOW))
            return false;

        return true;
    }

    void EditorView::DropResources()
    {
        // Other
        buildTex.reset();

        // AcquireResources()
        ui_small.reset();

        wireframeCube.reset();
    }

    void EditorView::AddBrush(int clientId, const char* name, const Float3& pos, const Float3& size, const char* material)
    {
        if (clientId == -1 && session != nullptr)
        {
            auto& msgbuf = session->BeginMessage(0x0002);
            msgbuf.writeString(name);
            msgbuf.write(&pos, sizeof(pos));
            msgbuf.write(&size, sizeof(size));
            msgbuf.writeString(material);
            session->Broadcast(msgbuf);
        }

        BrushNode* brush = new BrushNode(world.get(), "newbrush");
        brush->Init(pos, size, material);

        world->worldNode->Add(brush);

        if (clientId == -1)
            world->SetSelection(brush);
    }

    void EditorView::AddEntity(int clientId, const char* name, const char* entdef, const Float3& pos)
    {
        if (clientId == -1 && session != nullptr)
        {
            auto& msgbuf = session->BeginMessage(0x0003);
            msgbuf.writeString(name);
            msgbuf.writeString(entdef);
            msgbuf.write(&pos, sizeof(pos));
            session->Broadcast(msgbuf);
        }
                
        EntityNode* e = new EntityNode(world.get(), name);
        e->Init(entdef, pos);

        world->worldNode->Add(e);

        if (clientId == -1)
            world->SetSelection(e);
    }

    void EditorView::AddStaticModel(int clientId, const char* name, const Float3& pos, const char* path)
    {
        if (clientId == -1 && session != nullptr)
        {
            auto& msgbuf = session->BeginMessage(0x0001);
            msgbuf.writeString(name);
            msgbuf.write(&pos, sizeof(pos));
            msgbuf.writeString(path);
            session->Broadcast(msgbuf);
        }
                
        StaticModelNode* model = new StaticModelNode(world.get(), name);
        model->Init(pos, path);

        world->worldNode->Add(model);

        if (clientId == -1)
            world->SetSelection(model);
    }

    void EditorView::BeginDrawBrush(const char* material)
    {
        //buildTexName = (String)"media/texture/" + texture;
        buildTexName = material;

        buildTex.reset();
        buildTex = brushTexMgr->GetResourceByPath<ITexture>(buildTexName, RESOURCE_REQUIRED, 0);
        buildPhase = 0;

        m = MODE_DRAW_BRUSH;
    }

    void EditorView::Draw()
    {
        auto begin = sys->GetGlobalMicros();
                
        DrawAllViews( false, Int2() );

        auto end = sys->GetGlobalMicros();
            
        rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);

        ui_small->DrawText(sprintf_63("VIEWPORTS RENDERED IN : %i us", (int)(end - begin)), RGBA_WHITE, 0, Float3(8, 32, 0), Float2());

        if (m == MODE_DRAW_BRUSH)
        {
            auto size = glm::abs(buildEndpoint - buildOrigin);
            DrawToolTip(mousePos, sprintf_63("BuildSize (%g %g %g)", size.x, size.y, size.z));
        }

        if (posInWorldValid)
            DrawWorldCursor(Snap(posInWorld));
    }

    uint32_t EditorView::DrawAllViews( bool isPicking, const Int2& mousePos )
    {
        uint32_t pickingId = 0;
        
        if (isPicking)
            rm->SetClearColour(COLOUR_BLACK);
        else
            rm->SetClearColour(COLOUR_GREY(0.1f, 1.0f));

        rm->Clear();

        if ( isPicking )
        {
            int viewport = p_ViewportAtPos(mousePos);

            if (viewport >= 0)
            {
                picking.BeginPicking();

                DrawView(viewport, true);

                picking.EndPicking(mousePos, &pickingId);
            }
        }
        else
        {
            for (unsigned int i = 0; i < numViewports; i++)
            {
                DrawView(i, false);
            }
        }

        return pickingId;
    }

    void EditorView::DrawToolTip(Int2 mousePos, const char* text)
    {
        const Int2 pad(4, 4);

        Paragraph* p = ui_small->CreateParagraph();
        ui_small->LayoutParagraph(p, text, RGBA_WHITE, ALIGN_HCENTER | ALIGN_VCENTER);

        const Float3 pos(mousePos + pad, 0.0f);
        const Float3 size(ui_small->MeasureParagraph(p) + pad * 3, 0.0f);

        bp.DrawFilledRectangle(pos, size, RGBA_BLACK_A(192));

        ui_small->DrawParagraph(p, pos, Float2(size));
        ui_small->ReleaseParagraph(p);
    }

    void EditorView::DrawView(unsigned int view, bool isPicking)
    {
        Int2 viewportPos, viewportSize;
        const Float3 gridCell(1.0f, 1.0f, 0.0f);

        rm->GetViewportPosAndSize(&viewportPos, &viewportSize);
        rm->SetViewportPosAndSize(viewportPos + viewports[view].pos, viewports[view].size);
        
        selectCamera( view );

        auto cam = world->viewports[view].camera;
        cam->GetProjectionModelViewMatrix(&viewports[view].projection);
        viewports[view].unprojection = glm::inverse(viewports[view].projection);

        viewports[view].ready = true;

        rm->SetRenderState(RK_DEPTH_TEST, 1);

        if (!isPicking)
        {
            switch (world->viewports[view].type)
            {
                case kViewportPerspective:
                    bp.DrawGridAround(Float3(0.0f, 0.0f, 0.01f), gridCell, Int2(20, 20), RGBA_WHITE);
                    break;
            
                case kViewportTop:
                {
                    Float3 visibleMin, visibleMax;

                    if (AlgebraHelpers::TransformVec(Float4(-1.0f, 1.0f, 0.0f, 1.0f), viewports[view].unprojection, visibleMin)
                            && AlgebraHelpers::TransformVec(Float4(1.0f, -1.0f, 0.0f, 1.0f), viewports[view].unprojection, visibleMax))
                    {
                        const float z = -0.01f;

                        visibleMin = Float3(glm::floor(visibleMin.x), glm::floor(visibleMin.y), z);
                        visibleMax = Float3(glm::ceil(visibleMax.x), glm::ceil(visibleMax.y), z);

                        bp.DrawGrid(visibleMin, gridCell, Int2(visibleMax - visibleMin), RGBA_GREY(192));
                    }

                    break;
                }
            }

            if (m == MODE_DRAW_BRUSH)
            {
                if (buildPhase == 1 || buildPhase == 2)
                    tp.DrawFilledCuboid(buildTex.get(), buildOrigin, buildEndpoint - buildOrigin, RGBA_WHITE);
            }
        }

        RenderState_t rs = { bp, picking, tp };

        if (isPicking)
            world->worldNode->Draw(rs, kNodeDrawPicking);
        else if (world->viewports[view].type != kViewportPerspective)
            world->worldNode->Draw(rs, kNodeDrawWireframe);
        else
            world->worldNode->Draw(rs, 0);

        if (!isPicking)
        {
            rm->SetRenderState(RK_DEPTH_TEST, 0);

            Float3 bbPos, bbSize;

            if (world->selection != nullptr && world->selection->GetBoundingBox(bbPos, bbSize))
                bp.DrawCuboidWireframe(bbPos, bbSize, RGBA_COLOUR(0, 255, 0));
            
            // TODO: draw movement gizmo
            
            
        }

        /*
        const float scale = world->rootNode->camera[view].getDistance() / 5.0f;
        gizmoTransforms[0].vector.x = scale;
        gizmoTransforms[0].vector.y = scale;
        gizmoTransforms[0].vector.z = scale;
        
        for ( size_t i = 0; i < lengthof( gizmoVisible ); i++ )
            if ( gizmoVisible[i] )
            {
                if ( picking )
                    gizmoId[i] = res->gizmo[i]->pick( gizmoTransforms, lengthof( gizmoTransforms ) );
                else
                    res->gizmo[i]->render( gizmoTransforms, lengthof( gizmoTransforms ) );
            }*/

        rm->SetViewportPosAndSize(viewportPos, viewportSize);
    }

    void EditorView::DrawWorldCursor(const Float3& world_pos)
    {
        for (unsigned int i = 0; i < numViewports; i++)
        {
            if (!viewports[i].ready)
                continue;

            const Float2 eyeSpace = Float2(AlgebraHelpers::TransformVec(Float4(world_pos, 1.0f), viewports[i].projection));
            const Float3 screenSpace = Float3(Float2(viewports[i].pos)
                    + Float2(eyeSpace.x + 1.0f, 1.0f - eyeSpace.y) * Float2(viewports[i].size.x / 2, viewports[i].size.y / 2), 0.0f);

            const Float3 abc[] = {
                screenSpace,
                screenSpace + Float3(3, -10, 0),
                screenSpace + Float3(-3, -10, 0)
            };

            bp.DrawFilledTriangle(abc, RGBA_COLOUR(255, 0, 0, 160));
        }
    }

    IEntity* EditorView::FindEntity(const char* entity)
    {
        EntityNodeSearch es(entity);
        world->worldNode->SearchNode(&es);
        return (es.entNode != nullptr) ? es.entNode->GetEnt() : nullptr;
    }
    
    bool EditorView::Load(InputStream* stream)
    {
        cfx2::Document doc;

        doc.loadFrom(stream);
        world->worldNode->Load(doc, 0);

        return true;
    }

    void EditorView::OnFrame(double delta)
    {
        if (viewTransition)
            viewTransitionProgress = glm::min<float>(float(viewTransitionProgress + delta), 1.0f);

        //FIXME1
        /*for (size_t i = 0; i < Event::GetCount(); i++)
        {
            if (controlEnt != nullptr)
                controlEnt->OnEvent(-1, Event::GetCached(i));
        }

        if (controlEnt != nullptr)
            controlEnt->OnFrame(delta);*/
    }

    int EditorView::OnMouseButton(int h, int button, bool pressed, int x, int y)
    {
        if (h > h_indirect)
            return h;

        const int mouseViewport = p_ViewportAtPos(Int2(x, y));

        switch (button)
        {
            case MOUSEBTN_LEFT:
            {
                if (m == MODE_DRAW_BRUSH && pressed && mouseViewport >= 0)
                {
                    if (buildPhase == 0)
                    {
                        if (p_ScreenToWorld(viewports[mouseViewport], Int2(x, y), 0.0f, true, buildOrigin))
                        {
                            buildOrigin = Snap(buildOrigin);
                            buildEndpoint = buildOrigin;
                            buildPhase = 1;
                        }
                    }
                    else if (buildPhase == 1)
                    {
                        buildPhase = 2;
                    }
                    else if (buildPhase == 2)
                    {
                        buildTex.reset();

                        Float3 size(buildEndpoint - buildOrigin);

                        for (int a = 0; a < 3; a++)
                            if (size[a] < 0.0f)
                            {
                                buildOrigin[a] += size[a];
                                size[a] = -size[a];
                            }

                        AddBrush(-1, "newbrush", buildOrigin, size, buildTexName);

                        m = MODE_IDLE;
                    }

                    return 0;
                }

                /* if ( world != nullptr && ctrlDown )
                {
                    menuView = getViewportAt( Vector2<unsigned>( event.GetX(), event.GetY() ) );
            
                    res->viewMenu = res->gui->createPopupMenu();
            
                    res->topCmd = res->viewMenu->addCommand( "Top" );
                    res->frontCmd = res->viewMenu->addCommand( "Front" );
                    res->leftCmd = res->viewMenu->addCommand( "Left" );
                    res->viewMenu->addSpacer();
                    res->perspectiveTgl = res->viewMenu->addToggle( "Perspective", world->rootNode->viewports[menuView].perspective );
                    res->cullingTgl = res->viewMenu->addToggle( "Culling", world->rootNode->viewports[menuView].culling );
                    res->wireframeTgl = res->viewMenu->addToggle( "Wireframe", world->rootNode->viewports[menuView].wireframe );
                    res->shadedTgl = res->viewMenu->addToggle( "Shaded", world->rootNode->viewports[menuView].shaded );
            
                    res->viewMenu->setEventListener( this );
                    res->viewMenu->show( Vector2<>( event.GetX(), event.GetY() ) );
                }*/
                break;
            }

            case MOUSEBTN_RIGHT:
                if (world == nullptr)
                    break;

                if (m == MODE_DRAW_BRUSH && pressed)
                    m = MODE_IDLE;

                /*if ( res->gui->onMouseButton( MouseButton::right, true, Int2(x, y) ) )
                    ;
                else*/
                {
                    if (pressed && mouseViewport >= 0 && world->viewports[mouseViewport].type == kViewportPerspective)
                    {
                        draggedViewport = mouseViewport;
                        mouseMoveOrigin = Int2(x, y);
                        mouseMode = MS_ROTATE_VIEW;
                    }
                    else
                        mouseMode = -1;

                    /*selectCamera( dragView );
                    graphicsDriver->getProjectionInfo( projectionBuffer );*/
                }
        
            Repaint();
            break;

            case MOUSEBTN_MIDDLE:
                if ( world == nullptr )
                    break;

                if (pressed && mouseViewport >= 0)
                {
                    draggedViewport = mouseViewport;
                    mouseMoveOrigin = Int2(x, y);
                    mouseMode = MS_PAN_VIEW;
                }
                else
                    mouseMode = -1;

                /*
                selectCamera( dragView );
                graphicsDriver->getProjectionInfo( projectionBuffer );
                */
                break;

            case MOUSEBTN_WHEEL_UP:
            case MOUSEBTN_WHEEL_DOWN:
            {
                if ( world == nullptr || !pressed || mouseViewport < 0)
                    break;

                const float dir = (button == MOUSEBTN_WHEEL_UP) ? (-1.0f) : (1.0f);

                world->viewports[mouseViewport].camera->CameraZoom(dir * 3.0f, false);
                viewports[mouseViewport].ready = false;

                Repaint();
                break;
            }
        }
        
        return -1;
    }
    
    int EditorView::OnMouseMove(int h, int x, int y)
    {
        mousePos = Int2(x, y);
        const Int2 delta = mousePos - mouseMoveOrigin;

        const int mouseViewport = p_ViewportAtPos(Int2(x, y));
        posInWorldValid = (mouseViewport >= 0) && p_ScreenToWorld(viewports[mouseViewport], mousePos, 0.0f, true, posInWorld);

        switch (mouseMode)
        {
            case MS_PAN_VIEW:
            {
                Float3 worldDelta;

                if (p_ScreenToWorldDelta(viewports[draggedViewport], mouseMoveOrigin, delta, worldDelta))
                {
                    world->viewports[draggedViewport].camera->CameraMove(-worldDelta * 1.0f);
                    viewports[draggedViewport].ready = false;
                }
                break;
            }

            case MS_ROTATE_VIEW:
                world->viewports[draggedViewport].camera->CameraRotateXY( delta.y * (f_pi / 2) / 400.0f, false );
                world->viewports[draggedViewport].camera->CameraRotateZ( -delta.x * (f_pi / 2) / 200.0f, false );
                viewports[draggedViewport].ready = false;
                break;
        }
        
        if (m == MODE_DRAW_BRUSH)
        {
            if (buildPhase == 1)
            {
                if ((mouseViewport >= 0) && p_ScreenToWorld(viewports[mouseViewport], mousePos, buildOrigin.z, false, buildEndpoint))
                    buildEndpoint = Snap(buildEndpoint);
            }
            else if (buildPhase == 2)
            {
                if ((mouseViewport >= 0) && p_ScreenToWorld(viewports[mouseViewport], mousePos, Float2(buildEndpoint), buildEndpoint))
                    buildEndpoint = Snap(buildEndpoint);
            }
        }

        mouseMoveOrigin = mousePos;

        Repaint();

        return 0;
    }

    void EditorView::OnSessionMessage(int clientId, uint32_t id, ArrayIOStream& message) 
    {
        switch (id)
        {
            case 0x0001:
            {
                Float3 pos;

                String name = message.readString();
                message.read(&pos, sizeof(pos));
                String path = message.readString();

                AddStaticModel(clientId, name, pos, path);
                break;
            }

            case 0x0002:
            {
                Float3 pos, size;

                String name = message.readString();
                message.read(&pos, sizeof(pos));
                message.read(&size, sizeof(size));
                String material = message.readString();

                AddBrush(clientId, name, pos, size, material);
                break;
            }

            case 0x0003:
            {
                Float3 pos;

                String name = message.readString();
                String entity = message.readString();
                message.read(&pos, sizeof(pos));

                AddEntity(clientId, name, entity, pos);
                break;
            }
        }
    }

    bool EditorView::p_ScreenToWorld(const Viewport_t& viewport, Int2 mouseInEV, const Float2& xyInWorld, Float3& worldPos_out)
    {
        if (!viewport.ready)
            return false;

        const Int2 mouseInViewport = mouseInEV - viewport.pos;
        const Float2 eyeSpace(
                mouseInViewport.x / (viewport.size.x / 2.0f) - 1.0f,
                1.0f - mouseInViewport.y / (viewport.size.y / 2.0f)
        );
        
        // Calculate world position using plane-ray intersection
        // we pick 3 points in eye space and unproject them to get a plane that is horizontal in screenspace
        // line starts at `xyInWorld,0` and points up

        // n |    2
        //   |   /
        //   |  /
        //   | /
        //   |/________
        //  1          3

        const Float3 worldPos1 = AlgebraHelpers::TransformVec(Float4(eyeSpace.x, eyeSpace.y, 0.0f, 1.0f), viewport.unprojection);
        const Float3 worldPos2 = AlgebraHelpers::TransformVec(Float4(eyeSpace.x, eyeSpace.y, 1.0f, 1.0f), viewport.unprojection);
        const Float3 worldPos3 = AlgebraHelpers::TransformVec(Float4(eyeSpace.x - 1.0f, eyeSpace.y, 0.0f, 1.0f), viewport.unprojection);

        const Float3 normal = glm::cross(worldPos2 - worldPos1, worldPos3 - worldPos1);
        const float d = -glm::dot(normal, worldPos1);

        float t;
        return PlaneRayIntersect(Float3(xyInWorld, 0.0f), Float3(0.0f, 0.0f, 1.0f), normal, d, worldPos_out, t);
    }

    bool EditorView::p_ScreenToWorld(const Viewport_t& viewport, Int2 mouseInEV, float zInWorld, bool tryDepth, Float3& worldPos_out)
    {
        if (!viewport.ready)
            return false;

        const Int2 mouseInViewport = mouseInEV - viewport.pos;
        const Float2 eyeSpace(
                mouseInViewport.x / (viewport.size.x / 2.0f) - 1.0f,
                1.0f - mouseInViewport.y / (viewport.size.y / 2.0f)
        );

        if (tryDepth)
        {
            float depth;
        
            if (rm->ReadPixel(mouseInEV, nullptr, &depth) && depth < 1.0f - 1e-5f)
            {
                if (AlgebraHelpers::TransformVec(Float4(eyeSpace, depth, 1.0f), viewport.unprojection, worldPos_out))
                    return true;
            }
        }

        // Calculate world position using plane-ray intersection
        // plane is (0, 0, 1) with offset `zInWorld`
        // line is determined by picking 2 points in eye space and unprojecting them into world
        //  (you can't unproject direction directly in perspective)

        const Float3 worldPos1 = AlgebraHelpers::TransformVec(Float4(eyeSpace, 0.0f, 1.0f), viewport.unprojection);
        const Float3 worldPos2 = AlgebraHelpers::TransformVec(Float4(eyeSpace, 1.0f, 1.0f), viewport.unprojection);
        const Float3 worldDirNorm = glm::normalize(worldPos2 - worldPos1);

        float t;
        return PlaneRayIntersect(worldPos1, worldDirNorm, Float3(0.0f, 0.0f, 1.0f), -zInWorld, worldPos_out, t);
    }

    bool EditorView::p_ScreenToWorldDelta(const Viewport_t& viewport, Int2 mouseInEVOrig, Int2 mouseDelta, Float3& worldDelta_out)
    {
        if (!viewport.ready)
            return false;

        const Float2 eyeSpaceDelta(
                mouseDelta.x / (viewport.size.x / 2.0f),
                -mouseDelta.y / (viewport.size.y / 2.0f)
        );

        float eyeSpaceDepth = 0.0f;

        if (world->viewports[viewport.index].type == kViewportPerspective)
        {
            // Some magic required to determine correct screenspace depth
            Float3 v;

            if (!p_ScreenToWorld(viewport, mouseInEVOrig, 0.0f, true, v))
                return false;

            const glm::mat4x4& m = viewport.projection;
            eyeSpaceDepth = (m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2]) / (m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3]);
        }

        const Float3 worldPos1 = AlgebraHelpers::TransformVec(Float4(0.0f, 0.0f, eyeSpaceDepth, 1.0f), viewport.unprojection);
        const Float3 worldPos2 = AlgebraHelpers::TransformVec(Float4(eyeSpaceDelta, eyeSpaceDepth, 1.0f), viewport.unprojection);
        worldDelta_out = worldPos2 - worldPos1;

        return true;
    }

    int EditorView::p_ViewportAtPos(Int2 mousePos)
    {
        for (unsigned int i = 0; i < numViewports; i++)
        {
            if (mousePos.x >= viewports[i].pos.x && mousePos.y >= viewports[i].pos.y
                    && mousePos.x < viewports[i].pos.x + viewports[i].size.x
                    && mousePos.y < viewports[i].pos.y + viewports[i].size.y)
                return i;
        }

        return -1;
    }

    void EditorView::SelectAt(int x, int y, int mode)
    {
        ZFW_ASSERT(mode == 0)

        world->ClearSelection();
        
        uint32_t id = DrawAllViews(true, Int2(x, y));

        world->worldNode->OnAfterPicking(id);
    }

    void EditorView::selectCamera( unsigned viewport )
    {
        //graphicsDriver->clearLights();

        /*if ( world->viewports[viewport].perspective )
        {
            world->viewports[viewport].camera->Activate();
            //render::R::SetPerspectiveProjection(0.1f, 2000.0f, world->viewports[viewport].fov);
        }*/
        /*else
        {
            float vFovRange = world->camera[viewport].getDistance() * tan( world->rootNode->viewports[viewport].fov * M_PI / 180.0f );

            const Vector2<> vFov( -vFovRange / 2, vFovRange / 2 );
            const Vector2<> hFov( vFov * ( ( float ) canvas->GetSize().x / canvas->GetSize().y ) );

            graphicsDriver->setOrthoProjection( hFov, vFov, Vector2<>( 0.0f, 1000.0f ) );
        }*/

        //FIXME
#if 0
        if (viewTransition)
        {
            Camera entCam, viewCam, interpolated;
            sandboxPlayerEnt->GetViewCamera(entCam);
            viewCam = world->viewports[viewport].camera;

            interpolated.eye = entCam.eye * viewTransitionProgress
                + viewCam.eye * (1.0f - viewTransitionProgress);

            /*interpolated.center = interpolated.eye
                + glm::normalize(entCam.center - entCam.eye) * viewTransitionProgress
                + glm::normalize(viewCam.center - viewCam.eye) * (1.0f - viewTransitionProgress);*/

            interpolated.center = entCam.center * viewTransitionProgress
                + viewCam.center * (1.0f - viewTransitionProgress);

            interpolated.up = entCam.up * viewTransitionProgress
                + viewCam.up * (1.0f - viewTransitionProgress);

            render::R::SetCamera(interpolated);
        }
        else
#endif
            rm->SetCamera(world->viewports[viewport].camera.get());

        //graphicsDriver->setCamera( &world->rootNode->camera[viewport] );
        //graphicsDriver->setRenderFlag( RenderFlag::culling, world->rootNode->viewports[viewport].culling );
        //graphicsDriver->setRenderFlag( RenderFlag::wireframe, world->rootNode->viewports[viewport].wireframe );

        /*if ( !world->rootNode->viewports[viewport].shaded )
            graphicsDriver->setSceneAmbient( Colour::white() );
        else
        {
            graphicsDriver->setSceneAmbient( Colour::grey( 0.4f ) );
            res->worldLight->render( nullptr, 0, false );
        }*/
    }

    bool EditorView::Serialize(OutputStream* stream)
    {
        if (world == nullptr)
            return false;

        cfx2::Document doc;

        doc.create();
        world->worldNode->Serialize(doc);
        doc.save(stream);

        return true;
    }

    void EditorView::SetViewSetup(ViewSetup_t viewSetup)
    {
        Int2 viewSize;

        switch (viewSetup)
        {
            case kViewSetup1:
                numViewports = 1;
                viewports[0].pos = Int2();
                viewports[0].size = windowSize;
                break;

            case kViewSetup2_2:
                numViewports = 4;
                viewSize = windowSize / 2;
                viewports[0].pos = Int2(0, 0);
                viewports[0].size = viewSize;
                viewports[1].pos = Int2(viewSize.x, 0);
                viewports[1].size = viewSize;
                viewports[2].pos = Int2(0, viewSize.y);
                viewports[2].size = viewSize;
                viewports[3].pos = Int2(viewSize.x, viewSize.y);
                viewports[3].size = viewSize;
                break;
        }

        for (unsigned int i = 0; i < numViewports; i++)
        {
            viewports[i].index = i;
            viewports[i].ready = false;
        }
    }

#if 0
    void EditorView::TransitionToView(ViewType viewType, IEntity* controlEnt)
    {
        this->viewTransitionFrom = this->viewType;
        this->viewType = viewType;

        ZFW_ASSERT(false)

        /*if (viewType == VIEW_SANDBOX)
            sandboxPlayerEnt = static_cast<player_sandbox*>(controlEnt);

        viewTransitionProgress = 0.0f;
        viewTransition = true;

        this->controlEnt = new control_1stperson;
        this->controlEnt->LinkEntity(sandboxPlayerEnt);
        auto& controls = this->controlEnt->GetControls();
        controls.keys[control_1stperson::K_FWD].inclass = IN_KEY;
        controls.keys[control_1stperson::K_FWD].key = 'w';
        controls.keys[control_1stperson::K_BACK].inclass = IN_KEY;
        controls.keys[control_1stperson::K_BACK].key = 's';
        controls.keys[control_1stperson::K_LEFT].inclass = IN_KEY;
        controls.keys[control_1stperson::K_LEFT].key = 'a';
        controls.keys[control_1stperson::K_RIGHT].inclass = IN_KEY;
        controls.keys[control_1stperson::K_RIGHT].key = 'd';*/
    }
#endif
}
