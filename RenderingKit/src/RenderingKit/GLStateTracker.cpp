
#include "RenderingKitImpl.hpp"

namespace RenderingKit
{
    static const GLenum StateMapping[ST_MAX] = { GL_BLEND, GL_DEPTH_TEST };
    static const GLenum ClientStateMapping[CL_MAX] = { GL_COLOR_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_VERTEX_ARRAY };

    int numMaterialSetups, numTextureBinds, numVboBinds, numDrawCalls, numVFSetups;

    static unsigned int activeTexture, firstEnabledTex, numEnabledTex;
    static GLuint currentProgram, currentTex[MAX_TEX], currentIBO, currentVBO;
    static GLVertexFormat* currentVF;
    static IGLMaterial* currentMat;
    static bool clEnabled[CL_MAX], stEnabled[ST_MAX];

    void GLStateTracker::Init()
    {
        activeTexture = 0;
        firstEnabledTex = 0;
        numEnabledTex = 0;

        currentIBO = 0;
        currentProgram = 0;
        memset(currentTex, 0, sizeof(currentTex));
        currentVBO = 0;
        currentVF = nullptr;
        currentMat = nullptr;
        memset(clEnabled, 0, sizeof(clEnabled));
        memset(stEnabled, 0, sizeof(stEnabled));
    }

    void GLStateTracker::BindArrayBuffer(GLuint handle)
    {
        if (currentVBO == handle)
            return;
                
        glBindBuffer(GL_ARRAY_BUFFER, handle);
        currentVBO = handle;

        numVboBinds++;
    }

    void GLStateTracker::BindElementArrayBuffer(GLuint handle)
    {
        if (currentIBO == handle)
            return;
                
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
        currentIBO = handle;

        numVboBinds++;
    }

    void GLStateTracker::BindTexture(int unit, GLuint handle)
    {
        SetActiveTexture(unit);

        if (currentTex[unit] == handle)
            return;

        glBindTexture(GL_TEXTURE_2D, handle);
        currentTex[unit] = handle;

        numTextureBinds++;
    }

    void GLStateTracker::ClearStats()
    {
        numMaterialSetups = 0;
        numTextureBinds = 0;
        numVboBinds = 0;
        numDrawCalls = 0;
    }

    void GLStateTracker::InvalidateTexture(GLuint handle)
    {
        for (size_t i = 0; i < lengthof(currentTex); i++)
            if (currentTex[i] == handle)
                BindTexture(i, 0);
    }

    void GLStateTracker::SetActiveTexture(int unit)
    {
        if (activeTexture == unit)
            return;

        glActiveTexture(GL_TEXTURE0 + unit);
        activeTexture = unit;
    }

    void GLStateTracker::SetClientState(ClientState state, bool enabled)
    {
        if (clEnabled[state] == enabled)
            return;

        if (enabled)
            glEnableClientState(ClientStateMapping[state]);
        else
            glDisableClientState(ClientStateMapping[state]);

        clEnabled[state] = enabled;
    }

    void GLStateTracker::SetEnabledTextureUnits(unsigned int first, unsigned int count)
    {
        if (first == firstEnabledTex && count == numEnabledTex)
            return;

        for (unsigned int i = firstEnabledTex; i < firstEnabledTex + numEnabledTex && i < first; i++)
        {
            SetActiveTexture(i);
            glDisable(GL_TEXTURE_2D);
        }

        for (unsigned int i = first; i < first + count; i++)
        {
            SetActiveTexture(i);
            glEnable(GL_TEXTURE_2D);
        }

        for (unsigned int i = first + count; i < firstEnabledTex + numEnabledTex; i++)
        {
            SetActiveTexture(i);
            glDisable(GL_TEXTURE_2D);
        }

        firstEnabledTex = first;
        numEnabledTex = count;
    }

    void GLStateTracker::SetState(State state, bool enabled)
    {
        if (stEnabled[state] == enabled)
            return;

        if (enabled)
            glEnable(StateMapping[state]);
        else
            glDisable(StateMapping[state]);

        stEnabled[state] = enabled;
    }

    void GLStateTracker::UseProgram(GLuint handle)
    {
        if (currentProgram == handle)
            return;

        glUseProgram(handle);
        currentProgram = handle;
    }
}
