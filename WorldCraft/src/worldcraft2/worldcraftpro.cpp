
#include "worldcraftpro.hpp"

#include <framework/colorconstants.hpp>
#include <framework/entityhandler.hpp>
#include <framework/errorcheck.hpp>
#include <framework/filesystem.hpp>
#include <framework/graphics.hpp>
#include <framework/nativedialogs.hpp>
#include <framework/messagequeue.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/system.hpp>

#include <littl/Directory.hpp>
#include <littl/File.hpp>
#include <littl/FileName.hpp>

// TODO: LOD Control in CreateResource(ITexture)

#undef DrawText

namespace worldcraftpro
{
    static const char* kRecentFilesPath = APP_NAME ".recent";

    static BasicPainter2D<> bp;

    static IResourceManager* s_brushTexMgr;

    static ITexturePreviewCache* widgetLoaderTexPreviewCache = nullptr;

    class MaterialButton : public gameui::Widget
    {
        protected:
            static const int spacing = 16;

            IRKUIThemer* rkuiThemer;
            ITexturePreviewCache* texPreviewCache;

            shared_ptr<IGraphics> gRef;

            String label, texture, info;
            int previewSize;
            size_t font, infoFont;
            bool displayInfo;

            bool mouseOver;
            float mouseOverAmt;

        private:
            MaterialButton(const MaterialButton &);

        public:
            MaterialButton(gameui::UIThemer* themer, ITexturePreviewCache* texPreviewCache, String label, String texture, int previewSize)
                    : Widget(), texPreviewCache(texPreviewCache), label(label), texture(texture), previewSize(previewSize)
            {
                gRef = nullptr;

                font = themer->GetFontId("wc2_ui_small_bold");
                infoFont = themer->GetFontId("wc2_ui_small");
                displayInfo = true;

                mouseOver = false;
                mouseOverAmt = 0.0f;

                ZFW_ASSERT((rkuiThemer = dynamic_cast<IRKUIThemer*>(themer)) != nullptr)
            }

            virtual ~MaterialButton()
            {
                DropResources();
            }

            virtual bool AcquireResources() override
            {
                info.clear();

                if (!texture.isEmpty())
                {
                    StudioKit::TexturePreview_t cached;

                    if (texPreviewCache->RetrieveEntry(texture, cached))
                    {
                        if (displayInfo)
                            info = sprintf_255("%ix%i, %s, %g KiB", cached.originalSize.x, cached.originalSize.y,
                                    cached.formatName, ceil(cached.fileSize / 1024.0f));
                        else
                            info.clear();

                        gRef = cached.g;
                    }
                    else
                    {
                        auto tex = s_brushTexMgr->GetResourceByPath<ITexture>(texture, 0, 0);

                        if (tex)
                            gRef = g.rm->CreateGraphicsFromTexture(tex);
                        else
                            gRef = nullptr;
                    }
                }

                CalculateMinSize();
                return true;
            }

            virtual void DropResources() override
            {
            }

            virtual void CalculateMinSize()
            {
                Int2 labelSize = rkuiThemer->GetFont(font)->MeasureText(label);
                Int2 infoSize = rkuiThemer->GetFont(infoFont)->MeasureText(info);

                SetMinSize(Int2(previewSize + spacing + glm::max(labelSize.x, infoSize.x) + spacing, glm::max(previewSize, labelSize.y + infoSize.y)));
            }

            virtual void Draw() override
            {
                if (mouseOverAmt > 0.01f)
                {
                    bp.DrawFilledRectangle(Short2(pos.x + previewSize, pos.y),
                            Short2(size.x - previewSize, previewSize),
                            RGBA_WHITE_A(mouseOverAmt * 0.2f));
                }

                if (gRef != nullptr)
                    gRef->DrawStretched(nullptr, Float3(pos.x, pos.y, 0.0f), Float3(previewSize, previewSize, 0.0f));

                rkuiThemer->GetFont(font)->DrawText(label, RGBA_WHITE, ALIGN_LEFT | ALIGN_BOTTOM, Float3(pos.x + previewSize + spacing, pos.y + size.y / 2, 0.0f), Float2());
                rkuiThemer->GetFont(infoFont)->DrawText(info, RGBA_WHITE, ALIGN_LEFT | ALIGN_TOP, Float3(pos.x + previewSize + spacing, pos.y + size.y / 2, 0.0f), Float2());
            }

            const String& GetLabel() { return label; }

            virtual int OnMouseMove(int h, int x, int y) override
            {
                mouseOver = (h < 0 && IsInsideMe(x, y));
                return h;
            }

            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override
            {
                if (h <= gameui::h_indirect && IsInsideMe(x, y))
                {
                    if (pressed)
                        FireMouseDownEvent(MOUSEBTN_LEFT, x, y);
                    else
                        FireClickEvent(x, y);

                    return 1;
                }

                return h;
            }

            virtual void OnFrame(double delta) override
            {
                if (mouseOver)
                    mouseOverAmt = (float)glm::min(mouseOverAmt + delta / 0.1f, 1.0);
                else
                    mouseOverAmt = (float)glm::max(mouseOverAmt - delta / 0.1f, 0.0);
            }

            void SetDisplayInfo(bool show) { displayInfo = show; }
            void SetFont(size_t font) { this->font = font; }
            void SetInfoFont(size_t infoFont) { this->infoFont = infoFont; }
            void SetLabel(const char* label) { this->label = label; }
            void SetTexture(const char* texture) { this->texture = texture; }
    };

    class MaterialButtonBig : public MaterialButton
    {
        protected:
            static const int spacing = 16;

            String subText;
            size_t subFont;

        private:
            MaterialButtonBig(const MaterialButtonBig &);

        public:
            MaterialButtonBig(gameui::UIThemer* themer, ITexturePreviewCache* texPreviewCache, String label, String texture)
                : MaterialButton(themer, texPreviewCache, label, texture, 64)
            {
                subText = "CLICK TO SELECT ANOTHER >";
                subFont = themer->GetFontId("wc2_ui_small");
            }

            virtual void CalculateMinSize() override
            {
                Int2 labelSize = rkuiThemer->GetFont(font)->MeasureText(label);
                Int2 infoSize = rkuiThemer->GetFont(infoFont)->MeasureText(info);
                Int2 subSize = rkuiThemer->GetFont(subFont)->MeasureText(subText);

                SetMinSize(Int2(previewSize + spacing + glm::max(glm::max(labelSize.x, infoSize.x), subSize.x) + spacing, glm::max(previewSize, labelSize.y + infoSize.y + subSize.y)));
            }

            virtual void Draw() override
            {
                float offset;

                if (mouseOverAmt > 0.01f)
                {
                    bp.DrawFilledRectangle(Short2(pos.x + previewSize, pos.y),
                            Short2(size.x - previewSize, previewSize),
                            RGBA_WHITE_A(mouseOverAmt * 0.2f));

                    offset = -mouseOverAmt * rkuiThemer->GetFont(subFont)->GetLineHeight() / 2;
                    rkuiThemer->GetFont(subFont)->DrawText(subText, RGBA_WHITE_A(mouseOverAmt), ALIGN_LEFT | ALIGN_TOP,
                            Float3(pos.x + previewSize + spacing, pos.y + size.y / 2 + offset + rkuiThemer->GetFont(infoFont)->GetLineHeight(), 0.0f),
                            Float2());
                }
                else
                    offset = 0.0f;

                if (gRef != nullptr)
                    gRef->DrawStretched(nullptr, Float3(pos.x, pos.y, 0.0f), Float3(previewSize, previewSize, 0.0f));

                rkuiThemer->GetFont(font)->DrawText(label, RGBA_WHITE, ALIGN_LEFT | ALIGN_BOTTOM, Float3(pos.x + previewSize + spacing, pos.y + size.y / 2 + offset, 0.0f), Float2());
                rkuiThemer->GetFont(infoFont)->DrawText(info, RGBA_WHITE, ALIGN_LEFT | ALIGN_TOP, Float3(pos.x + previewSize + spacing, pos.y + size.y / 2 + offset, 0.0f), Float2());
            }
    };

    static void WidgetLoaderCallback(gameui::UILoader* ldr, const char *fileName, cfx2::Node node, const gameui::WidgetLoadProperties& properties,
            gameui::Widget*& widget, gameui::WidgetContainer*& container)
    {
        String type = node.getName();

        if (type == "worldcraftpro/MaterialButtonBig")
        {
            widget = new MaterialButtonBig(ldr->GetThemer(), widgetLoaderTexPreviewCache, node.getAttrib("label"), node.getAttrib("texture"));
        }
        else if (type == "worldcraftpro/MaterialButton")
        {
            auto mb = new MaterialButton(ldr->GetThemer(), widgetLoaderTexPreviewCache, node.getAttrib("label"), node.getAttrib("texture"), node.getAttribInt("previewSize"));

            int font = ldr->GetFontId(node.getAttrib("font"));

            if (font >= 0)
                mb->SetFont(font);

            int infoFont = ldr->GetFontId(node.getAttrib("infoFont"));

            if (infoFont >= 0)
                mb->SetInfoFont(infoFont);

            widget = mb;
        }
    }

    WorldcraftScene::WorldcraftScene(ITexturePreviewCache* texPreviewCache)
            : texPreviewCache(texPreviewCache)
    {
        static const BuiltInBrush_t brush_nodraw = {"brush_nodraw", "NODRAW"};
        static const BuiltInBrush_t brush_portal = {"brush_portal", "PORTAL"};

        builtInBrushes.add(brush_nodraw);
        builtInBrushes.add(brush_portal);

        ui = nullptr;
        uiThemer = nullptr;
        editorView = nullptr;

        contentMgr = nullptr;
        materialPanel = nullptr;
        menu = nullptr;
        exportMenu = nullptr;
        materialMenu = nullptr;
    }

    bool WorldcraftScene::Init()
    {
        //FIXME1
        //auto ieh = Sys::GetEntityHandler();
        //Entity::Register("control_1stperson", &control_1stperson::Create);
        //ieh->Register("player_sandbox", &player_sandbox::Create);
        
        recentFiles.reset(studio::RecentFiles::Create(g_sys));
        recentFiles->Load(kRecentFilesPath);

        brushTexMgr.reset(g_sys->CreateResourceManager("brushTexMgr"));
        s_brushTexMgr = brushTexMgr.get();

        static const std::type_index resourceClasses[] = { typeid(ITexture) };
        brushTexMgr->RegisterResourceProvider(resourceClasses, lengthof(resourceClasses), this, 0);

        widgetLoaderTexPreviewCache = texPreviewCache;
        
        if (!bp.Init(g.rm))
            return false;

        auto imh = g_sys->GetModuleHandler(true);

        auto themer = TryCreateRKUIThemer(imh);
        ErrorPassthru(themer);
        uiThemer.reset(themer);
        themer->Init(g_sys, g.rk.get(), g.res.get());

        themer->PreregisterFont("wc2_ui",               "path=media/font/DejaVuSans.ttf,size=11");
        themer->PreregisterFont("wc2_ui_bold",          "path=media/font/DejaVuSans-Bold.ttf,size=11");
        themer->PreregisterFont("wc2_ui_small",         "path=media/font/DejaVuSans.ttf,size=10");
        themer->PreregisterFont("wc2_ui_small_bold",    "path=media/font/DejaVuSans-Bold.ttf,size=10");

        ErrorPassthru(uiThemer->AcquireResources());

        CreateUI();

        // Init the editor component
        editorView.reset(CreateEditorView());

        ErrorPassthru(editorView->Init(g_sys, g.rk.get(), g.rm, brushTexMgr));
        ErrorPassthru(AcquireResources());

        PositionToolbarSelectionShadow();

        SetMaterial("brush_solid");

        return true;
    }

    void WorldcraftScene::Shutdown()
    {
        if (recentFiles->IsDirty())
            recentFiles->Save(kRecentFilesPath);
        
        recentFiles.reset();

        bp.Shutdown();
        DropResources();

        editorView.reset();
        ui.reset();
        uiThemer.reset();
    }

    bool WorldcraftScene::AcquireResources()
    {
        //g.rm->SetClearColour(COLOUR_GREY(0.1f, 1.0f));

        if (!ui->AcquireResources())
            return false;

        if (!editorView->AcquireResources())
            return false;

        errorTex = g.res->GetResourceByPath<ITexture>("media/notexture.png", RESOURCE_REQUIRED, 0);

        if (errorTex == nullptr)
            return false;

        ui->SetArea(Int3(), g.rm->GetViewportSize());

        return true;
    }

    void WorldcraftScene::DropResources()
    {
    }

    shared_ptr<IResource> WorldcraftScene::CreateResource(IResourceManager* res, const std::type_index& resourceClass,
            const char* normparams_in, int flags)
    {
        String normparams = normparams_in;

        if (resourceClass == typeid(ITexture))
        {
            const char* path;

            ZFW_ASSERT(Params::GetValueForKey(normparams, "path", path))

            shared_ptr<ITexture> tex;

            if (path[0] == '$')
            {
                tex = p_CreateBrushTexture(res, path + 1);
            }
            else
                tex = g.res->GetResource<ITexture>(normparams, 0, flags);

            if (tex == nullptr)
                tex = errorTex;

            return tex;
        }
        else
        {
            ZFW_ASSERT(resourceClass != resourceClass)
            return nullptr;
        }
    }

    void WorldcraftScene::CreateUI()
    {
        using namespace gameui;

        Widget::SetMessageQueue(g.msgQueue.get());

        auto ui = new UIContainer(g_sys);
        this->ui.reset(ui);
        
        UILoader ldr(g_sys, ui, uiThemer.get());
        ldr.SetCustomLoaderCallback(WidgetLoaderCallback);
        ErrorLog(g_sys, g.eb, ldr.Load("StudioKit/worldcraft/contentmgr.cfx2", ui, false));
        ErrorLog(g_sys, g.eb, ldr.Load("StudioKit/worldcraft/exportmenu.cfx2", ui, false));
        ErrorLog(g_sys, g.eb, ldr.Load("StudioKit/worldcraft/editorpanels.cfx2", ui, false));
        ErrorLog(g_sys, g.eb, ldr.Load("StudioKit/worldcraft/materialmenu.cfx2", ui, false));
        ErrorLog(g_sys, g.eb, ldr.Load("StudioKit/worldcraft/menu.cfx2", ui, false));

        contentMgr =        ui->FindWidget<gameui::Window>("contentMgr");
        exportMenu =        ui->FindWidget<gameui::Window>("exportMenu");
        materialMenu =      ui->FindWidget<gameui::Window>("materialMenu");
        materialPanel =     ui->FindWidget<gameui::Window>("materialPanel");

        //ZFW_ASSERT(contentMgr != nullptr)
        //ZFW_ASSERT(exportMenu != nullptr)
        //ZFW_ASSERT(materialMenu != nullptr)
        ZFW_ASSERT(materialPanel != nullptr)

        gameui::ListCtrl* texlist = ui->FindWidget<gameui::ListCtrl>("contentMgr.texlist");

        if (texlist != nullptr)
        {
            auto fs = g_sys->GetFileSystem();
            String d = "media/texture";

            unique_ptr<IDirectory> dir(fs->OpenDirectory(d, 0));

            const char* next;

            while (dir != nullptr && (next = dir->ReadDir()) != nullptr)
            {
                String fileName = d + "/" + next;
                FSStat_t stat;

                if (!fs->Stat(fileName, &stat))
                    continue;

                if (stat.isDirectory)
                    continue;

                auto button = new MaterialButton(uiThemer.get(), texPreviewCache, next, fileName, 32);
                button->SetFont(uiThemer->GetFontId("wc2_ui_small_bold"));
                button->SetInfoFont(uiThemer->GetFontId("wc2_ui_small"));
                button->SetName("contentMgr.texlist.entry");
                texlist->Add(button);
            }
        }

        auto menu = ui->FindWidget<gameui::MenuBar>("menu");

        if (menu != nullptr)
        {
            menu->SetDragsAppWindow(true);

            auto openRecent = menu->GetItemByName("openRecent");

            if (openRecent != nullptr)
            {
                if (recentFiles->GetNumItems() > 0)
                {
                    for (size_t i = 0; i < recentFiles->GetNumItems(); i++)
                        menu->Add(openRecent, recentFiles->GetItemAt(i), "openRecentFile", gameui::MENU_ITEM_SELECTABLE);
                }
                else
                    menu->SetItemFlags(openRecent, openRecent->flags & ~gameui::MENU_ITEM_SELECTABLE);
            }
        }

        auto materialMenu_builtIn = ui->FindWidget<gameui::Table>("materialMenu.builtIn");

        if (materialMenu_builtIn != nullptr)
        {
            iterate2 (i, builtInBrushes)
            {
                auto& brush = *i;

                auto materialButton = new MaterialButton(uiThemer.get(), texPreviewCache, brush.name, (String) "$" + brush.label, 32);
                materialButton->SetDisplayInfo(false);
                materialButton->SetFont(uiThemer->GetFontId("wc2_ui_small_bold"));
                materialButton->SetName("contentMgr.texlist.entry");
                materialMenu_builtIn->Add(materialButton);
            }
        }

        toolbarSelection = 0;
    }

    void WorldcraftScene::DrawScene()
    {
        //g.rm->Clear();

        editorView->Draw();

        g.rm->SetRenderState(RK_DEPTH_TEST, 0);
        g.rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);
        ui->Draw();
    }

    String WorldcraftScene::GetTextureForMaterial(const String& materialName)
    {
        // TODO: a better way to do this?

        size_t i;

        for (i = 0; i < builtInBrushes.getLength(); i++)
            if (materialName == builtInBrushes[i].name)
                break;

        if (i < builtInBrushes.getLength())
            return (String) "$" + builtInBrushes[i].label;
        else
            return "media/texture/" + materialName;
    }

    void WorldcraftScene::OnFrame( double delta )
    {
        ui->OnFrame(delta);
        editorView->OnFrame(delta);

        /*
        case EV_UIITEMSELECTED:
            if (event->ui_item.widget->GetName() == "modeSelect")
            {
                if (event->ui_item.itemIndex == 1)
                {
                    IEntity* spawn = editorView->FindEntity("hint/spawn_sandbox");

                    if (spawn == nullptr)
                        uiContainer->PushMessageBox("Sandbox mode requires at least one 'hint/spawn_sandbox' entity");
                    else
                    {
                        auto ent = dynamic_cast<player_sandbox *>(Entity::Create("player_sandbox", true));

                        editorView->TransitionToView(VIEW_SANDBOX, ent);
                    }
                }
            }
            break;
        */

        static Int2 mousePos;       // FIXME0

        for (MessageHeader* msg = nullptr; (msg = g.msgQueue->Retrieve(Timeout(0))) != nullptr; msg->Release())
        {
            int h = gameui::h_new;
            int rc = 0;

            switch (msg->type)
            {
                case EVENT_MOUSE_MOVE:
                {
                    auto payload = msg->Data<EventMouseMove>();

                    mousePos = Int2(payload->x, payload->y);

                    h = ui->OnMouseMove(h, payload->x, payload->y);

                    //if (h < 0)
                    //    h = cam.HandleMessage(msg);

                    if (h <= h_indirect)
                        h = editorView->OnMouseMove(h, payload->x, payload->y);

                    break;
                }

                case EVENT_UNICODE_INPUT:
                {
                    auto payload = msg->Data<EventUnicodeInput>();

                    h = ui->OnCharacter(h, payload->c);

                    break;
                }

                case EVENT_VKEY:
                {
                    auto payload = msg->Data<EventVkey>();
                    int button;
                    bool pressed;

                    if (Vkey::IsMouseButtonEvent(payload->input, button, pressed))
                    {
                        int x = mousePos.x;
                        int y = mousePos.y;

                        h = ui->OnMouseButton(h, button, pressed, x, y);
                        h = editorView->OnMouseButton(h, button, pressed, x, y);

                        if (h >= gameui::h_direct)
                            break;

                        if (button == MOUSEBTN_LEFT && pressed)
                        {
                            switch (toolbarSelection)
                            {
                                case TOOLBAR_SELECT:
                                    editorView->SelectAt(x, y, 0);
                                    break;

                                case TOOLBAR_BUILD_ADD:
                                    editorView->BeginDrawBrush(GetTextureForMaterial(currentMaterialName));
                                    editorView->OnMouseButton(-1, button, pressed, x, y);
                                    break;
                            }
                        }
                    }

                    //if (h < 0)
                    //    h = cam.HandleMessage(msg);
                    break;
                }

                case EVENT_WINDOW_CLOSE:
                    g_sys->StopMainLoop();
                    break;

                case EVENT_WINDOW_RESIZE:
                {
                    auto payload = msg->Data<EventWindowResize>();

                    ui->SetArea(Int3(), Int2(payload->width, payload->height));
                    PositionToolbarSelectionShadow();
                    break;
                }

                case gameui::EVENT_CONTROL_USED:
                {
                    auto payload = msg->Data<gameui::EventControlUsed>();
                    String name = payload->widget->GetName();

                    if (name == "connection.host")
                    {
                        /*FIXMES
                        try
                        {
                            session.release();
                            session = IEditorSession::CreateHostSession(Var::GetInt("mus_port", true));

                            auto status = ui->FindWidget<gameui::StaticText>("connection.status");
                            li_tryCall(status, SetLabel("Accepting connections."));
                        } catch (const String& err) {
                            ui->PushMessageBox(err);
                        }*/

                        //FIXMES
                        //editorView->SetSession(session);
                    }
                    else if (name == "connection.join")
                    {
                        /*try
                        {FIXMES
                            session.release();
                            session = IEditorSession::CreateClientSession(Var::GetStr("mus_hostname", true), Var::GetInt("mus_port", true));

                            auto status = ui->FindWidget<gameui::StaticText>(uiContainer->Find("connection.status");
                            li_tryCall(status, SetLabel((String) "Connected to " + session->GetHostname()));
                        } catch (const String& err) {
                            ui->PushMessageBox(err);
                        }*/

                        //FIXMES
                        //editorView->SetSession(session);
                    }
                    else if (name == "connection.end")
                    {
                        //FIXMES
                        //session.release();
                        //editorView->SetSession(session);
                    }
                    else if (name == "displayEntBrowser")
                    {
                        auto entlist = ui->FindWidget<gameui::Window>("entlist");

                        if (entlist) entlist->SetVisible(true);
                    }
                    else if (name == "addent")
                    {
                        auto entlist_combo = ui->FindWidget<gameui::ComboBox>("entlist_combo");

                        if (entlist_combo != nullptr)
                        {
                            /*auto item = dynamic_cast<gameui::StaticText*>(entlist_combo->GetSelectedItem());

                            if (item != nullptr)
                                editorView->AddEntity(-1, "unnamed_ent", item->GetLabel(), Float3());*/
                            //FIXME1
                        }

                        auto entlist = ui->FindWidget<gameui::Window>("entlist");
                        if (entlist) entlist->SetVisible(false);
                    }
                    break;

                    break;
                }

                case gameui::EVENT_LOOP_EVENT:
                    ui->OnLoopEvent(msg->Data<gameui::EventLoopEvent>());
                    break;

                case gameui::EVENT_MENU_ITEM_SELECTED:
                {
                    auto payload = msg->Data<gameui::EventMenuItemSelected>();
                    
                    if (payload->item->name == "dumpRes")
                    {
                        g.res->DumpStatistics();
                    }
                    else if (payload->item->name == "export")
                    {
                        p_ExportWorld();
                    }
                    else if (payload->item->name == "open" || payload->item->name == "openRecentFile")
                    {
                        const char* newPath;

                        if (payload->item->name == "openRecentFile")
                            newPath = payload->menuBar->GetItemLabel(payload->item);
                        else
                            newPath = NativeDialogs::FileOpen(".", nullptr);

                        if (newPath != nullptr)
                        {
                            path = newPath;

                            // FIXME: Update recent files menu
                            if (p_LoadDocument())
                                recentFiles->BumpItem(path);
                        }
                    }
                    else if (payload->item->name == "new")
                    {
                        p_UnloadDocument();

                        path.clear();

                        p_NewDocument();
                    }
                    else if (payload->item->name == "save")
                    {
                        p_SaveDocument();
                    }
                    else if (payload->item->name == "saveAs")
                    {
                        const char* newPath = NativeDialogs::FileSaveAs(".", nullptr);

                        if (newPath != nullptr)
                        {
                            path = newPath;
                            
                            // FIXME: Update recent files menu
                            if (p_SaveDocument(), true)
                                recentFiles->BumpItem(path);
                        }
                    }
                    else if (payload->item->name == "quit")
                        g_sys->StopMainLoop();
                    else if (payload->item->name == "v1")
                        editorView->SetViewSetup(kViewSetup1);
                    else if (payload->item->name == "v2,2")
                        editorView->SetViewSetup(kViewSetup2_2);

                    break;
                }

                case gameui::EVENT_MOUSE_DOWN:
                {
                    auto payload = msg->Data<gameui::EventMouseDown>();
                    String name = payload->widget->GetName();

                    if (name == "cmd_open_contentmgr")
                    {
                        contentMgr->SetVisible(true);
                        contentMgr->Focus();
                    }
                    else if (name == "materialPanel.bigButton")
                    {
                        if (materialPanel != nullptr && materialMenu != nullptr)
                            materialMenu->ShowAt(Int3(materialPanel->GetPos().x + materialPanel->GetSize().x,
                                    materialPanel->GetPos().y + materialPanel->GetSize().y - materialMenu->GetSize().y,
                                    0));

                        break;
                    }
                    else if (name == "contentMgr.texlist.entry")
                    {
                        auto item = dynamic_cast<MaterialButton*>(payload->widget);

                        if (item != nullptr)
                            SetMaterial(item->GetLabel());
                    }
                    else if (name == "toolbar.icons")
                    {
                        toolbarSelection = (payload->y - payload->widget->GetPos().y) / payload->widget->GetSize().x;
                        PositionToolbarSelectionShadow();

                        if (toolbarSelection == TOOLBAR_STATICMDL_ADD)
                        {
                            //FIXME1
                            //editorView->AddStaticModel(-1, "newmodel", Float3(), "zmf_model.txt");
                        }
                    }

                    if (materialMenu) materialMenu->SetVisible(false);
                }

                case gameui::EVENT_TREE_ITEM_SELECTED:
                {
                    auto payload = msg->Data<gameui::EventTreeItemSelected>();

                    break;
                }

                case gameui::EVENT_VALUE_CHANGED:
                {
                    auto payload = msg->Data<gameui::EventValueChanged>();

                    break;
                }

                default:
                    //cam.HandleMessage(msg);
                    ;
            }
        }

        if (session) session->OnFrame(delta);
    }

    shared_ptr<ITexture> WorldcraftScene::p_CreateBrushTexture(IResourceManager* res, const char* label)
    {
        IRenderingManager* rm = g.rm;

        // generate a 64x64 texture
        const static int size = 64;

        // create and enter a throwaway renderbuffer
        auto rb = rm->CreateRenderBuffer("brush", Int2(size, size), kRenderBufferColourTexture);
        rm->PushRenderBuffer(rb.get());
        rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);

        // fill with light grey & draw label over it
        bp.DrawFilledRectangle(Short2(0, 0), Short2(size, size), RGBA_GREY(192));
        uiThemer->GetFont(1)->DrawText(label, RGBA_BLACK, ALIGN_HCENTER | ALIGN_VCENTER, Float3(), Float2(size, size));

        // leave renderbuffer
        rm->PopRenderBuffer();

        // retrieve texture from renderbuffer and return to ResourceManager
        return rb->GetTexture();
    }

    bool WorldcraftScene::p_ExportWorld()
    {
        char buffer[2048];
        if (!Params::BuildIntoBuffer(buffer, sizeof(buffer), 3,
                "path",         (const char*) path,
                "output",       (const char*) (path + ".zmf"),
                "waitforkey",   "0"
        ))
        {
            ErrorBuffer::SetBufferOverflowError(g.eb, li_functionName);
            g_sys->DisplayError(g.eb, false);
        }
        else
            system((String) "worldcraft2 -compile " + buffer);

        return true;
    }

    bool WorldcraftScene::p_LoadDocument()
    {
        p_UnloadDocument();

        if (path.isEmpty())
            return false;

        File file(path);

        if (!file)
            return false;

        ErrorDisplayPassthru(g_sys, g.eb, false, editorView->Load(&file));
        return true;
    }

    void WorldcraftScene::p_NewDocument()
    {
        p_UnloadDocument();

        // FIXME
        return;
    }

    bool WorldcraftScene::p_SaveDocument()
    {
        File file(path, true);

        if (!file)
            return false;

        ErrorDisplayPassthru(g_sys, g.eb, false, editorView->Serialize(&file));
        return true;
    }

    void WorldcraftScene::p_UnloadDocument()
    {
        // FIXME
    }

    void WorldcraftScene::PositionToolbarSelectionShadow()
    {
        auto icons =                ui->FindWidget<gameui::StaticImage>("toolbar.icons");
        auto selection_shadow =     ui->FindWidget<gameui::StaticImage>("toolbar.selection_shadow");

        if (icons != nullptr && selection_shadow != nullptr)
            selection_shadow->ShowAt(icons->GetPos() + Int3(0, icons->GetSize().x * toolbarSelection, 0));
    }

    void WorldcraftScene::SetMaterial(const String& name)
    {
        auto bigButton = ui->FindWidget<MaterialButtonBig>("materialPanel.bigButton");

        if (bigButton != nullptr)
        {
            bigButton->DropResources();
            bigButton->SetLabel(name);
            bigButton->SetTexture(GetTextureForMaterial(name));
            bigButton->AcquireResources();
        }

        if (materialPanel)
            materialPanel->Layout();

        currentMaterialName = name;
    }

    const char* WorldcraftScene::TryGetResourceClassName(const std::type_index& resourceClass)
    {
        if (resourceClass == typeid(ITexture))
            return "RenderingKit::ITexture";
        else
            return nullptr;
    }
}
