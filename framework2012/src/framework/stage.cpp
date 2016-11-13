
#include <framework/entity.hpp>
#include <framework/rendering.hpp>
#include <framework/stage.hpp>

#include <gameui/gameui.hpp>

namespace zfw
{
    StageLayerBG::StageLayerBG(CFloat4& colour)
        : colour(colour)
    {
    }
    
    void StageLayerBG::Draw()
    {
        zr::R::SetClearColour(colour);
        zr::R::Clear();
    }

    StageLayerWorld::StageLayerWorld(World* world, Name drawName, IProjectionHandler* projHandler)
            : world(world), drawName(drawName), projHandler(projHandler)
    {
    }

    void StageLayerWorld::Draw()
    {
        li_tryCall(projHandler, SetUpProjection());

        world->Draw(drawName);
    }
    
    int StageLayerWorld::OnEvent(int h, const Event_t* ev)
    {
        iterate2 (i, world->entities)
        {
            if ((h = i->OnEvent(h, ev)) >= 1)
                return h;
        }
        
        return h;
    }
    
    void StageLayerWorld::OnFrame(double delta)
    {
    }
    
    StageLayerUI::StageLayerUI(gameui::UIContainer* container)
            : container(container)
    {
    }
    
    StageLayerUI::~StageLayerUI()
    {
        delete container;
    }
    
    void StageLayerUI::Draw()
    {
        zr::R::Set2DView();
        zr::R::EnableDepthTest(false);

        container->Draw();
    }
    
    Stage::~Stage()
    {
        iterate2 (i, layers)
            delete i;
    }
    
    void Stage::AddLayer(StageLayer* layer)
    {
        layers.add(layer);
    }
    
    StageLayerBG* Stage::AddLayerBG(CFloat4& colour)
    {
        auto layer = new StageLayerBG(colour);
        layers.add(layer);
        return layer;
    }
    
    StageLayerWorld* Stage::AddLayerWorld(World* world, Name drawName, zfw::IProjectionHandler *projHandler)
    {
        auto layer = new StageLayerWorld(world, drawName, projHandler);
        layers.add(layer);
        return layer;
    }
    
    StageLayerUI* Stage::AddLayerUI(gameui::UIContainer *container)
    {
        auto layer = new StageLayerUI(container);
        layers.add(layer);
        return layer;
    }
    
    void Stage::DrawStage()
    {
        iterate2 (i, layers)
            i->Draw();
    }
    
    int Stage::OnStageEvent(int h, const Event_t* ev)
    {
        iterate2 (i, layers)
        {
            if ((h = i->OnEvent(h, ev)) >= 1)
                return h;
        }
        
        return h;
    }

    void Stage::OnStageFrame(double delta)
    {
        iterate2 (i, layers)
            i->OnFrame(delta);
    }

    void Stage::RemoveLayer(StageLayer* layer)
    {
        layers.removeItem(layer);
    }
}