
#include "n3d_gl_internal.hpp"

#include "SMAA/AreaTex.h"
#include "SMAA/SearchTex.h"

namespace n3d
{
    // FIXME: Releasing resources

    SMAA::SMAA()
    {
        active = false;
    }

    bool SMAA::Init()
    {
        /*// FIXME: Viewport-dependent stuff

        // Textures
        areaTex.reset(new GLTexture(TEXTURE_BILINEAR));
        areaTex->SetAlignment(1);
        areaTex->Init(media::TexFormat::TEX_RG8, Int2(AREATEX_WIDTH, AREATEX_HEIGHT), areaTexBytes);

        searchTex = new GLTexture(0);
        searchTex->SetAlignment(1);
        searchTex->Init(media::TexFormat::TEX_R8, Int2(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT), searchTexBytes);

        // Framebuffers
        edgesTex = new GLFrameBuffer(gl.renderViewport, false);
        blendTex = new GLFrameBuffer(gl.renderViewport, false);

        // Shaders
        String header =
            "#define SMAA_GLSL_3 1\n"
            "#define SMAA_PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)\n"
            "#define SMAA_PRESET_HIGH 1\n\n";

        String header_vs = header +
            "#define SMAA_ONLY_COMPILE_VS 1\n\n";

        String header_ps = header +
            "#define SMAA_ONLY_COMPILE_PS 1\n\n";

        // Edge
        if ((smaa_edge = glr->LoadShaderProgram("ntile/smaa/edge", header_vs, header_ps, 0)) == nullptr)
            return false;

        smaa_edge_colorTex = smaa_edge->GetUniform("colorTex");

        // Weight
        if ((smaa_weight = glr->LoadShaderProgram("ntile/smaa/weight", header_vs, header_ps, 0)) == nullptr)
            return false;

        smaa_weight_edgesTex = smaa_weight->GetUniform("edgesTex");
        smaa_weight_areaTex = smaa_weight->GetUniform("areaTex");
        smaa_weight_searchTex = smaa_weight->GetUniform("searchTex");

        // Blending
        if ((smaa_blending = glr->LoadShaderProgram("ntile/smaa/blending", header_vs, header_ps, 0)) == nullptr)
            return false;

        smaa_blending_colorTex = smaa_blending->GetUniform("colorTex");
        smaa_blending_blendTex = smaa_blending->GetUniform("blendTex");*/
        return true;
    }

    void SMAA::Process(GLFrameBuffer* image)
    {
        if (!active)
            return;

        /*
        // Set-up Edge Detection
        glr->SetFrameBuffer(nullptr);
        glr->SetFrameBuffer(edgesTex);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Edge Detection
        glr->SetShaderProgram(smaa_edge);
        smaa_edge->SetUniformInt(smaa_edge_colorTex, 0);

        proj.SetOrtho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
        proj.R_SetUpMatrices();
        glr->DrawTexture(image->texture, Int2(-1, -1), Int2(2, 2), Int2(0, 0), gl.renderViewport);

        // Set-up Weight Calculation
        glr->SetFrameBuffer(nullptr);
        glr->SetFrameBuffer(blendTex);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Weight Calculation
        glr->SetShaderProgram(smaa_weight);
        smaa_weight->SetUniformInt(smaa_weight_edgesTex, 0);
        smaa_weight->SetUniformInt(smaa_weight_areaTex, 1);
        smaa_weight->SetUniformInt(smaa_weight_searchTex, 2);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, areaTex->tex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, searchTex->tex);
        glActiveTexture(GL_TEXTURE0);

        proj.SetOrtho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
        proj.R_SetUpMatrices();
        glr->DrawTexture(edgesTex->texture, Int2(-1, -1), Int2(2, 2), Int2(0, 0), gl.renderViewport);

        // Set-up final step
        glr->SetFrameBuffer(nullptr);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Blending
        glr->SetShaderProgram(smaa_blending);
        smaa_weight->SetUniformInt(smaa_blending_colorTex, 0);
        smaa_weight->SetUniformInt(smaa_blending_blendTex, 1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, blendTex->texture->tex);
        glActiveTexture(GL_TEXTURE0);

        proj.SetOrtho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
        proj.R_SetUpMatrices();
        glr->DrawTexture(image->texture, Int2(-1, -1), Int2(2, 2), Int2(0, 0), gl.renderViewport);
        */
    }
}
