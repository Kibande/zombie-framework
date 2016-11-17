#include "ContainerMapView.hpp"

#include <RenderingKit/WorldGeometry.hpp>

namespace example {
    bool Ent_WorldGeometry::Init() {
        zombie_assert(geom.ByRecipe(recipe.c_str()));

        return true;
    }

    void Ent_WorldGeometry::Draw(const UUID_t* modeOrNull) {
        if (modeOrNull != nullptr)
            return;

        geom->Draw();
    }
}
