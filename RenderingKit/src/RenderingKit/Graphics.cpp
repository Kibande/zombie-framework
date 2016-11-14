
#include "RenderingKitImpl.hpp"

#include <RenderingKit/utility/TexturedPainter.hpp>

#include <framework/resourcemanager.hpp>

#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/params.hpp>

#include <utility>

namespace RenderingKit
{
    using namespace zfw;

    typedef TexturedPainter2D<Float2, Float2, Byte4> PainterType;

    class GraphicsState
    {
        public:
            String name;
            IResourceManager* res;

            GraphicsState(IResourceManager* res, String&& name) : name(std::move(name)), res(res) {}
            virtual ~GraphicsState() {}

            virtual bool Init(ErrorBuffer_t* eb, const char* normparamSets) = 0;
            virtual GraphicsState* Clone(String&& name) = 0;

            virtual void Draw(const UUID_t* modeOrNull, const Float3& pos, const Float3& size, PainterType& painter, float alphaBlend) = 0;
            virtual Float2 GetSize() = 0;
            virtual bool SetParam(const char* key, const char* value) = 0;
    };

    class GraphicsStateImage : public GraphicsState
    {
        shared_ptr<ITexture> tex;
        Float2 uv[2];

        public:
            GraphicsStateImage(IResourceManager* res, String&& name);
            GraphicsStateImage(IResourceManager* res, String&& name, shared_ptr<ITexture>&& tex);

            virtual bool Init(ErrorBuffer_t* eb, const char* normparamSets) override;
            virtual GraphicsState* Clone(String&& name) override;

            virtual void Draw(const UUID_t* modeOrNull, const Float3& pos, const Float3& size, PainterType& painter, float alphaBlend) override;
            virtual Float2 GetSize() override;
            virtual bool SetParam(const char* key, const char* value) override;

            void SetUV(const Float2 uv[2]) { this->uv[0] = uv[0]; this->uv[1] = uv[1]; }
    };

    class GLGraphics : public IGLGraphics
    {
        public:
            GLGraphics();
            ~GLGraphics();

            virtual bool Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    zfw::IResourceManager* res, const char* normparamSets, int flags) override;

            virtual bool InitWithTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    shared_ptr<ITexture> texture, const Float2 uv[2]) override;

            virtual const char* GetName() override { return name.c_str(); }

            virtual void Draw(const UUID_t* modeOrNull, const Float3& pos) override;
            virtual void DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size) override;

            virtual Float2 GetSize() override { return currentState->GetSize(); }
            virtual bool SelectState(const char* stateNameOrNull) override;

        private:
            String name;
            List<GraphicsState*> states;

            GraphicsState* currentState;

            PainterType tp;
    };

    static void SetErrorParamMissing(ErrorBuffer_t* eb, const char* normparamSets, const char* stateType, const char* paramName)
    {
        ErrorBuffer::SetError2(eb, EX_INVALID_ARGUMENT, 2,
                "desc", sprintf_255("Missing required param '%s' for Graphics State '%s'", paramName, stateType),
                "params", normparamSets
                );
    }

    // ====================================================================== //
    //  class GraphicsStateImage
    // ====================================================================== //

    GraphicsStateImage::GraphicsStateImage(IResourceManager* res, String&& name)
            : GraphicsState(res, std::move(name))
    {
        tex = nullptr;
        uv[0] = Float2(0.0f, 0.0f);
        uv[1] = Float2(1.0f, 1.0f);
    }

    GraphicsStateImage::GraphicsStateImage(IResourceManager* res, String&& name, shared_ptr<ITexture>&& tex)
            : GraphicsState(res, std::move(name))
    {
        this->tex = move(tex);
        uv[0] = Float2(0.0f, 0.0f);
        uv[1] = Float2(1.0f, 1.0f);
    }

    GraphicsState* GraphicsStateImage::Clone(String&& name)
    {
        auto copy = new GraphicsStateImage(res, std::move(name));
        copy->tex = tex;
        return copy;
    }

    void GraphicsStateImage::Draw(const UUID_t* modeOrNull, const Float3& pos, const Float3& size, PainterType& painter, float alphaBlend)
    {
        painter.DrawFilledRectangleUV(tex.get(), Float2(pos), Float2(size), uv, RGBA_WHITE_A(alphaBlend));
    }

    Float2 GraphicsStateImage::GetSize()
    {
        return Float2(tex->GetSize());
    }

    bool GraphicsStateImage::Init(ErrorBuffer_t* eb, const char* normparamSets)
    {
        if (tex == nullptr)
            return SetErrorParamMissing(eb, normparamSets, "image", "src"),
                    false;

        return true;
    }

    bool GraphicsStateImage::SetParam(const char* key, const char* value)
    {
        if (strcmp(key, "src") == 0)
        {
            tex.reset();

            // FIXME: Normalize path
            tex = res->GetResourceByPath<ITexture>(value, RESOURCE_REQUIRED, 0);

            if (tex == nullptr)
                return false;
        }

        return true;
    }

    // ====================================================================== //
    //  class GLGraphics
    // ====================================================================== //

    shared_ptr<IGLGraphics> p_CreateGLGraphics()
    {
        return std::make_shared<GLGraphics>();
    }

    GLGraphics::GLGraphics()
    {
        currentState = nullptr;
    }

    GLGraphics::~GLGraphics()
    {
        states.deleteAllItems();
    }

    void GLGraphics::Draw(const UUID_t* modeOrNull, const Float3& pos)
    {
        currentState->Draw(modeOrNull, pos, Float3(currentState->GetSize(), 0.0f), tp, 1.0f);
    }

    void GLGraphics::DrawStretched(const UUID_t* modeOrNull, const Float3& pos, const Float3& size)
    {
        currentState->Draw(modeOrNull, pos, size, tp, 1.0f);
    }

    bool GLGraphics::Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
            zfw::IResourceManager* res, const char* normparamSets, int flags)
    {
        this->name = normparamSets;

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
                        return ErrorBuffer::SetError2(eb, EX_INVALID_ARGUMENT, 2,
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
                return ErrorBuffer::SetError2(eb, EX_INVALID_ARGUMENT, 3,
                        "desc", "No valid data specified for Graphics State",
                        "params", normparamSets,
                        "function", li_functionName),
                        false;

            if (!state->Init(eb, normparamSets))
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
                return ErrorBuffer::SetError2(eb, EX_INVALID_ARGUMENT, 3,
                        "desc", "Graphics error: mismatch in State sizes",
                        "params", normparamSets,
                        "function", li_functionName),
                        false;
        }

        if (!tp.Init(rm))
            return false;

        return true;
    }

    bool GLGraphics::InitWithTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    shared_ptr<ITexture> texture, const Float2 uv[2])
    {
        this->name = (String) "graphics:" + texture->GetName();

        auto state = new GraphicsStateImage(nullptr, String(), move(texture));
        state->SetUV(uv);

        currentState = state;
        states.add(currentState);

        if (!tp.Init(rm))
            return false;

        return true;
    }

    bool GLGraphics::SelectState(const char* stateNameOrNull)
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
