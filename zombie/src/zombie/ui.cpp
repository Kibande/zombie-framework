
#include "zombie.hpp"

namespace zombie
{
    static const float fadetime = 0.2f;

    UIOverlay::UIOverlay() : state(FADE_IN), alpha(0.0f)
    {
    }

    void UIOverlay::FadeOut()
    {
        state = FADE_OUT;
    }

    bool UIOverlay::IsFinished()
    {
        return state == FINISHED;
    }

    void UIOverlay::UIOverlay_Draw( const glm::vec2& pos, const glm::vec2& size )
    {
    }

    void UIOverlay::UIOverlay_OnFrame( double delta )
    {
        if (state == FADE_IN)
        {
            alpha += (float)(delta / fadetime);

            if ( alpha >= 1.0f )
            {
                alpha = 1.0f;
                state = ACTIVE;
            }
        }
        else if (state == FADE_OUT)
        {
            alpha -= (float)(delta / fadetime);

            if ( alpha <= 0.0f )
            {
                alpha = 0.0f;
                state = FINISHED;
            }
        }
    }
}