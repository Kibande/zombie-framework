
#include "n3d_gl_internal.hpp"

#include <framework/colorconstants.hpp>
#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/params.hpp>

#include <utility>

namespace n3d
{
    class GraphicsState
    {
        public:
            String name;
            IResourceManager2* res;

            GraphicsState(IResourceManager2* res, String&& name) : name(std::move(name)), res(res) {}
            virtual ~GraphicsState() {}

            virtual bool Init(const char* normparamSets) = 0;
            virtual GraphicsState* Clone(String&& name) = 0;

            virtual void Draw(const UUID_t* uuidOrNull, const Float3& pos, float alphaBlend) = 0;
            virtual void DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size) = 0;
            virtual Float2 GetSize() = 0;
            virtual bool SetParam(const char* key, const char* value) = 0;
    };

    class GraphicsStateImage : public GraphicsState
    {
        ITexture* tex;
        Float2 uv[2];

        public:
            GraphicsStateImage(IResourceManager2* res, String&& name);

            virtual bool Init(const char* normparamSets) override;
            virtual GraphicsState* Clone(String&& name) override;

            virtual void Draw(const UUID_t* uuidOrNull, const Float3& pos, float alphaBlend) override;
            virtual void DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size) override;
            virtual Float2 GetSize() override;
            virtual bool SetParam(const char* key, const char* value) override;
    };

    /*class GraphicsStateSolid : implements(GraphicsState)
    {
        Byte4 colour;

        public:
            GraphicsStateSolid(IResourceManager2* res, const char* name);
    };*/

    class GLGraphics2 : public IGraphics2
    {
        List<GraphicsState*> states;

        GraphicsState* currentState;

        public:
            GLGraphics2();
            virtual ~GLGraphics2();

            //virtual bool Init(IResourceManager2* res, const char* normparams, int flags) override;

            virtual void Draw(const UUID_t* uuidOrNull, const Float3& pos) override;
            virtual void DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size) override;

            virtual Float2 GetSize() override { return currentState->GetSize(); }
            virtual bool SelectState(const char* stateNameOrNull) override;
    };

    static void SetErrorParamMissing(const char* normparamSets, const char* stateType, const char* paramName)
    {
        ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                "desc", sprintf_255("Missing required param '%s' for Graphics State '%s'", paramName, stateType),
                "params", normparamSets
                );
    }

    // ====================================================================== //
    //  class GraphicsStateImage
    // ====================================================================== //

    GraphicsStateImage::GraphicsStateImage(IResourceManager2* res, String&& name)
            : GraphicsState(res, std::move(name))
    {
    }

    GraphicsState* GraphicsStateImage::Clone(String&& name)
    {
        auto copy = new GraphicsStateImage(res, std::move(name));
        copy->tex = tex;
        return copy;
    }

    void GraphicsStateImage::Draw(const UUID_t* uuidOrNull, const Float3& pos, float alphaBlend)
    {
        glr->SetColour(RGBA_WHITE);
        glr->DrawTexture(tex, Int2(pos.x, pos.y));
    }

    void GraphicsStateImage::DrawStretched(const UUID_t* uuidOrNull, const Float3& pos, const Float3& size)
    {
        glr->SetColour(RGBA_WHITE);
        glr->DrawTexture(tex, Int2(pos.x, pos.y), Int2(size.x, size.y), Int2(pos.x, pos.y), Int2(size.x, size.y));
    }

    Float2 GraphicsStateImage::GetSize()
    {
        return Float2(tex->GetSize());
    }

    bool GraphicsStateImage::Init(const char* normparamSets)
    {
        if (tex == nullptr)
            return SetErrorParamMissing(normparamSets, "image", "src"),
                    false;

        return true;
    }

    bool GraphicsStateImage::SetParam(const char* key, const char* value)
    {
        if (strcmp(key, "src") == 0)
            res->ResourceByPath(&tex, value);

        return true;
    }

    // ====================================================================== //
    //  class GLGraphics2
    // ====================================================================== //

    IGraphics2* p_CreateGLGraphics2(String&& recipe)
    {
        //return new GLGraphics2();
        zombie_assert(false);
        return nullptr;
    }

    GLGraphics2::GLGraphics2()
    {
        currentState = nullptr;
    }

    GLGraphics2::~GLGraphics2()
    {
        states.deleteAllItems();
    }

    void GLGraphics2::Draw(const UUID_t* uuidOrNull, const Float3& pos)
    {
        currentState->Draw(uuidOrNull, pos, 1.0f);
    }

    void GLGraphics2::DrawStretched(const UUID_t* uuidOrNull, const Float3& pos, const Float3& size)
    {
        currentState->DrawStretched(uuidOrNull, pos, size);
    }
    /*
    bool GLGraphics2::Init(IResourceManager2* res, const char* normparamSets, int flags)
    {
        GraphicsState* state = nullptr;
        GraphicsState* lastState = nullptr;

        String stateName;

        const char* p_params = normparamSets;

        for (;;)
        {
            const char *key, *value;

            bool first = true;

            while (Params::Next(p_params, key, value))
            {
                if (state == nullptr && strcmp(key, "type") == 0)
                {
                    if (strcmp(value, "image") == 0)
                        state = new GraphicsStateImage(res, std::move(stateName));
                    //else if (strcmp(value, "solid") == 0)
                    //    state = new GraphicsStateSolid();
                    else
                        return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                                "desc", sprintf_255("Invalid Graphics type '%s'", value),
                                "params", normparamSets,
                                "function", li_functionName),
                                false;

                    states.add(state);
                }
                else
                {
                    if (state == nullptr)
                    {
                        ZFW_ASSERT(lastState != nullptr)

                        state = lastState->Clone(std::move(stateName));
                        states.add(state);
                    }

                    if (!state->SetParam(key, value))
                        return false;
                }
            }

            if (state == nullptr)
                return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                        "desc", "No valid data specified for Graphics State",
                        "params", normparamSets,
                        "functionName", li_functionName),
                        false;

            if (!state->Init(normparamSets))
                return false;

            lastState = state;
            state = nullptr;

            if (*p_params != ';')
                break;

            p_params++;

            while (Params::Next(p_params, key, value))
            {
                if (strcmp(key, "state") == 0)
                    stateName = value;
            }

            if (*p_params != ';')
                break;

            p_params++;
        }

        currentState = states[0];

        // Validate size
        const auto size = currentState->GetSize();

        iterate2 (i, states)
        {
            if (i->GetSize() != size)
                return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                        "desc", "Graphics error: mismatch in State sizes",
                        "params", normparamSets,
                        "functionName", li_functionName),
                        false;
        }

        return true;
    }
    */
    bool GLGraphics2::SelectState(const char* stateNameOrNull)
    {
        iterate2 (i, states)
            if (i->name == stateNameOrNull)
            {
                currentState = i;
                return true;
            }

        return false;
    }
}
