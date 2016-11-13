#pragma once

#include "../framework/framework.hpp"

namespace contentkit
{
    using namespace zfw;

    class SplashScene : public IScene
    {
        IScene *mainScene;

        render::ITexture *tex;
        glm::ivec2 pos;

        int f;
        bool clearBackground;

        public:
            SplashScene(IScene* mainScene, bool clearBackground) : mainScene(mainScene), clearBackground(clearBackground)
            {
            }

            virtual void DrawScene();
            virtual void Init();
            virtual void OnFrame( double delta );
            virtual void ReleaseMedia();
            virtual void ReloadMedia();
    };
}