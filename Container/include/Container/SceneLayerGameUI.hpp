#pragma once

#include <Container/SceneGraph.hpp>

#include <gameui/gameui.hpp>

namespace Container {
    class SceneLayerGameUI : public SceneLayerScreenSpace {
    public:
        SceneLayerGameUI(zfw::IEngine* sys) : ui(sys) {}

        virtual void DrawContents() override {
            ui.Draw();
        }

        gameui::UIContainer* GetUIContainer() { return &ui; }

    private:
        gameui::UIContainer ui;
    };
}
