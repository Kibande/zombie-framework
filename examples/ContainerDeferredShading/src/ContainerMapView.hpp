#pragma once

#include <Container/ContainerBase.hpp>

#include <framework/pointentity.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/scene.hpp>

#include <RenderingKit/RenderingKit.hpp>
#include <RenderingKit/utility/BasicPainter.hpp>

namespace example
{
    constexpr static bool useDeferred = true;

    std::shared_ptr<zfw::IScene> CreateSandboxScene(Container::ContainerApp* app);
}
