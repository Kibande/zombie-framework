#pragma once

#include <Container/ContainerBase.hpp>

#include <framework/base.hpp>

namespace example
{
    std::shared_ptr<zfw::IScene> CreateMapSelectionScene(Container::ContainerApp* app);
}
