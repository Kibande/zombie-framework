
#include "splashscene.hpp"

namespace contentkit
{
    using namespace zfw::render;

    void SplashScene::DrawScene()
    {
        if (clearBackground)
            R::Clear();

        R::Set2DView();

        R::SetBlendColour(COLOUR_WHITE);
        R::DrawTexture(tex, pos);
    }

    void SplashScene::Init()
    {
        f = 3;

        ReloadMedia();
    }
            
    void SplashScene::OnFrame( double delta )
    {
        if (--f == 0)
            Sys::ChangeScene(mainScene);
    }

    void SplashScene::ReleaseMedia()
    {
        li::release(tex);
    }

    void SplashScene::ReloadMedia()
    {
        tex = render::Loader::LoadTexture("contentkit/splash.png", true);
        pos = (glm::ivec2(Var::GetInt("r_viewportw", true), Var::GetInt("r_viewporth", true)) - tex->GetSize()) / 2;

        R::EnableDepthTest( false );
        R::SetRenderFlags(R_COLOURS | R_UVS);
        R::SetClearColour(COLOUR_GREY(0.3f, 0.0f));
    }
}