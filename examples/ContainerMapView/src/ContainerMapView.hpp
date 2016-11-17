#pragma once

#include <Container/ContainerBase.hpp>

#include <framework/pointentity.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/scene.hpp>

#include <RenderingKit/RenderingKit.hpp>
#include <RenderingKit/utility/BasicPainter.hpp>

#include <littl/List.hpp>

namespace example
{
    using namespace RenderingKit;
    using namespace zfw;

    class Ent_WorldGeometry : public PointEntityBase
    {
    public:
        Ent_WorldGeometry(const char* recipe) : recipe(recipe) {}

        virtual bool Init() override;

        virtual void Draw(const UUID_t* modeOrNull) override;

    private:
        std::string recipe;

        Resource<RenderingKit::IWorldGeometry> geom;
    };

    class Ent_SkyBox : public PointEntityBase
    {
    public:
        virtual bool Init() override;

        virtual void Draw(const UUID_t* modeOrNull) override;

    private:
        Resource<ITexture> tex;

        IEntity* linked_ent;
    };

    shared_ptr<zfw::IScene> CreateMapViewScene(Container::ContainerApp* app);
}
