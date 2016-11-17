#include "ContainerMapView.hpp"

#include <framework/colorconstants.hpp>

#include <RenderingKit/utility/TexturedPainter.hpp>

namespace example {
    extern TexturedPainter3D<> g_tp;

    bool Ent_SkyBox::Init() {
        zombie_assert(tex.ByPath("media/texture/skytex.jpg"));

        return true;
    }

    void Ent_SkyBox::Draw(const UUID_t* modeOrNull) {
        if (modeOrNull == nullptr) {
            const Float3 size(500.0f, 500.0f, 500.0f);
            g_tp.DrawFilledCuboid(*tex, pos + size * 0.5f, -size, RGBA_WHITE);
        }
    }
}
