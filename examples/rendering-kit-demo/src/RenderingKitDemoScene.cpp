
#include "RenderingKitDemo.hpp"

#include <framework/colorconstants.hpp>
#include <framework/event.hpp>
#include <framework/modulehandler.hpp>
#include <framework/nativedialogs.hpp>
#include <framework/resourcemanager.hpp>

#include <gameui/dialogs.hpp>

#include <RenderingKit/RenderingKitUtility.hpp>

namespace client
{
    using std::make_shared;
    using std::make_unique;

    static const char* kUISavedStatePath = APP_NAME ".uistate";
    static const char* kRecentFilesPath = APP_NAME ".recent";

    static bool IsCharacterEvent(const VkeyState_t& input, Unicode::Char& char_out, bool& pressed_out)
    {
        if (input.vkey.type == VKEY_KEY)
        {
            char_out = input.vkey.key;
            pressed_out = (input.flags & VKEY_PRESSED) ? true : false;
            return true;
        }
        else
            return false;
    }

    RenderingKitDemoScene::RenderingKitDemoScene()
    {
        ui = nullptr;
        uiThemer = nullptr;
        mdl = nullptr;
        selAnimation = nullptr;
        edAnimation = nullptr;
        selJoint = nullptr;
        selBoneAnim = nullptr;
        selBoneAnimKeyframe = nullptr;
        timeScale = 1.0f;

        geomBuffer = nullptr;
        cam2D = nullptr;
        cam3D = nullptr;
    }

    bool RenderingKitDemoScene::Init()
    {
        recentFiles.reset(studio::RecentFiles::Create(g_sys));
        recentFiles->Load(kRecentFilesPath);

        g_world = new EntityWorld(g_sys);

        return AcquireResources();
    }

    void RenderingKitDemoScene::Shutdown()
    {
        if (ui != nullptr)
        {
            ZFW_DBGASSERT(ui->SaveUIState(kUISavedStatePath))
        }

        recentFiles->Save(kRecentFilesPath);

        delete g_world;
        g_world = nullptr;

        recentFiles.reset();

        DropResources();

        delete ui;
        uiThemer.reset();
    }

    bool RenderingKitDemoScene::AcquireResources()
    {
        // Scene
        irm->SetClearColour(COLOUR_GREY(0.15f));

        cam2D = irm->CreateCamera("cam2D");
        cam2D->SetOrthoScreenSpace();
        
        cam3D = irm->CreateCamera("cam3D");
        cam3D->SetClippingDist(4.0f, 512.0f);
        cam3D->SetPerspective();
        cam3D->SetVFov(45.0f * f_pi / 180.0f);
        cam3D->SetView(Float3(0.0f, 64.0f, 64.0f), Float3(0.0f, 0.0f, 0.0f), Float3(0.0f, -0.707f, 0.707f));
        cam.SetCamera(cam3D.get());

        geomBuffer = irm->CreateGeomBuffer("RenderingKitDemoScene/geomBuffer");

        g_world->AddEntity(make_shared<editor_grid>(this, Float3(0.0f, 0.0f, 0.0f), Float2(1.0f, 1.0f), Int2(16, 16)));

        // UI
        uiThemer.reset(CreateRKUIThemer());

        if (uiThemer == nullptr)
            return g_sys->DisplayError(g_eb, true), false;

        uiThemer->Init(g_sys, irk, g.res);
        // FIXME: Cleaner way to set up fonts
        uiThemer->GetFontId("path=media/fonts/DejaVuSans.ttf,size=11,flags=SHADOW");
        uiThemer->GetFontId("path=media/fonts/DejaVuSans-Bold.ttf,size=11,flags=SHADOW");

        if (!uiThemer->AcquireResources())
            return g_sys->DisplayError(g_eb, true), false;
        
        gameui::Widget::SetMessageQueue(g_msgQueue);
        ui = new gameui::UIContainer(g_sys);
        ui->SetOnlineUpdate(false);
        
        gameui::UILoader ldr(g_sys, ui, uiThemer.get());
        ldr.Load("rkui.cfx2", ui, false);

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

        ui->AcquireResources();
        ui->SetArea(Int3(0, 0, 0), Int2(1280, 720));
        ui->SetOnlineUpdate(true);

        ui->RestoreUIState(kUISavedStatePath);

        //
        //path = "player.cfx2";
        //p_LoadModel();

        return true;
    }

    void RenderingKitDemoScene::DropResources()
    {
        p_UnloadModel();

        delete ui;
        ui = nullptr;

        uiThemer.reset();

        geomBuffer.reset();
        cam3D.reset();
        cam2D.reset();
    }

    void RenderingKitDemoScene::DrawScene()
    {
        irm->Clear();

        irm->SetRenderState(RK_DEPTH_TEST, 1);
        irm->SetCamera(cam3D.get());

        g_world->Draw(nullptr);

        if (mdl != nullptr)
        {
            mdl->Draw();
            irm->SetRenderState(RK_DEPTH_TEST, 0);
            mdl->StudioDrawBones();
        }

        irm->SetRenderState(RK_DEPTH_TEST, 0);
        //irm->SetCamera(cam2D.get());
        irm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);
        ui->Draw();
    }

    void RenderingKitDemoScene::OnAnimationEnding(ntile_model::CharacterModel* mdl, ntile_model::CharacterModel::Animation* anim) 
    {
        if (anim == edAnimation)
            p_SelectAnimation(nullptr);
    }

    void RenderingKitDemoScene::OnFrame(double delta)
    {
        p_ProcessEvents();

        ui->OnFrame(delta);
    }

    void RenderingKitDemoScene::OnTicks(int ticks)
    {
        while (ticks--)
        {
            if (mdl != nullptr)
            {
                mdl->AnimationTick();
                mdl->StudioAnimationTick();
            }
        }
    }

    void RenderingKitDemoScene::p_JumpToKeyframe(int dir)
    {
        selBoneAnimKeyframeIndex = mdl->StudioGetKeyframeIndexNear(selBoneAnim, selAnimationTime, dir);

        if (selBoneAnimKeyframeIndex >= 0)
        {
            selBoneAnimKeyframe = mdl->StudioGetKeyframeByIndex(selBoneAnim, selBoneAnimKeyframeIndex);

            selAnimationTime = selBoneAnimKeyframe->time;
            mdl->StudioSetAnimationTime(edAnimation, selAnimationTime);

            auto timeline = ui->FindWidget<gameui::Slider>("timeline");

            if (timeline != nullptr)
                timeline->SetValue(selAnimationTime);

            p_UpdateTimelineLabel();
            p_UpdatePitchYawRollSliders();
        }
        else
            selBoneAnimKeyframe = nullptr;
    }

    bool RenderingKitDemoScene::p_LoadModel()
    {
        p_UnloadModel();

        if (path.isEmpty())
            return false;

        mdl = make_unique<ntile_model::CharacterModel>(g_eb, g.res);
        mdl->StudioSetOnAnimationEnding(this);

        if (!mdl->Load(path))
        {
            client::g_sys->DisplayError(g_eb, false);

            return false;
        }

        auto joints = ui->FindWidget<gameui::TreeBox>("joints");

        if (joints != nullptr)
        {
            joints->Clear();
            pr_AddJoints(joints, nullptr, mdl->StudioGetJoint(nullptr, 0));
            joints->ExpandAll();
        }

        auto animations = ui->FindWidget<gameui::TreeBox>("animations");

        if (animations != nullptr)
        {
            animations->Clear();

            for (size_t i = 0; ; i++)
            {
                const char* n = mdl->StudioGetAnimationName(i);

                if (n != nullptr)
                    animations->Add(n);
                else
                    break;
            }
        }

        auto primitives = ui->FindWidget<gameui::TreeBox>("primitives");

        if (primitives != nullptr)
        {
            primitives->Clear();

            ntile_model::StudioPrimitive_t** mdlPrimitives;
            size_t numPrimitives;

            mdl->StudioGetPrimitivesList(0, mdlPrimitives, numPrimitives);

            for (size_t i = 0; i < numPrimitives; i++)
            {
                auto prim = mdlPrimitives[i];
                primitives->Add(sprintf_63("%i (%g %g %g)+(%g %g %g) %06X", (int)i, prim->a.x, prim->a.y, prim->a.z, prim->b.x, prim->b.y, prim->b.z, prim->colour));
            }
        }

        return true;
    }

    void RenderingKitDemoScene::p_ProcessEvents()
    {
        MessageHeader* msg;
        static Int2 mousePos;       // can't wait to kill this one!

        while ((msg = g_msgQueue->Retrieve(Timeout(0))) != nullptr)
        {
            switch (msg->type)
            {
                case EVENT_MOUSE_MOVE:
                {
                    auto payload = msg->Data<EventMouseMove>();
                    int h = -1;

                    mousePos = Int2(payload->x, payload->y);

                    h = ui->OnMouseMove(h, payload->x, payload->y);

                    if (h < 0)
                        h = cam.HandleMessage(msg);
                    break;
                }

                case EVENT_VKEY:
                {
                    auto payload = msg->Data<EventVkey>();
                    int h = gameui::h_new;
                    int button;
                    Unicode::Char c;
                    bool pressed;

                    if (Vkey::IsMouseButtonEvent(payload->input, button, pressed))
                    {
                        int x = mousePos.x;
                        int y = mousePos.y;

                        h = ui->OnMouseButton(h, button, pressed, x, y);
                    }
                    else if (IsCharacterEvent(payload->input, c, pressed) && pressed)
                    {
                        h = ui->OnCharacter(h, c);

                        if (h > 0)
                            break;
                    }

                    if (h < 0)
                        h = cam.HandleMessage(msg);
                    break;
                }

                case EVENT_WINDOW_CLOSE:
                    g_sys->StopMainLoop();
                    break;

                case gameui::EVENT_CONTROL_USED:
                {
                    auto payload = msg->Data<gameui::EventControlUsed>();

                    if (selBoneAnimKeyframe != nullptr && payload->widget->GetName() == "delKey")
                    {
                        mdl->StudioRemoveKeyframe(edAnimation, selBoneAnim, selBoneAnimKeyframeIndex);

                        p_UpdateSelectedKeyframe();
                    }
                    else if (selAnimation != nullptr && payload->widget->GetName() == "editAnim")
                    {
                        p_SelectAnimation(selAnimation);
                    }
                    else if (selBoneAnim != nullptr && payload->widget->GetName() == "nextKey")
                    {
                        p_JumpToKeyframe(1);
                    }
                    else if (payload->widget->GetName() == "newAnim")
                    {
                        auto dlg = new gameui::MessageDlg(uiThemer.get(), "New Animation", "Animation Name:",
                                gameui::DLG_BTN_OK | gameui::DLG_BTN_CANCEL | gameui::DLG_TEXT_BOX | gameui::DLG_CUSTOM_CONTROLS);
                        dlg->SetName("newAnimDlg");

                        gameui::Table* customControlsTbl = new gameui::Table(2);

                        customControlsTbl->Add(new gameui::StaticText(uiThemer.get(), "Animation Length:"));
                        auto textBox = new gameui::TextBox(uiThemer.get());
                        textBox->SetName("animLength");
                        customControlsTbl->Add(textBox);

                        dlg->AddCustomControl(customControlsTbl);

                        dlg->AcquireResources();
                        //ui->CenterWidget(dlg);
                        ui->PushModal(dlg);
                    }
                    else if (selAnimation != nullptr && payload->widget->GetName() == "playAnim")
                    {
                        if (edAnimation == selAnimation)
                            p_SelectAnimation(nullptr);

                        mdl->StartAnimation(selAnimation);
                    }
                    else if (selBoneAnim != nullptr && payload->widget->GetName() == "prevKey")
                    {
                        p_JumpToKeyframe(-1);
                    }
                    else if (mdl != nullptr && payload->widget->GetName() == "resetAnims")
                    {
                        mdl->StopAllAnimations();
                    }
                    else if (selBoneAnim != nullptr && payload->widget->GetName() == "setKey")
                    {
                        if (selBoneAnimKeyframe == nullptr)
                        {
                            ntile_model::StudioBoneAnimKeyframe_t newKey;

                            mdl->StudioGetBoneAnimationAtTime(selBoneAnim, selAnimationTime, newKey.pitchYawRoll);

                            mdl->StudioAddKeyframeAtTime(edAnimation, selBoneAnim, selAnimationTime, &newKey);

                            p_UpdateSelectedKeyframe();
                        }
                    }

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
                    else if (payload->item->name == "open" || payload->item->name == "openRecentFile")
                    {
                        if (payload->item->name == "openRecentFile")
                            path = payload->menuBar->GetItemLabel(payload->item);
                        else
                            path = NativeDialogs::FileOpen(".", nullptr);

                        // FIXME: Update recent files menu
                        if (p_LoadModel())
                            recentFiles->BumpItem(path);
                    }
                    else if (payload->item->name == "save")
                    {
                        p_SaveModel();
                    }
                    else if (payload->item->name == "saveAs")
                    {
                        path = NativeDialogs::FileSaveAs(".", nullptr);

                        // TODO
                    }
                    else if (payload->item->name == "quit")
                        g_sys->StopMainLoop();

                    break;
                }

                case gameui::EVENT_MESSAGE_DLG_CLOSED:
                {
                    auto payload = msg->Data<gameui::EventMessageDlgClosed>();

                    if (mdl != nullptr && payload->dialog->GetName() == "newAnimDlg" && payload->btnCode == gameui::DLG_BTN_OK)
                    {
                        const char* animName = payload->dialog->GetText();

                        if (animName != nullptr)
                        {
                            auto newAnim = mdl->StudioAddAnimation(animName, 0);
                            p_SelectAnimation(newAnim);

                            auto animations = ui->FindWidget<gameui::TreeBox>("animations");

                            // FIXME: animLength

                            if (animations != nullptr)
                                animations->Add(mdl->StudioGetAnimationName(newAnim));
                        }
                    }

                    delete payload->dialog;
                    break;
                }

                case gameui::EVENT_TREE_ITEM_SELECTED:
                {
                    auto payload = msg->Data<gameui::EventTreeItemSelected>();

                    if (mdl != nullptr && payload->item != nullptr && payload->treeBox->GetName() == "joints")
                    {
                        const char* jname = payload->treeBox->GetItemLabel(payload->item);
                        ntile_model::Joint_t* joint = mdl->FindJoint(jname);

                        if (joint != nullptr)
                        {
                            mdl->StudioSetHighlightedJoint(joint);

                            p_SelectJoint(jname, joint);
                        }
                    }
                    else if (mdl != nullptr && payload->item != nullptr && payload->treeBox->GetName() == "animations")
                    {
                        const char* aname = payload->treeBox->GetItemLabel(payload->item);
                        ntile_model::CharacterModel::Animation* anim = mdl->GetAnimationByName(aname);

                        if (anim != nullptr)
                            selAnimation = anim;
                    }
                    break;
                }

                case gameui::EVENT_VALUE_CHANGED:
                {
                    auto payload = msg->Data<gameui::EventValueChanged>();

                    if (payload->widget->GetName() == "animSpeed")
                    {
                        timeScale = pow(2.0f, payload->floatValue);

                        mdl->SetAnimTimeScale(timeScale);

                        auto animSpeedLabel = ui->FindWidget<gameui::StaticText>("animSpeedLabel");
                        
                        if (animSpeedLabel != nullptr)
                        {
                            if (timeScale >= 1.0f)
                                animSpeedLabel->SetLabel(sprintf_t<63>("x %.1f", timeScale));
                            else
                                animSpeedLabel->SetLabel(sprintf_t<63>("/ %.1f", 1.0f / timeScale));
                        }
                    }
                    else if (edAnimation != nullptr && payload->widget->GetName() == "timeline")
                    {
                        selAnimationTime = payload->floatValue;
                        mdl->StudioSetAnimationTime(edAnimation, selAnimationTime);

                        p_UpdateSelectedKeyframe();

                        p_UpdateTimelineLabel();
                        p_UpdatePitchYawRollSliders();
                    }
                    else if (mdl != nullptr && payload->widget->GetName() == "paused")
                    {
                        mdl->SetAnimTimeScale((payload->value == 0) ? timeScale : 0.0f);
                    }
                    else if (selBoneAnimKeyframe != nullptr && payload->widget->GetName() == "pitch")
                    {
                        selBoneAnimKeyframe->pitchYawRoll.x = payload->floatValue;
                        mdl->StudioUpdateKeyframe(edAnimation, selBoneAnim, selBoneAnimKeyframeIndex);
                    }
                    else if (selBoneAnimKeyframe != nullptr && payload->widget->GetName() == "yaw")
                    {
                        selBoneAnimKeyframe->pitchYawRoll.y = payload->floatValue;
                        mdl->StudioUpdateKeyframe(edAnimation, selBoneAnim, selBoneAnimKeyframeIndex);
                    }
                    else if (selBoneAnimKeyframe != nullptr && payload->widget->GetName() == "roll")
                    {
                        selBoneAnimKeyframe->pitchYawRoll.z = payload->floatValue;
                        mdl->StudioUpdateKeyframe(edAnimation, selBoneAnim, selBoneAnimKeyframeIndex);
                    }

                    break;
                }

                default:
                    cam.HandleMessage(msg);
            }

            msg->Release();
        }
    }

    void RenderingKitDemoScene::p_SaveModel()
    {
        String actualPath;

        if (path.endsWith(".cfx2"))
            actualPath = path.dropRightPart(4) + "zmf";
        else
            actualPath = path;

        unique_ptr<zshared::MediaFile> modelFile(zshared::MediaFile::Create());
        modelFile->SetSectorSize(256);

        if (!modelFile->Open(actualPath, false, true))
        {
            g_sys->Log(kLogError, "MediaFile error: %s", modelFile->GetErrorDesc());
            return;
        }

        modelFile->SetMetadata("media.original_name", path);
        modelFile->SetMetadata("media.authored_by", "unknown");
        modelFile->SetMetadata("media.authored_using", "name=" APP_TITLE ",version=" APP_VERSION ",vendor=" APP_VENDOR);

        if (!mdl->Save(modelFile.get()))
            g_sys->DisplayError(g_eb, false);
    }

    void RenderingKitDemoScene::p_SelectAnimation(ntile_model::CharacterModel::Animation* animation)
    {
        if (edAnimation == animation)
            return;

        if (edAnimation != nullptr)
            mdl->StudioStopAnimationEditing(edAnimation);

        edAnimation = animation;

        selAnimationTime = 0.0f;
        selBoneAnimKeyframeIndex = -1;
        selBoneAnimKeyframe = nullptr;

        auto timeline = ui->FindWidget<gameui::Slider>("timeline");

        if (edAnimation != nullptr)
        {
            mdl->StudioStartAnimationEditing(edAnimation);

            selBoneAnim = (selJoint != nullptr) ? mdl->StudioGetBoneAnimation(edAnimation, selJoint) : nullptr;

            if (timeline != nullptr)
            {
                float dur = mdl->StudioGetAnimationDuration(edAnimation);

                if (dur > 1.0e-3f)
                {
                    timeline->SetRange(0.0f, dur, 1.0f);
                    timeline->SetSnap(gameui::SNAP_ALWAYS);
                    timeline->SetValue(0.0f);
                    timeline->SetEnabled(true);
                }
                else
                    timeline->SetEnabled(false);
            }
        }
        else
        {
            if (timeline != nullptr)
            {
                timeline->SetEnabled(false);
            }
        }

        p_UpdateBoneAnimInfo();
        p_UpdateTimelineLabel();
        p_UpdatePitchYawRollSliders();
    }

    void RenderingKitDemoScene::p_SelectJoint(const char* jointName, ntile_model::Joint_t* joint)
    {
        selJoint = joint;
        selBoneAnim = (edAnimation != nullptr) ? mdl->StudioGetBoneAnimation(edAnimation, joint) : nullptr;

        p_UpdateBoneAnimInfo();
        p_UpdatePitchYawRollSliders();
        p_UpdateSelectedKeyframe();
    }

    void RenderingKitDemoScene::p_UnloadModel()
    {
        mdl.reset();

        selAnimation = nullptr;
        edAnimation = nullptr;
        selJoint = nullptr;
        selBoneAnim = nullptr;
        selBoneAnimKeyframe = nullptr;
        timeScale = 1.0f;
    }

    void RenderingKitDemoScene::p_UpdateBoneAnimInfo()
    {
        auto animatingLabel = ui->FindWidget<gameui::StaticText>("animatingLabel");
        bool hasBoneAnim = (selBoneAnim != nullptr);

        if (animatingLabel != nullptr)
        {
            if (edAnimation != nullptr && selJoint != nullptr)
                animatingLabel->SetLabel(sprintf_255("%s/%s (%s)", mdl->StudioGetAnimationName(edAnimation),
                        selJoint->name.c_str(), hasBoneAnim ? "defined" : "not defined"));
            else
                animatingLabel->SetLabel("---");
        }

        auto toggleBoneAnim = ui->FindWidget<gameui::Button>("toggleBoneAnim");

        if (toggleBoneAnim != nullptr)
        {
            toggleBoneAnim->SetLabel(hasBoneAnim ? "REMOVE" : "CREATE");
        }
    }

    void RenderingKitDemoScene::p_UpdatePitchYawRollSliders()
    {
        Float3 selPitchYawRoll = Float3();

        if (selBoneAnim != nullptr)
            mdl->StudioGetBoneAnimationAtTime(selBoneAnim, selAnimationTime, selPitchYawRoll);

        auto pitch = ui->FindWidget<gameui::Slider>("pitch");
        auto yaw = ui->FindWidget<gameui::Slider>("yaw");
        auto roll = ui->FindWidget<gameui::Slider>("roll");

        auto pitchLabel = ui->FindWidget<gameui::StaticText>("pitchLabel");
        auto yawLabel = ui->FindWidget<gameui::StaticText>("yawLabel");
        auto rollLabel = ui->FindWidget<gameui::StaticText>("rollLabel");

        if (pitch != nullptr) pitch->SetValue(selPitchYawRoll.x);
        if (yaw != nullptr) yaw->SetValue(selPitchYawRoll.y);
        if (roll != nullptr) roll->SetValue(selPitchYawRoll.z);

        if (pitchLabel != nullptr) pitchLabel->SetLabel(sprintf_t<15>("%.1f", selPitchYawRoll.x));
        if (yawLabel != nullptr) yawLabel->SetLabel(sprintf_t<15>("%.1f", selPitchYawRoll.y));
        if (rollLabel != nullptr) rollLabel->SetLabel(sprintf_t<15>("%.1f", selPitchYawRoll.z));
    }

    void RenderingKitDemoScene::p_UpdateSelectedKeyframe()
    {
        selBoneAnimKeyframe = nullptr;

        if (selBoneAnim != nullptr)
        {
            selBoneAnimKeyframeIndex = mdl->StudioGetKeyframeIndexNear(selBoneAnim, selAnimationTime, 0);

            if (selBoneAnimKeyframeIndex >= 0)
                selBoneAnimKeyframe = mdl->StudioGetKeyframeByIndex(selBoneAnim, selBoneAnimKeyframeIndex);
        }   
    }

    void RenderingKitDemoScene::p_UpdateTimelineLabel()
    {
        auto timelineLabel = ui->FindWidget<gameui::StaticText>("timelineLabel");
                        
        if (timelineLabel != nullptr)
            timelineLabel->SetLabel(sprintf_t<63>("%i", (int) selAnimationTime));
    }

    void RenderingKitDemoScene::pr_AddJoints(gameui::TreeBox* widget, gameui::TreeItem_t* parent, ntile_model::Joint_t* joint)
    {
        auto node = widget->Add(joint->name, parent);

        for (size_t i = 0; ; i++)
        {
            auto childJoint = mdl->StudioGetJoint(joint, i);

            if (childJoint != nullptr)
                pr_AddJoints(widget, node, childJoint);
            else
                break;
        }
    }
}
