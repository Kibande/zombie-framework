
#include "RenderingKitImpl.hpp"

namespace RenderingKit
{
    static const GLenum StateMapping[ST_MAX] = { GL_BLEND, GL_DEPTH_TEST };

    static int numTextureBinds, numVboBinds, numDrawCalls, numPrimitivesDrawn;

    static unsigned int activeTexture;
    static GLuint currentProgram, currentTex[MAX_TEX], currentIBO, currentVBO;
    static GLVertexFormat* currentVF;
    static bool stEnabled[ST_MAX];

    void GLStateTracker::Init()
    {
        activeTexture = 0;

        currentIBO = 0;
        currentProgram = 0;
        memset(currentTex, 0, sizeof(currentTex));
        currentVBO = 0;
        currentVF = nullptr;
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
        numTextureBinds = 0;
        numVboBinds = 0;
        numDrawCalls = 0;
        numPrimitivesDrawn = 0;
    }

    int GLStateTracker::GetNumDrawCalls()
    {
        return numDrawCalls;
    }

    int GLStateTracker::GetNumPrimitivesDrawn()
    {
        return numPrimitivesDrawn;
    }

    void GLStateTracker::IncreaseDrawCallCounter(int numPrimitives)
    {
        numDrawCalls++;
        numPrimitivesDrawn += numPrimitives;
    }

    void GLStateTracker::InvalidateTexture(GLuint handle)
    {
        for (size_t i = 0; i < li_lengthof(currentTex); i++)
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
