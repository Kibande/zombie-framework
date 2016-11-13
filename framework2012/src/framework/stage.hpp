#pragma once

#include <framework/framework.hpp>

namespace gameui
{
    class UIContainer;
}

namespace zfw {
    class IEntity;

    class IProjectionHandler
    {
        public:
            virtual void SetUpProjection() = 0;
    };

    class StageLayer
    {
        public:
            virtual ~StageLayer() {}

            virtual void Draw() = 0;
            virtual int OnEvent(int h, const Event_t* ev) { return h; }
            virtual void OnFrame(double delta) {}
    };

    class StageLayerBG : public StageLayer
    {
        friend class Stage;

        Float4 colour;

        private:
            StageLayerBG(CFloat4& colour);

        public:
            virtual void Draw() override;
    };
    
    class StageLayerWorld : public StageLayer
    {
        friend class Stage;

        World* world;
        Name drawName;
        IProjectionHandler* projHandler;

        private:
            StageLayerWorld(World* world, Name drawName, IProjectionHandler* projHandler);
            
        public:
            virtual ~StageLayerWorld() {}

            virtual void Draw() override;
            virtual int OnEvent(int h, const Event_t* ev) override;
            virtual void OnFrame(double delta) override;
    };

    class StageLayerUI : public StageLayer
    {
        friend class Stage;

        gameui::UIContainer *container;

        private:
            StageLayerUI(gameui::UIContainer *container);

        public:
            virtual ~StageLayerUI();

            virtual void Draw() override;
    };

    class Stage
    {
        List<StageLayer*> layers;

        public:
            virtual ~Stage();

            void                DrawStage();
            int                 OnStageEvent(int h, const Event_t* ev);
            void                OnStageFrame(double delta);

            void                AddLayer(StageLayer* layer);
            StageLayerBG*       AddLayerBG(CFloat4& colour);
            StageLayerWorld*    AddLayerWorld(World* world, Name drawName, IProjectionHandler* projHandler);
            StageLayerUI*       AddLayerUI(gameui::UIContainer *container);
            void                RemoveLayer(StageLayer* layer);
    };
}