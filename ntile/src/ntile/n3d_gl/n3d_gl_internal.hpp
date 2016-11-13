#pragma once

#include "n3d_gl.hpp"

namespace n3d
{
    enum State { ST_GL_BLEND, ST_GL_DEPTH_TEST, ST_GL_TEXTURE_2D, ST_MAX };
    static const GLenum StateMapping[ST_MAX] = { GL_BLEND, GL_DEPTH_TEST, GL_TEXTURE_2D };

    enum ClientState { CL_GL_COLOR_ARRAY, CL_GL_NORMAL_ARRAY, CL_GL_TEXTURE_COORD_ARRAY, CL_GL_VERTEX_ARRAY, CL_MAX };
    static const GLenum ClientStateMapping[CL_MAX] = { GL_COLOR_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_VERTEX_ARRAY };

    class OpenGLState
    {
        GLuint currentTex0, currentIBO, currentVBO;

        bool clEnabled[CL_MAX], stEnabled[ST_MAX];

        public:
            int numMaterialSetups, numTextureBinds, numVboBinds, numDrawCalls;
            size_t sceneBytesSent;

        public:
            Int2 renderWindow, renderViewport;

            OpenGLState() : currentTex0(0), currentIBO(0), currentVBO(0)
            {
                // Statically allocated class - don't do anything crazy here

                memset(clEnabled, 0, sizeof(clEnabled));
                memset(stEnabled, 0, sizeof(stEnabled));
            }

            void BindTexture0(GLuint tex)
            {
                if (currentTex0 == tex)
                    return;

                glBindTexture(GL_TEXTURE_2D, tex);
                numTextureBinds++;
                currentTex0 = tex;
            }

            void BindIBO(GLuint tex)
            {
                if (currentIBO == tex)
                    return;

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tex);
                numVboBinds++;
                currentIBO = tex;
            }

            void BindVBO(GLuint tex)
            {
                if (currentVBO == tex)
                    return;

                glBindBuffer(GL_ARRAY_BUFFER, tex);
                numVboBinds++;
                currentVBO = tex;
            }

            void ClearStats()
            {
                numMaterialSetups = 0;
                numTextureBinds = 0;
                numVboBinds = 0;
                numDrawCalls = 0;
                sceneBytesSent = 0;
            }

            void SetState(State state, bool enabled)
            {
                if (stEnabled[state] == enabled)
                    return;

                if (enabled)
                    glEnable(StateMapping[state]);
                else
                    glDisable(StateMapping[state]);

                stEnabled[state] = enabled;
            }

            void SetClientState(ClientState state, bool enabled)
            {
                if (clEnabled[state] == enabled)
                    return;

                if (enabled)
                    glEnableClientState(ClientStateMapping[state]);
                else
                    glDisableClientState(ClientStateMapping[state]);

                clEnabled[state] = enabled;
            }
    };

    extern ISystem* g_sys;
    extern GLRenderer* glr;
    extern OpenGLState gl;
    extern unique_ptr<GLVertexCache> vertexCache;
}
