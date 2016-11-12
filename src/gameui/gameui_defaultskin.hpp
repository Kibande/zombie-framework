
#include "../framework/framework.hpp"

#include <framework/rendering.hpp>

namespace gameui
{
    typedef glm::ivec2  Int2;
    typedef glm::vec4   Float4;

    using namespace zfw;

    static const Int2 button_padding(6, 4);
    static const Int2 button_shadowSize(1, 1);

    static const int scrollbar_height   = 10;
    static const int scrollbar_pad      = 1;
    static const int scrollbar_width    = 10;

    static const int round_rect_r       = 16;

    class UIResManager;

    typedef void (*UILoaderCallback)(UIResManager *res, const char *fileName, cfx2::Node node, const WidgetLoadProperties& properties, Widget*& widget, WidgetContainer*& container);

    class UIResManager
    {
        struct FontEntry {
            String path;
            int size, flags;
            //render::IFont* font;
            zr::Font* font;
        };

        struct TexEntry {
            render::ITexture* tex;
            int referenceCount;
        };

        glm::vec4 buttonColour, panelColour, shadowColour, textColour;

        List<FontEntry> fonts;
        //HashMap<String, TexEntry> textures;
        HashMap<String, zr::ITexture*> textures;

        UILoaderCallback customLoaderCb;
        String fontPrefix;

        render::ITexture *ui_tex;

        protected:
            Widget* Load(const char* fileName, cfx2::Node node);

        public:
            UIResManager();
            ~UIResManager();

            size_t AddCustomFont(const char* path, int size, int flags);

            Float4 GetButtonColour() const { return buttonColour; }
            //render::IFont* GetFont(size_t i) { return fonts[i].font; }
            zr::Font* GetFont(size_t i) { return fonts[i].font; }
            Float4 GetPanelColour() const { return panelColour; }
            Float4 GetShadowColour() const { return shadowColour; }
            Float4 GetTextColour() const { return textColour; }

            zr::ITexture* GetTexture(const char* path, bool required);
            zr::ITexture* GetUITexture() { return ui_tex; }

            Widget* Load(const char* fileName);
            size_t Load(const char* fileName, WidgetContainer* container);
            int ParseFont(const char *font);

            void SetCustomLoaderCallback(UILoaderCallback cb) { customLoaderCb = cb; }
            void SetFontPrefix(const char* prefix) { fontPrefix = prefix; }

            void SetButtonColour(const glm::vec4& colour) { buttonColour = colour; }
            void SetPanelColour(const glm::vec4& colour) { panelColour = colour; }
            void SetShadowColour(const glm::vec4& colour) { shadowColour = colour; }
            void SetTextColour(const glm::vec4& colour) { textColour = colour; }

            void DrawRoundRect(CShort2& a, CShort2& b, CByte4 colour);

            void ReleaseTexture(render::ITexture *tex);

            void ReleaseMedia();
            void ReloadMedia();
    };

    class UIGraphics
    {
        UIResManager* res;

        // locator

        Reference<zr::ITexture> tex;
        Float2 uv[2];

        public:
            void Draw(Int2 pos, Int2 size)
            {
                zr::R::SetTexture(tex);
                zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), uv[0], uv[1], RGBA_WHITE);
            }

            // TODO: AcquireResources, DropResources
    };

    class Common
    {
        public:
            static void DrawHorScrollbar(Int2 pos, Int2 size, int scrollbar_x, int scrollbar_w)
            {
                render::R::SetBlendColour(COLOUR_GREY(1.0f, 0.2f));
                render::R::DrawRectangle(glm::vec2(pos.x, pos.y + size.y - scrollbar_height), glm::vec2(pos.x + size.x, pos.y + size.y), 0.0f);

                render::R::SetBlendColour(COLOUR_GREY(0.0f, 0.6f));
                render::R::DrawRectangle(glm::vec2(pos.x + scrollbar_pad + scrollbar_x, pos.y + size.y - scrollbar_height + scrollbar_pad),
                        glm::vec2(pos.x + scrollbar_pad + scrollbar_x + scrollbar_w, pos.y + size.y - scrollbar_pad), 0.0f);
            }

            static void DrawVertScrollbar(Int2 pos, Int2 size, int scrollbar_y, int scrollbar_h)
            {
                zr::R::SetTexture(nullptr);

                zr::R::DrawFilledRect(Short2(pos.x, pos.y), Short2(pos.x + size.x, pos.y + size.y), RGBA_GREY(1.0f, 0.2f));

                zr::R::DrawFilledRect(Short2(pos.x + size.x - scrollbar_width + scrollbar_pad, pos.y + scrollbar_pad + scrollbar_y),
                        Short2(pos.x + size.x - scrollbar_pad, pos.y + scrollbar_pad + scrollbar_y + scrollbar_h), RGBA_GREY(0.0f, 0.6f));
            }
    };

    class ButtonPainter
    {
        UIResManager* res;

        String label;
        size_t font;
        Byte4 colour, shadow_colour;

        bool mouseOver;
        float mouseOverAmt;

        public:
            ButtonPainter(UIResManager* res, const char* label, size_t font)
                    : res(res), label(label), font(font),
                    mouseOver(false), mouseOverAmt(0.0f)
            {
                colour = res->GetButtonColour() * 255.0f;
                shadow_colour = res->GetShadowColour() * 255.0f;
            }

            void Draw(Int2 pos, Int2 size)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos + button_shadowSize), Short2(pos + size), shadow_colour);

                if (mouseOverAmt > 0.01f)
                    zr::R::DrawFilledRect(Short2(pos), Short2(pos + size - button_shadowSize),
                            Byte4(colour.r, colour.g, colour.b, colour.a + mouseOverAmt * 0.2f * 255));

                //if (mouseOverAmt > 0.01f)
                //    res->DrawRoundRect(Short2(pos), Short2(pos + size - button_shadowSize),
                //            Byte4(colour.r, colour.g, colour.b, colour.a + mouseOverAmt * 0.2f * 255));

                res->GetFont(font)->DrawAsciiString(label, Int2(pos.x + size.x / 2, pos.y + size.y / 2), RGBA_WHITE, ALIGN_CENTER | ALIGN_MIDDLE);
            }

            void GetMinSize(Int2& minSize)
            {
                if (res->GetFont(font) != nullptr)
                    minSize = res->GetFont(font)->MeasureAsciiString(label) + button_padding * 2 + Int2(1, 1);
            }

            void SetColour(const glm::vec4& colour) { this->colour = colour * 255.0f; }

            void SetMouseOver(bool mouseOver)
            {
                this->mouseOver = mouseOver;
            }

            void SetFont(size_t font) { this->font = font; }

            void Update(double delta)
            {
                if (mouseOver)
                    mouseOverAmt = (float)glm::min(mouseOverAmt + delta / 0.1f, 1.0);
                else
                    mouseOverAmt = (float)glm::max(mouseOverAmt - delta / 0.1f, 0.0);
            }
    };

    class ComboBoxPainter
    {
        UIResManager* res;

        public:
            ComboBoxPainter(UIResManager* res) : res(res)
            {
            }

            void DrawContentsBg(const Int2& pos, const Int2& size)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), RGBA_BLACK_A(0.6f));
            }

            void DrawIcon(const Int2& pos, const Int2& size, bool open)
            {
                /*zr::R::SetTexture(nullptr);
                zr::R::DrawFilledTri(Short2(pos.x + size.x / 2, pos.y + size.y * 3 / 4),
                        Short2(pos.x + size.x / 8, pos.y + size.y / 4),
                        Short2(pos.x + size.x * 7 / 8, pos.y + size.y / 4),
                        RGBA_WHITE);*/

                zr::R::SetTexture(res->GetUITexture());

                if (!open)
                    zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), Float2(0.0f / 128.0f, 96.0f / 128.0f), Float2(32.0f / 128.0f, 128.0f / 128.0f), RGBA_WHITE);
                else
                    zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), Float2(32.0f / 128.0f, 96.0f / 128.0f), Float2(64.0f / 128.0f, 128.0f / 128.0f), RGBA_WHITE);
            }

            void DrawMainBg(const Int2& pos, const Int2& size, bool open)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), RGBA_BLACK_A(0.8f));
            }

            void DrawSelectionBg(const Int2& pos, const Int2& size)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), RGBA_WHITE_A(0.25f));
            }

            int GetIconMinSize() const
            {
                return 16;
            }

            Int2 GetPadding() const
            {
                return Int2(4, 4);
            }
    };

    class ListBoxPainter
    {
        public:
            ListBoxPainter()
            {
            }

            void BeginClip(CInt2& pos, CInt2& size)
            {
                zr::R::PushClippingRect(pos.x, pos.y, size.x, size.y);
            }

            void EndClip()
            {
                zr::R::PopClippingRect();
            }
    };

    class PanelPainter
    {
        Byte4 colour;

        public:
            PanelPainter(UIResManager* res)
            {
                colour = res->GetPanelColour() * 255.0f;
            }

            void Draw(CInt2& pos, CInt2& size)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), colour);
            }

            void SetColour(CFloat4& colour) { this->colour = colour * 255.0f; }
    };

    class ScrollBarPainter
    {
        public:
            void DrawVScrollBar(CInt2& pos, CInt2& size, int scrollbar_y, int scrollbar_h)
            {
                Common::DrawVertScrollbar(pos, size, scrollbar_y, scrollbar_h);
            }

            int GetScrollbarWidth() const { return scrollbar_width; }
    };

    class StaticImagePainter
    {
        UIResManager* res;

        String path;
        Byte4 colour;

        ImageMapping mapping;

        render::ITexture* tex;

        public:
            StaticImagePainter(UIResManager* res, const char* path, ImageMapping mapping)
                    : res(res), path(path), mapping(mapping)
            {
                colour = RGBA_WHITE;
                tex = nullptr;
            }

            void Draw(Int2 pos, Int2 size)
            {
                const Int2 texsize = tex->GetSize();
                zr::R::SetTexture(tex);

                if (mapping == IMAGEMAP_NORMAL)
                {
                    zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), Float2(0.0f, 0.0f), Float2(1.0f, 1.0f), colour);
                }
                else if (mapping == IMAGEMAP_HSLICE)
                {
                    zr::R::DrawFilledRect(Short2(pos.x, pos.y), Short2(texsize.x / 2, pos.y + size.y), Float2(0.0f, 0.0f), Float2(0.5f, 1.0f), colour);
                    zr::R::DrawFilledRect(Short2(pos.x + texsize.x / 2, pos.y), Short2(pos.x + size.x - texsize.x / 2, pos.y + size.y), Float2(0.5f, 0.0f), Float2(0.5f, 1.0f), colour);
                    zr::R::DrawFilledRect(Short2(pos.x + size.x - texsize.x / 2, pos.y), Short2(pos.x + size.x, pos.y + size.y), Float2(0.5f, 0.0f), Float2(1.0f, 1.0f), colour);
                }
                else if (mapping == IMAGEMAP_VSLICE)
                {
                    zr::R::DrawFilledRect(Short2(pos.x, pos.y), Short2(pos.x + size.x, pos.y + texsize.y / 2), Float2(0.0f, 0.0f), Float2(1.0f, 0.5f), colour);
                    zr::R::DrawFilledRect(Short2(pos.x, pos.y + texsize.y / 2), Short2(pos.x + size.x, pos.y + size.y - texsize.y / 2), Float2(0.0f, 0.5f), Float2(1.0f, 0.5f), colour);
                    zr::R::DrawFilledRect(Short2(pos.x, pos.y + size.y - texsize.y / 2), Short2(pos.x + size.x, pos.y + size.y), Float2(0.0f, 0.5f), Float2(1.0f, 1.0f), colour);
                }
            }

            void ReleaseMedia()
            {
                res->ReleaseTexture(tex);
            }

            void ReloadMedia()
            {
                if (tex == nullptr)
                {
                    if ((tex = res->GetTexture(path, false)) == nullptr)
                        tex = res->GetTexture("media/notexture.png", true);

                    //minSize = tex->GetSize();
                    //size = glm::max(size, minSize);
                }
            }

            /*void GetMinSize(Int2& minSize)
            {
                if (res->GetFont(font) != nullptr)
                    minSize = res->GetFont(font)->MeasureAsciiString(label);
            }*/

            void SetColour(CFloat4& colour) { this->colour = colour * 255.0f; }
            void SetMapping(ImageMapping mapping) { this->mapping = mapping; }
    };

    class StaticTextPainter
    {
        UIResManager* res;

        String label;
        size_t font;
        Byte4 colour;

        public:
            StaticTextPainter(UIResManager* res, const char* label, size_t font)
                    : res(res), label(label), font(font)
            {
                colour = res->GetTextColour() * 255.0f;
            }

            void Draw(Int2 pos, Int2 size)
            {
                res->GetFont(font)->DrawAsciiString(label, Int2(pos.x + size.x / 2, pos.y + size.y / 2), colour, ALIGN_CENTER | ALIGN_MIDDLE);
            }

            const char* GetLabel() { return label; }

            void GetMinSize(Int2& minSize)
            {
                if (res->GetFont(font) != nullptr)
                    minSize = res->GetFont(font)->MeasureAsciiString(label);
            }

            void SetColour(const glm::vec4& colour) { this->colour = colour * 255.0f; }
            void SetFont(size_t font) { this->font = font; }
            void SetLabel(const char* label) { this->label = label; }
    };

    class TextBoxPainter
    {
        UIResManager* res;

        size_t font;
        glm::vec4 colour;

        zr::Font* fontPtr;

        public:
            TextBoxPainter(UIResManager* res, size_t font) : res(res), font(font), fontPtr(nullptr)
            {
                colour = res->GetTextColour();
            }

            void Draw(Int2 pos, Int2 size)
            {
            }

            void ReloadMedia()
            {
                fontPtr = res->GetFont(font);
            }

            void SetFont(size_t font) { this->font = font; }
    };

    class TreeBoxPainter
    {
        UIResManager* res;

        size_t font;
        Byte4 colour;

        zr::Font* fontPtr;
        render::ITexture* tex;
        //int hspacing;

        public:
            TreeBoxPainter(UIResManager* res, size_t font) : res(res), font(font), fontPtr(nullptr)
            {
                colour = res->GetTextColour() * 255.0f;
            }

            void DrawArrow(Int2 pos, bool open)
            {
                render::R::SetBlendColour(COLOUR_WHITEA(open ? 1.0f : 0.5f));
                render::R::DrawTexture(tex, glm::vec3(pos.x, pos.y, 0.0f), glm::vec2(tex->GetSize()));
            }

            void DrawLabel(Int2 pos, const char* label)
            {
                fontPtr->DrawAsciiString(label, Int2(pos.x + 3, pos.y + 2), colour, ALIGN_LEFT | ALIGN_TOP);
            }

            void DrawLabelHighlight(Int2 pos, Int2 size)
            {
                render::R::SetBlendColour(COLOUR_WHITEA(0.25f));
                render::R::DrawRectangle(glm::vec2(pos), glm::vec2(pos + size), 0.0f);
            }

            Int2 GetArrowSize() const { return tex->GetSize(); }

            int GetLabelXOffset() const { return 4; }

            int GetScrollbarHeight() const { return scrollbar_height; }
            int GetScrollbarWidth() const { return scrollbar_width; }

            Int2 GetSize(const char* label) {
                return fontPtr->MeasureAsciiString(label) + glm::ivec2(7, 4);
            }

            int GetSpacingY() const { return 0; }

            void ReloadMedia()
            {
                fontPtr = res->GetFont(font);
                tex = res->GetTexture("media/treectrl.png", true);
                //hspacing = glm::max(fontPtr->GetLineSkip(), tex->GetSize().y);
            }

            void SetFont(size_t font) { this->font = font; }
    };

    class PopupPainter
    {
        UIResManager *res;

        Byte4 colour;

        public:
            PopupPainter(UIResManager* res) : res(res)
            {
                colour = res->GetPanelColour() * 255.0f;
            }

            void Draw(Int2 pos, Int2 size)
            {
                res->DrawRoundRect(Short2(pos), Short2(pos + size), colour);
            }
    };

    class WindowPainter
    {
        UIResManager* res;

        Byte4 colour, shadow_colour;
        size_t titleFont;
        int titleHeight;

        public:
            WindowPainter(UIResManager* res, size_t titleFont) : res(res), titleFont(titleFont)
            {
                colour = res->GetPanelColour() * 255.0f;
                shadow_colour = res->GetShadowColour() * 255.0f;

                titleHeight = 0;
            }

            void AcquireResources()
            {
                titleHeight = 4 + res->GetFont(titleFont)->GetHeight() + 4;
            }

            void DrawWindow(CInt2& pos, Int2& size)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos.x + 2, pos.y + 2), Short2(pos.x + size.x + 2, pos.y + size.y + 2), shadow_colour);
                zr::R::DrawFilledRect(Short2(pos), Short2(pos + size), colour);
            }

            void DrawWindowTitle(CInt2& pos, CInt2& size, const char* title)
            {
                zr::R::SetTexture(nullptr);
                zr::R::DrawFilledRect(Short2(pos.x, pos.y - titleHeight), Short2(pos.x + size.x, pos.y), RGBA_GREY(0.2f, 0.6f));

                res->GetFont(titleFont)->DrawAsciiString(title, Int2(pos.x + size.x / 2, pos.y - titleHeight / 2),
                    RGBA_WHITE, ALIGN_CENTER | ALIGN_MIDDLE);
            }

            int GetTitleHeight() const { return titleHeight; }
    };
}
