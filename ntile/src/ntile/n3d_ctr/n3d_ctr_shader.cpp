
#include "n3d_ctr_internal.hpp"

#include <framework/system.hpp>

namespace n3d
{
    CTRShaderProgram::CTRShaderProgram(String&& path)
            : state(CREATED), path(std::forward<String>(path))
    {
        glInitProgramCTR(&shader);
        shbin = nullptr;
    }

    CTRShaderProgram::~CTRShaderProgram()
    {
        glShutdownProgramCTR(&shader);
    }

    int CTRShaderProgram::GetUniform(const char* name)
    {
        return glGetUniformLocation((GLuint) &shader, name);
    }

    bool CTRShaderProgram::Preload(IResourceManager2* resMgr)
    {
        if (shbin)
            return true;

        String shbinPath = (String) path + ".shbin";

        unique_ptr<InputStream> file(g_sys->OpenInput(shbinPath));
        zombie_assert(file);        // FIXME: proper error checking
        size = file->getSize();

        shbin = malloc(size);
        zombie_assert(file->read(shbin, size) == size);

        return true;
    }

    bool CTRShaderProgram::Realize(IResourceManager2* resMgr)
    {
        glLoadProgramBinary2CTR((GLuint) &shader, shbin, size, GL_MEMORY_TRANSFER_CTR);
        shbin = nullptr;

        return true;
    }

    void CTRShaderProgram::SetUniformInt(int id, int value)
    {
        zombie_assert(false);
        /*if (id < 0)
            return;

        GPU_SetUniform(id, (u32*) &value, 1);*/
    }

    void CTRShaderProgram::SetUniformVec3(int id, const Float3& value)
    {
        if (id < 0)
            return;

        const Float4 data(1.0f, value.z, value.y, value.x);
        glUniform4fv(id, 1, &data[0]);
    }

    void CTRShaderProgram::SetUniformVec4(int id, const Float4& value)
    {
        if (id < 0)
            return;

        const Float4 data(value.w, value.z, value.y, value.x);
        glUniform4fv(id, 1, &data[0]);
    }

    void CTRShaderProgram::Unload()
    {
        free(shbin);
        shbin = nullptr;
    }

    void CTRShaderProgram::Unrealize()
    {
    }
}
