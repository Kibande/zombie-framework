#pragma once

#include "../RenderingKit.hpp"

#include <framework/errorcheck.hpp>
#include <framework/resourcemanager.hpp>

namespace RenderingKit
{
    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    template <int dummy = 0>
    class Picking
    {
        public:
            Picking();

            bool Init(IRenderingManager* rm);
            void Shutdown();

            void BeginPicking();
            void EndPicking(Int2 samplePosInFB, uint32_t* index_out);

            void SetPickingIndex(uint32_t index);
            uint32_t SetPickingIndexNext();

        protected:
            IRenderingManager* rm;

            shared_ptr<IShader> shader;
            shared_ptr<IMaterial> material;
            intptr_t u_PickingColor;

            uint32_t nextIndex;

            void DropResources();
    };

    // ====================================================================== //
    //  class Picking
    // ====================================================================== //

    template <int dummy>
    Picking<dummy>::Picking()
    {
        rm = nullptr;
    }

    template <int dummy>
    bool Picking<dummy>::Init(IRenderingManager* rm)
    {
        this->rm = rm;

        shader = rm->GetSharedResourceManager()->GetResource<IShader>("path=RenderingKit/picking", zfw::RESOURCE_REQUIRED, 0);
        zombie_ErrorCheck(shader);

        u_PickingColor = shader->GetUniformLocation("u_PickingColor");

        material = rm->CreateMaterial("RenderingKit.Picking.material", shader);
        zombie_ErrorCheck(material);

        return true;
    }

    template <int dummy>
    void Picking<dummy>::Shutdown()
    {
        DropResources();
    }

    template <int dummy>
    void Picking<dummy>::DropResources()
    {
        material.reset();
    }

    template <int dummy>
    void Picking<dummy>::BeginPicking()
    {
        rm->VertexCacheFlush();
        rm->SetMaterialOverride(material.get());

        nextIndex = 1;
    }

    template <int dummy>
    void Picking<dummy>::EndPicking(Int2 samplePosInFB, uint32_t* index_out)
    {
        rm->VertexCacheFlush();
        rm->SetMaterialOverride(nullptr);

        Byte4 colour;
        rm->ReadPixel(samplePosInFB, &colour, nullptr);

        *index_out = colour.r | (colour.g << 8) | (colour.b << 16);
    }

    template <int dummy>
    void Picking<dummy>::SetPickingIndex(uint32_t index)
    {
        Float4 colour;
        colour.r = (index & 0xFF)           / 255.0f;
        colour.g = ((index >> 8) & 0xFF)    / 255.0f;
        colour.b = ((index >> 16) & 0xFF)   / 255.0f;
        colour.a = 1.0f;

        rm->FlushMaterial(material.get());
        shader->SetUniformFloat4(u_PickingColor, colour);
    }

    template <int dummy>
    uint32_t Picking<dummy>::SetPickingIndexNext()
    {
        SetPickingIndex(nextIndex);
        return nextIndex++;
    }
}
