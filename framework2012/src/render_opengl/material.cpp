
#include "rendering.hpp"

namespace zr
{
    Material::Material()
    {
    }

    void Material::Bind()
    {
        zr::R::SetTexture(difftexture);
    }

    Material* Material::LoadFromDataBlock(InputStream* stream)
    {
        Object<Material> mat = new Material;

        mat->name = stream->readString();

        String diffuseName = stream->readString();
        mat->difftexture = render::Loader::LoadTexture(diffuseName, true);

        return mat.detach();
    }

    METHOD_NOT_IMPLEMENTED(void Material::Unbind())
}