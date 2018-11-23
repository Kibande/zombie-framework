
// new pallette is 7 colours from #264C6F to #000000

#include <RenderingKit/gameui/RKUIThemer.hpp>

#include <framework/colorconstants.hpp>
#include <framework/errorcheck.hpp>
#include <framework/graphics.hpp>
#include <framework/engine.hpp>

#if ZOMBIE_API_VERSION < 201701
#include <framework/resourcemanager.hpp>
#else
#include <framework/resourcemanager2.hpp>
#endif

namespace RenderingKit
{
    using namespace li;
    using namespace gameui;
    using namespace zfw;

    class RKUIThemer;

    enum { SHADOW_BOTTOM = 1, SHADOW_RIGHT = 2, SHADOW_FULL = 3 };

    struct UIVertex_t
    {
        short pos[2];       // 4
        short uv[2];        // 4
        short texsel;       // 2
		short unused;		// 2
        uint8_t rgba[4];    // 4
    };

    static const VertexAttrib_t uiVertexAttribs[] =
    {
        {"in_Position",     0,  RK_ATTRIB_SHORT_2,  RK_ATTRIB_NOT_NORMALIZED},
        {"in_UV",           4,  RK_ATTRIB_SHORT_2},
        {"in_TexSel",       8,  RK_ATTRIB_SHORT,	RK_ATTRIB_NOT_NORMALIZED},
        {"in_Color",        12, RK_ATTRIB_UBYTE_4},
        {}
    };

    static const Int2 kFontAtlasInitSize(1024, 1024);

    static const Int2 kButtonPadding(5, 3);
    static const Int2 kWindowTitlePadding(8, 2);
    static const Short2 kMenuItemArrowSize(8, 8);
    static const Short2 kTreeBoxArrowSize(10, 10);

    static const short kCheckBoxLabelOffsetX = 8;
    static const short kCheckBoxSizeXY = 12;
    static const short kCheckBoxTickSizeXY = 6;
    static const short kSliderHeight = 4;
    static const short kSliderThumbSizeXY = 10;

    class TextObj
    {
        IFontFace* font;
        Paragraph* p;

        public:
            TextObj()
            {
                p = nullptr;
            }

            ~TextObj()
            {
                if (p != nullptr)
                    font->ReleaseParagraph(p);
            }

            void Draw(const Short2 areaPos, const Short2 areaSize, Byte4 colour = RGBA_WHITE)
            {
                font->DrawParagraph(p, Float3(areaPos.x, areaPos.y, 0.0f), Float2(areaSize.x, areaSize.y), colour);
            }

            intptr_t GetCharAt(const Short2 areaPos, const Short2 areaSize, const Short2 pos, Short2* charPos_out)
            {
                ZFW_ASSERT(p != nullptr)

                const Float3 areaPos_Float(areaPos.x, areaPos.y, 0.0f);
                const Float2 areaSize_Float(areaSize.x, areaSize.y);

                const Float3 posInPS = font->ToParagraphSpace(p, areaPos_Float, areaSize_Float, Float3(pos.x, pos.y, 0.0f));

                intptr_t followingCharIndex = 0;
                Float3 followingCharPosInPS;
                font->GetCharNear(p, posInPS, &followingCharIndex, &followingCharPosInPS);

                *charPos_out = Short2(font->FromParagraphSpace(p, areaPos_Float, areaSize_Float, followingCharPosInPS));
                return followingCharIndex;
            }

            void GetCharPos(const Short2 areaPos, const Short2 areaSize, intptr_t index, Short2* charPos_out)
            {
                ZFW_ASSERT(p != nullptr)

                const Float3 areaPos_Float(areaPos.x, areaPos.y, 0.0f);
                const Float2 areaSize_Float(areaSize.x, areaSize.y);

                Float3 charPosInPS;
                font->GetCharPos(p, index, &charPosInPS);

                *charPos_out = Short2(font->FromParagraphSpace(p, areaPos_Float, areaSize_Float, charPosInPS));
            }

            bool IsEmpty() const
            {
                return (p == nullptr);
            }

            bool LayoutCentered(IFontFace* face, const char* label, int unused);

            Int2 Measure()
            {
                if (p == nullptr)
                    return Int2();

                return font->MeasureParagraph(p);
            }
    };

    class RKUIPainter : public RenderingKit::IFontQuadSink
    {
        IRenderingManager* rm;

#if ZOMBIE_API_VERSION < 201701
        IResourceManager* res = nullptr;
#else
        IResourceManager2* res2 = nullptr;
#endif

        shared_ptr<ITextureAtlas> fontAtlas;
        shared_ptr<IVertexFormat> vertexFormat;

        IMaterial* material;
        shared_ptr<IMaterial> materialReference;

        public:
#if ZOMBIE_API_VERSION < 201701
            RKUIPainter(IRenderingManager* rm, IResourceManager* res);
#else
            RKUIPainter(IRenderingManager* rm, IResourceManager2* res2);
#endif
            ~RKUIPainter() { DropResources(); }

            bool AcquireResources();
            void DropResources();

            void DrawFilledRectangle(const Short2 pos, const Short2 size, const Byte4 colour);
            void DrawFilledRectangle(const Short2 pos, const Short2 size, const Byte4 colours[4]);
            void DrawFilledTriangle(const Short2 abc[3], const Byte4 colours[3]);
            void DrawRectangleShadow(const Short2 pos, const Short2 size, int flags);

            shared_ptr<ITextureAtlas> GetFontAtlas() { return fontAtlas; }

            virtual int OnFontQuad(const Float3& pos, const Float2& size, const Float2 uv[2], Byte4 colour) override;
    };

    class ThemerBase
    {
        protected:
            RKUIThemer* themer;
            Short2 pos;
            Short2 size;

        protected:
            ThemerBase(RKUIThemer* themer)
                    : themer(themer)
            {
            }

            ThemerBase(RKUIThemer* themer, Int3 pos, Int2 size)
                    : themer(themer), pos(pos.x, pos.y), size(size.x, size.y)
            {
            }

            void SetArea(Int3 pos, Int2 size)
            {
                this->pos = Short2(pos.x, pos.y);
                this->size = Short2(size.x, size.y);
            }
    };
}

namespace gameui
{
    using namespace RenderingKit;

    struct IMenuBarThemer::Item_t
    {
        String label;
        Short2 labelSize;
        int flags;

        TextObj labelTxt;
    };

    struct ITreeBoxThemer::Item_t
    {
        String label;
        Short2 labelSize;
        bool expanded, hasChildren, selected;

        TextObj labelTxt;
    };
}

namespace RenderingKit
{
    class RKButtonThemer : public IButtonThemer, public ThemerBase
    {
        String label;

        Byte4 colour, colourMouseOver;
        intptr_t fontid;

        IFontFace* font;
        Int2 minSize;
        bool mouseOver;

        TextObj txt;

        public:
            RKButtonThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* label);
            ~RKButtonThemer();

            virtual bool AcquireResources() override;

            virtual const char* GetLabel() override { return label.c_str(); }
            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetLabel(const char* label) override;
            virtual void SetMouseOver(bool isOver) override { mouseOver = isOver; }

            virtual void Draw() override;
    };

    class RKCheckBoxThemer : public ICheckBoxThemer, public ThemerBase
    {
        String label;
        bool checked;

        Byte4 colour, colourMouseOver;
        intptr_t fontid;

        IFontFace* font;
        Int2 minSize;
        bool mouseOver;

        TextObj txt;

        public:
            RKCheckBoxThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* label, bool checked);
            ~RKCheckBoxThemer();

            virtual bool AcquireResources() override;

            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetChecked(bool isChecked) override { checked = isChecked; }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetMouseOver(bool isOver) override { mouseOver = isOver; }

            virtual void Draw() override;
    };

    class RKComboBoxThemer : public IComboBoxThemer, public ThemerBase
    {
        protected:
            ~RKComboBoxThemer() {}

        public:
            RKComboBoxThemer(RKUIThemer* themer) : ThemerBase(themer) {}

            virtual void Release() { delete this; }

            virtual bool AcquireResources() override { return true; }

            virtual Int2 GetMinSize(Int2 contentsMinSize) { return contentsMinSize; }
            virtual void SetArea(Int3 pos, Int2 size) { ThemerBase::SetArea(pos, size); }
            virtual void SetContentsSize(Int2 size) { }
            virtual void SetExpanded(bool isExpanded) {}
            virtual void SetPopupArea(Int3 pos, Int2 size) {}
            virtual void SetPopupSelectionArea(bool enabled, Int3 pos, Int2 size) {}

            virtual void Draw() {}
            virtual void DrawPopup() {}
    };

    class RKListBoxThemer : public IListBoxThemer
    {
        public:
            RKListBoxThemer() {}
            ~RKListBoxThemer() {}

            virtual bool AcquireResources() override { return true; }

            virtual void Draw() override {}
    };

    class RKMenuBarThemer : public IMenuBarThemer, public ThemerBase
    {
        intptr_t fontid, itemsFontid;
        
        IFontFace* font, * itemsFont;
        Int2 minSize;
        
        public:
            RKMenuBarThemer(RKUIThemer* themer);
            ~RKMenuBarThemer();

            virtual bool AcquireResources() override;
        
            virtual Item_t* AllocItem(Item_t* parent, const char* label) override;
            virtual void Draw() override;
            virtual void DrawItem(Item_t* item, const Int3& pos, const Int2& size, const Int3& labelPos, const Int2& labelSize) override;
            virtual void DrawMenu(const Int3& pos, const Int2& size) override;
            virtual const char* GetItemLabel(Item_t* item) override { return item->label; }
            virtual Int2 GetItemLabelSize(Item_t* item) override;
            virtual void ReleaseItem(Item_t* item) override { delete item; }
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetItemFlags(Item_t* item, int flags) override { item->flags = flags; }
    };

    class RKPanelThemer : public IPanelThemer, public IWindowThemer, public ThemerBase
    {
        bool isWindow;

        Byte4 colour;

        String title;
        TextObj titleTxt;
        Int2 titleMinSize;

        public:
            RKPanelThemer(RKUIThemer* themer, Int3 pos, Int2 size);

            virtual bool AcquireResources() override;

            virtual const char* GetTitle() override { return title; }
            virtual Int2 GetTitleMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetColour(const Float4& colour) override;
            virtual void SetTitle(const char* title) override { this->title = title; titleMinSize.x = -1; }

            virtual void Draw() override;

            void SetTypeToWindow(int flags);
    };

    class RKScrollBarThemer : public IScrollBarThemer
    {
        public:
            RKScrollBarThemer(RKUIThemer* themer) : themer(themer) {}

            virtual bool AcquireResources() override { return true; }

            virtual void DrawVScrollBar(Int2 pos, Int2 size, int y, int h) override;

            virtual int GetWidth() override { return 12; }

        private:
            RKUIThemer* themer;
    };

    class RKSliderThemer : public ISliderThemer, public ThemerBase
    {
        float min, max, tick, recip_range;
        
        public:
            RKSliderThemer(RKUIThemer* themer);

            virtual bool AcquireResources() override { return true; }
        
            virtual void Draw(bool enabled, float value) override;
            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetRange(float min, float max, float tickStep) override;
    };

    class RKStaticImageThemer : public IStaticImageThemer, public ThemerBase
    {
        String path;
        
        Byte4 colour;
        intptr_t fontid;
        
        shared_ptr<IGraphics> g;
        Int2 minSize;
        
        TextObj txt;
        
        public:
            RKStaticImageThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* path);
            ~RKStaticImageThemer();

            virtual bool AcquireResources() override;
            virtual void DropResources() override;
        
            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
    };

    class RKStaticTextThemer : public IStaticTextThemer, public ThemerBase
    {
        String label;
        
        Byte4 colour;
        intptr_t fontid;
        
        IFontFace* font;
        Int2 minSize;
        
        TextObj txt;
        
        public:
            RKStaticTextThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* label);
            ~RKStaticTextThemer();

            virtual bool AcquireResources() override;
        
            virtual const char* GetLabel() override { return label; }
            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetColour(const Float4& colour) override { this->colour = Byte4(colour * 255.0f); }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetLabel(const char* label) override;

            virtual void Draw() override;
    };

    class RKTextBoxThemer : public ITextBoxThemer, public ThemerBase
    {
        String text;
        
        intptr_t fontid;
        
        IFontFace* font;
        Int2 minSize;
        
        TextObj txt;
        Short2 cursorPos;
        bool active;
        
        public:
            RKTextBoxThemer(RKUIThemer* themer, Int3 pos, Int2 size);
            ~RKTextBoxThemer();

            virtual bool AcquireResources() override;

            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual const char* GetText() override { return text; }
            virtual size_t InsertCharacter(size_t pos, gameui::UnicodeChar c);
            virtual void SetActive(bool active) override { this->active = active; }
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetCursorPos(Int3 pos, size_t* followingCharIndex_out) override;
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetMouseOver(bool isOver) override {}
            virtual void SetText(const char* label) override;
    };

    class RKTreeBoxThemer : public ITreeBoxThemer, public ThemerBase
    {
        intptr_t fontid;
        
        IFontFace* font;
        Int2 minSize;
        
        public:
            RKTreeBoxThemer(RKUIThemer* themer, Int3 pos, Int2 size);
            ~RKTreeBoxThemer();

            virtual bool AcquireResources() override;
        
            virtual Item_t* AllocItem(Item_t* parent, const char* label) override;
            virtual void DrawItem(Item_t* item, const Int3& arrowPos, const Int3& labelPos) override;
            virtual Int2 GetArrowSize() override { return Int2(kTreeBoxArrowSize); }
            virtual const char* GetItemLabel(Item_t* item) override { return item->label; }
            virtual Int2 GetItemLabelSize(Item_t* item) override;
            virtual void ReleaseItem(Item_t* item) override { delete item; }
            virtual void SetItemExpanded(Item_t* item, bool expanded) override { item->expanded = expanded; }
            virtual void SetItemHasChildren(Item_t* item, bool hasChildren) override { item->hasChildren = hasChildren; }
            virtual void SetItemLabel(Item_t* item, const char* label) override;
            virtual void SetItemSelected(Item_t* item, bool selected) override { item->selected = selected; }
    };

    class RKUIThemer : public IRKUIThemer
    {
        protected:
            zfw::ErrorBuffer_t* eb;
            zfw::IEngine* sys;
            IRenderingManager* rm;

            IResourceManager* res;

            unique_ptr<RKUIPainter> painter;

        protected:
            struct FontEntry
            {
                String name;
                String path;
                int size, flags;
                shared_ptr<IFontFace> font;
            };
            
            List<FontEntry> fonts;
            
        public:
            RKUIThemer();
            ~RKUIThemer();

#if ZOMBIE_API_VERSION >= 201901
            bool Init(zfw::IEngine* sys, IRenderingManager* rm, IResourceManager2* res) override;
#elif ZOMBIE_API_VERSION >= 201701
            virtual bool Init(zfw::IEngine* sys, IRenderingKit* rk, IResourceManager2* res) override;
#else
            virtual void Init(zfw::IEngine* sys, IRenderingKit* rk, IResourceManager* resRef) override;
#endif

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual intptr_t PreregisterFont(const char* name, const char* params) override;
            virtual intptr_t GetFontId(const char* font) override;

            virtual IButtonThemer*      CreateButtonThemer(Int3 pos, Int2 size, const char* label) override;
            virtual ICheckBoxThemer*    CreateCheckBoxThemer(Int3 pos, Int2 size, const char* label, bool checked) override;
            virtual IComboBoxThemer*    CreateComboBoxThemer(Int3 pos, Int2 size) { return new RKComboBoxThemer(this); }
            virtual IGraphicsThemer*    CreateGraphicsThemer(shared_ptr<IGraphics> g) { return nullptr; }
            virtual IListBoxThemer*     CreateListBoxThemer() { return new RKListBoxThemer(); }
            virtual IMenuBarThemer*     CreateMenuBarThemer() override;
            virtual IPanelThemer*       CreatePanelThemer(Int3 pos, Int2 size) override;
            virtual IScrollBarThemer*   CreateScrollBarThemer(bool horizontal) { return new RKScrollBarThemer(this); }
            virtual ISliderThemer*      CreateSliderThemer() override;
            virtual IStaticImageThemer* CreateStaticImageThemer(Int3 pos, Int2 size, const char* path) override;
            virtual IStaticTextThemer*  CreateStaticTextThemer(Int3 pos, Int2 size, const char* label) override;
            virtual ITextBoxThemer*     CreateTextBoxThemer(Int3 pos, Int2 size) override;
            virtual ITreeBoxThemer*     CreateTreeBoxThemer(Int3 pos, Int2 size) override;
            virtual IWindowThemer*      CreateWindowThemer(IPanelThemer* thm, int flags) override;
        
            virtual IFontFace* GetFont(intptr_t fontIndex) override;

            bool AcquireFontResources(FontEntry& entry);
            RKUIPainter* GetPainter() { return painter.get(); }
            IResourceManager* GetResourceManager() { return res; }
            IEngine* GetSys() { return sys; }
    };

    bool TextObj::LayoutCentered(IFontFace* face, const char* label, int unused)
    {
        ZFW_ASSERT(face != nullptr)

        font = face;

        if (p == nullptr)
            p = font->CreateParagraph();

        font->LayoutParagraph(p, label, RGBA_WHITE, ALIGN_HCENTER | ALIGN_VCENTER | IFontFace::LAYOUT_FORCE_FIRST_LINE);
        return true;
    }

    IRKUIThemer* CreateRKUIThemer()
    {
        return new RKUIThemer();
    }

    RKButtonThemer::RKButtonThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* label)
            : ThemerBase(themer, pos, size), label(label)
    {
        mouseOver = false;

        colour = RGBA_WHITE;
        colourMouseOver = RGBA_WHITE;

        fontid = 0;

        minSize = Int2(-1, -1);
    }

    RKButtonThemer::~RKButtonThemer()
    {
    }

    bool RKButtonThemer::AcquireResources()
    {
        font = themer->GetFont(fontid);

        if (!label.isEmpty())
        {
            txt.LayoutCentered(font, label, fontid);
            minSize.x = -1;
        }

        return true;
    }

    void RKButtonThemer::Draw()
    {
        auto painter = themer->GetPainter();

        painter->DrawRectangleShadow(pos, size, SHADOW_FULL);

        if (!mouseOver)
        {
            static const Byte4 colours[4] = {
                RGBA_GREY(104, 255),    RGBA_GREY(104, 255),
                RGBA_GREY(80, 255),     RGBA_GREY(80, 255)
            };

            painter->DrawFilledRectangle(pos, size, colours);
        }
        else
        {
            static const Byte4 colours[4] = {
                RGBA_GREY(128, 255),    RGBA_GREY(128, 255),
                RGBA_GREY(104, 255),    RGBA_GREY(104, 255)
            };

            painter->DrawFilledRectangle(pos, size, colours);
        }

        if (!txt.IsEmpty())
            txt.Draw(pos, size);
    }

    Int2 RKButtonThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = txt.Measure() + 2 * kButtonPadding;

        return minSize;
    }

    void RKButtonThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    void RKButtonThemer::SetLabel(const char* label)
    {
        this->label = label;

        if (font != nullptr)
            txt.LayoutCentered(font, label, fontid);

        minSize = Int2(-1, -1);
    }

    RKCheckBoxThemer::RKCheckBoxThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* label, bool checked)
            : ThemerBase(themer, pos, size), label(label), checked(checked)
    {
        mouseOver = false;

        colour = RGBA_GREY(32);
        colourMouseOver = RGBA_GREY(128);
        
        SetFont(0);
    }

    RKCheckBoxThemer::~RKCheckBoxThemer()
    {
    }

    bool RKCheckBoxThemer::AcquireResources()
    {
        txt.LayoutCentered(font, label, fontid);
        minSize.x = -1;
        return true;
    }

    void RKCheckBoxThemer::Draw()
    {
        auto painter = themer->GetPainter();

        static const Byte4 checkBoxColours[4] = {
            RGBA_GREY(255, 255),    RGBA_GREY(224, 255),
            RGBA_GREY(224, 255),    RGBA_GREY(192, 255)
        };

        painter->DrawRectangleShadow(Short2(pos.x, pos.y + size.y / 2 - kCheckBoxSizeXY / 2),
                Short2(kCheckBoxSizeXY, kCheckBoxSizeXY), SHADOW_FULL);
        painter->DrawFilledRectangle(Short2(pos.x, pos.y + size.y / 2 - kCheckBoxSizeXY / 2),
                Short2(kCheckBoxSizeXY, kCheckBoxSizeXY), checkBoxColours);

        if (checked)
            painter->DrawFilledRectangle(Short2(pos.x + (kCheckBoxSizeXY - kCheckBoxTickSizeXY) / 2, pos.y + size.y / 2 - kCheckBoxTickSizeXY / 2),
                    Short2(kCheckBoxTickSizeXY, kCheckBoxTickSizeXY), RGBA_BLACK_A(192));

        txt.Draw(Short2(pos.x + (kCheckBoxSizeXY + kCheckBoxLabelOffsetX), pos.y), Short2(size.x - (kCheckBoxSizeXY + kCheckBoxLabelOffsetX), size.y));
    }

    Int2 RKCheckBoxThemer::GetMinSize()
    {
        if (minSize.x == -1)
        {
            minSize = txt.Measure();
            minSize.x += kCheckBoxSizeXY + kCheckBoxLabelOffsetX;
            minSize.y = std::max<int>(minSize.y, kCheckBoxSizeXY);
        }

        return minSize;
    }

    void RKCheckBoxThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    RKMenuBarThemer::RKMenuBarThemer(RKUIThemer* themer)
            : ThemerBase(themer)
    {
        fontid = 1;
        itemsFontid = 0;

        font = themer->GetFont(fontid);
        itemsFont = themer->GetFont(itemsFontid);
        minSize = Int2(-1, -1);
    }

    RKMenuBarThemer::~RKMenuBarThemer()
    {
    }

    bool RKMenuBarThemer::AcquireResources()
    {
        return true;
    }
        
    RKMenuBarThemer::Item_t* RKMenuBarThemer::AllocItem(Item_t* parent, const char* label)
    {
        Item_t* item = new Item_t;
        item->label = label;
        item->labelSize = Short2(-1, -1);

        if (!Util::StrEmpty(label))
            item->labelTxt.LayoutCentered((parent == nullptr) ? font : itemsFont, label, (parent == nullptr) ? fontid : itemsFontid);

        item->flags = 0;
        return item;
    }

    void RKMenuBarThemer::Draw()
    {
        auto painter = themer->GetPainter();

        // menu bar
        static const Byte4 colours[4] = {
            rgbCSS(0x132638),       rgbCSS(0x132638),
            rgbCSS(0x0A131C),       rgbCSS(0x0A131C),
        };

        painter->DrawRectangleShadow(pos, size, SHADOW_BOTTOM);
        painter->DrawFilledRectangle(pos, size, colours);
    }

    void RKMenuBarThemer::DrawItem(Item_t* item, const Int3& pos, const Int2& size, const Int3& labelPos, const Int2& labelSize)
    {
        auto painter = themer->GetPainter();

        if ((item->flags & ITEM_EXPANDED))
        {
            // menu bar item highlight
            static const Byte4 colours[4] = {
                rgbCSS(0x264C6F),       rgbCSS(0x264C6F),
                rgbCSS(0x132638),       rgbCSS(0x132638),
            };

            painter->DrawFilledRectangle(Short2(pos), Short2(size), colours);
        }

        if ((item->flags & ITEM_SELECTED))
        {
            // menu item highlight
            static const Byte4 colours[4] = {
                rgbCSS(0x132638),       rgbCSS(0x132638),
                rgbCSS(0x264C6F),       rgbCSS(0x264C6F),
            };

            painter->DrawFilledRectangle(Short2(pos), Short2(size), colours);
        }
        
        if (!item->labelTxt.IsEmpty())
        {
            item->labelTxt.Draw(Short2(labelPos), Short2(labelSize));

            if ((item->flags & (ITEM_HAS_CHILDREN | ITEM_HAS_PARENT)) == (ITEM_HAS_CHILDREN | ITEM_HAS_PARENT))
            {
                const Short2 abc[3] = {
                    Short2(pos.x + size.x - 4, pos.y + size.y / 2 - kMenuItemArrowSize.y / 2),
                    Short2(pos.x + size.x - 4, pos.y + size.y / 2 + kMenuItemArrowSize.y / 2),
                    Short2(pos.x + size.x - 4 + kMenuItemArrowSize.x, pos.y + size.y / 2)
                };

                static const Byte4 colours[3] = {
                    RGBA_GREY(96, 255),
                        RGBA_GREY(128, 255),
                    RGBA_GREY(160, 255)
                };

                painter->DrawFilledTriangle(abc, colours);
            }
        }
        else
            // A slight bit of black magic here.
            painter->DrawFilledRectangle(Short2(labelPos.x, labelPos.y + 1),
                    Short2(size.x - 2 * (labelPos.x - pos.x), labelSize.y - 2), RGBA_WHITE_A(128));
    }

    void RKMenuBarThemer::DrawMenu(const Int3& pos, const Int2& size)
    {
        auto painter = themer->GetPainter();

        static const Byte4 colours[4] = {
            rgbCSS(0x0A131C),       rgbCSS(0x0A131C),
            rgbCSS(0x000000),       rgbCSS(0x000000),
        };

        painter->DrawRectangleShadow(Short2(pos), Short2(size), SHADOW_FULL);
        painter->DrawFilledRectangle(Short2(pos), Short2(size), colours);
    }

    Int2 RKMenuBarThemer::GetItemLabelSize(Item_t* item)
    {
        if (item->labelSize.x == -1)
        {
            if (!item->labelTxt.IsEmpty())
                item->labelSize = item->labelTxt.Measure();
            else
                item->labelSize = Int2(4, 3);
        }
        
        return Int2(item->labelSize);
    }

    RKPanelThemer::RKPanelThemer(RKUIThemer* themer, Int3 pos, Int2 size)
            : ThemerBase(themer, pos, size)
    {
        isWindow = false;

        titleMinSize.x = -1;
    }

    bool RKPanelThemer::AcquireResources()
    {
        if (isWindow)
        {
            auto font = themer->GetFont(0);
            titleTxt.LayoutCentered(font, title, 0);
            titleMinSize.x = -1;
        }

        return true;
    }

    void RKPanelThemer::Draw()
    {
        auto painter = themer->GetPainter();

        if (colour.a > 0)
        {
            painter->DrawRectangleShadow(pos, size, SHADOW_FULL);
            painter->DrawFilledRectangle(pos, size, colour);
        }

        if (isWindow && !title.isEmpty())
        {
            // window title bar
            static const Byte4 colours[4] = {
                rgbCSS(0x132638),       rgbCSS(0x132638),
                rgbCSS(0x0A131C),       rgbCSS(0x0A131C),
            };

            painter->DrawFilledRectangle(Short2(pos.x, pos.y - titleMinSize.y - 2 * kWindowTitlePadding.y),
                    Short2(size.x, titleMinSize.y + 2 * kWindowTitlePadding.y), colours);

            //painter->DrawRectangleShadow(Short2(pos.x, pos.y - titleMinSize.y - 2 * kWindowTitlePadding.y),
            //        Short2(size.x, titleMinSize.y + 2 * kWindowTitlePadding.y), SHADOW_BOTTOM);

            titleTxt.Draw(Short2(pos.x, pos.y - titleMinSize.y - kWindowTitlePadding.y), Short2(size.x, titleMinSize.y));
        }
    }

    Int2 RKPanelThemer::GetTitleMinSize()
    {
        if (titleMinSize.x == -1)
            titleMinSize = titleTxt.Measure() + 2 * kWindowTitlePadding;
        
        return titleMinSize;
    }

    void RKPanelThemer::SetColour(const Float4& colour)
    {
        this->colour = Byte4(colour * 255.0f);
    }

    void RKPanelThemer::SetTypeToWindow(int flags)
    {
        isWindow = true;
        colour = rgbCSS(0x0E1D2A);
    }

    void RKScrollBarThemer::DrawVScrollBar(Int2 pos, Int2 size, int y, int h)
    {
        auto painter = themer->GetPainter();

        painter->DrawFilledRectangle(Short2(pos), Short2(size), RGBA_BLACK_A(96));
        painter->DrawFilledRectangle(Short2(pos.x + 2, pos.y + 2 + y), Short2(size.x - 4, h), RGBA_GREY(192));
    }

    RKSliderThemer::RKSliderThemer(RKUIThemer* themer)
            : ThemerBase(themer)
    {
    }
    
    void RKSliderThemer::Draw(bool enabled, float value)
    {
        auto painter = themer->GetPainter();

        const int drawPosX = pos.x + kSliderThumbSizeXY / 2;
        const int drawnWidth = size.x - kSliderThumbSizeXY;

        {
            static const Byte4 colours[4] = {
                RGBA_GREY(16, 192),     RGBA_GREY(16, 192),
                RGBA_GREY(32, 192),     RGBA_GREY(32, 192)
            };
        
            painter->DrawFilledRectangle(Short2(drawPosX, pos.y + size.y / 2 - kSliderHeight / 2),
                    Short2(drawnWidth, kSliderHeight), colours);
        }

        {
            static const Byte4 colours[4] = {
                RGBA_GREY(192, 255),    RGBA_GREY(192, 255),
                RGBA_GREY(128, 255),    RGBA_GREY(128, 255)
            };

            const Short2 thumbPos(
                    drawPosX + drawnWidth * (value - min) * recip_range - kSliderThumbSizeXY / 2,
                    pos.y + size.y / 2 - kSliderThumbSizeXY / 2);

            painter->DrawRectangleShadow(thumbPos, Short2(kSliderThumbSizeXY, kSliderThumbSizeXY), SHADOW_FULL);

            if (enabled)
                painter->DrawFilledRectangle(thumbPos, Short2(kSliderThumbSizeXY, kSliderThumbSizeXY), colours);
            else
                painter->DrawFilledRectangle(thumbPos, Short2(kSliderThumbSizeXY, kSliderThumbSizeXY), RGBA_GREY(128, 192));
        }
    }

    Int2 RKSliderThemer::GetMinSize()
    {
        return Int2(6, 6);
    }

    void RKSliderThemer::SetRange(float min, float max, float tickStep)
    {
        this->min = min;
        this->max = max;
        this->tick = tickStep;

        recip_range = 1.0f / (max - min);
    }

    RKStaticTextThemer::RKStaticTextThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* label)
            : ThemerBase(themer, pos, size), label(label)
    {
        colour = RGBA_WHITE;
        
        fontid = 0;

        minSize = Int2(-1, -1);
    }
    
    RKStaticTextThemer::~RKStaticTextThemer()
    {
    }

    bool RKStaticTextThemer::AcquireResources()
    {
        font = themer->GetFont(fontid);

        if (!label.isEmpty())
        {
            txt.LayoutCentered(font, label, fontid);
            minSize.x = -1;
        }

        return true;
    }
    
    void RKStaticTextThemer::Draw()
    {
        if (!txt.IsEmpty())
            txt.Draw(pos, size, colour);
    }

    Int2 RKStaticTextThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = txt.Measure() + Int2(6, 6);
        
        return minSize;
    }

    void RKStaticTextThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }
    
    void RKStaticTextThemer::SetLabel(const char* label)
    {
        this->label = label;

        if (font != nullptr)
            txt.LayoutCentered(font, label, fontid);

        minSize = Int2(-1, -1);
    }

    RKStaticImageThemer::RKStaticImageThemer(RKUIThemer* themer, Int3 pos, Int2 size, const char* path)
            : ThemerBase(themer, pos, size), path(path)
    {
        g = nullptr;
    }
    
    RKStaticImageThemer::~RKStaticImageThemer()
    {
        DropResources();
    }

    bool RKStaticImageThemer::AcquireResources()
    {
        char params[256];

        Params::BuildIntoBuffer(params, sizeof(params), 2,
            "type", "image",
            "src", path.c_str());

#if ZOMBIE_API_VERSION < 201701
        g = themer->GetResourceManager()->GetResource<IGraphics>(params, RESOURCE_REQUIRED, 0);

        if (g == nullptr)
        {
            auto sys = themer->GetSys();
            sys->PrintError(GetErrorBuffer(), kLogWarning);
        }
#else
        zombie_assert(!"Not implemented");
#endif

        return true;
    }
    
    void RKStaticImageThemer::DropResources()
    {
        g.reset();
    }

    void RKStaticImageThemer::Draw()
    {
        if (g != nullptr)
            g->DrawStretched(nullptr, Float3(pos, 0.0f), Float3(size, 0.0f));
    }

    Int2 RKStaticImageThemer::GetMinSize()
    {
        if (g != nullptr)
            return Int2(g->GetSize());
        else
            return Int2();
    }

    RKTextBoxThemer::RKTextBoxThemer(RKUIThemer* themer, Int3 pos, Int2 size)
            : ThemerBase(themer, pos, size)
    {
        SetFont(0);

        active = false;
    }
    
    RKTextBoxThemer::~RKTextBoxThemer()
    {
    }

    bool RKTextBoxThemer::AcquireResources()
    {
        txt.LayoutCentered(font, text, fontid);
        minSize.x = -1;

        return true;
    }
    
    void RKTextBoxThemer::Draw()
    {
        auto painter = themer->GetPainter();

        painter->DrawFilledRectangle(pos, size, RGBA_GREY(64));

        if (!txt.IsEmpty())
            txt.Draw(pos, size);

        if (active)
            painter->DrawFilledRectangle(cursorPos, Short2(1, font->GetLineHeight()), RGBA_WHITE);
    }

    Int2 RKTextBoxThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = txt.Measure() + Int2(6, 6);
        
        return minSize;
    }

    size_t RKTextBoxThemer::InsertCharacter(size_t pos, gameui::UnicodeChar c)
    {
        if (c == Unicode::backspaceChar)
        {
            if (pos > 0)
            {
                SetText(text.leftPart(pos - 1) + text.dropLeftPart(pos));
                txt.GetCharPos(this->pos, this->size, pos - 1, &cursorPos);
                return pos - 1;
            }
        }
        else
        {
            SetText(text.leftPart(pos) + c + text.dropLeftPart(pos));
            txt.GetCharPos(this->pos, this->size, pos + 1, &cursorPos);
            return pos + 1;
        }

        return pos;
    }

    void RKTextBoxThemer::SetCursorPos(Int3 pos, size_t* followingCharIndex_out)
    {
        *followingCharIndex_out = txt.GetCharAt(this->pos, this->size, Short2(pos), &cursorPos);
    }

    void RKTextBoxThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }
    
    void RKTextBoxThemer::SetText(const char* text)
    {
        this->text = text;

        if (font != nullptr)
            txt.LayoutCentered(font, text, fontid);

        minSize = Int2(-1, -1);
    }

    RKTreeBoxThemer::RKTreeBoxThemer(RKUIThemer* themer, Int3 pos, Int2 size)
            : ThemerBase(themer, pos, size)
    {
        fontid = 0;

        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    RKTreeBoxThemer::~RKTreeBoxThemer()
    {
    }

    bool RKTreeBoxThemer::AcquireResources()
    {
        return true;
    }
        
    RKTreeBoxThemer::Item_t* RKTreeBoxThemer::AllocItem(Item_t* parent, const char* label)
    {
        Item_t* item = new Item_t;
        item->label = label;
        item->labelSize = Short2(-1, -1);
        item->labelTxt.LayoutCentered(font, label, fontid);

        return item;
    }

    void RKTreeBoxThemer::DrawItem(Item_t* item, const Int3& arrowPos, const Int3& labelPos)
    {
        auto painter = themer->GetPainter();

        if (item->hasChildren)
        {
            if (!item->expanded)
                painter->DrawFilledRectangle(Short2(arrowPos), kTreeBoxArrowSize, RGBA_WHITE);
            else
                painter->DrawFilledRectangle(Short2(arrowPos), kTreeBoxArrowSize, RGBA_GREY(128));
        }
        else
            painter->DrawFilledRectangle(Short2(arrowPos) + Short2(3, 3), kTreeBoxArrowSize - Short2(6, 6), RGBA_GREY(128));

        if (item->selected)
        {
            painter->DrawFilledRectangle(Short2(labelPos), item->labelSize, RGBA_WHITE_A(32));
        }

        item->labelTxt.Draw(Short2(labelPos), item->labelSize);
    }

    Int2 RKTreeBoxThemer::GetItemLabelSize(Item_t* item)
    {
        if (item->labelSize.x == -1)
            item->labelSize = item->labelTxt.Measure();
        
        return Int2(item->labelSize);
    }

    void RKTreeBoxThemer::SetItemLabel(Item_t* item, const char* label)
    {
        item->labelTxt.LayoutCentered(font, label, fontid);
    }

#if ZOMBIE_API_VERSION < 201701
    RKUIPainter::RKUIPainter(IRenderingManager* rm, IResourceManager* res)
            : rm(rm), res(res)
    {
    }
#else
    RKUIPainter::RKUIPainter(IRenderingManager* rm, IResourceManager2* res2)
            : rm(rm), res2(res2)
    {
    }
#endif

    bool RKUIPainter::AcquireResources()
    {
#if ZOMBIE_API_VERSION < 201701
        zombie_assert(res != nullptr);

        shared_ptr<IShader> program = res->GetResource<IShader>("path=RenderingKit/ui", RESOURCE_REQUIRED, 0);

        if (program == nullptr)
            return false;

        materialReference = rm->CreateMaterial("ui_material", program);
        this->material = materialReference.get();
#else
        zombie_assert(res2 != nullptr);

        material = res2->GetResource<IMaterial>("shader=path=RenderingKit/ui", IResourceManager2::kResourceRequired);
        
        if (material == nullptr)
            return false;
#endif

        fontAtlas = rm->CreateTextureAtlas2D("RKUIPainter::fontAtlas", kFontAtlasInitSize);
        material->SetTexture("tex", fontAtlas->GetTexture().get());

        vertexFormat = rm->CompileVertexFormat(material->GetShader(), 16, uiVertexAttribs, false);

        return true;
    }

    void RKUIPainter::DropResources()
    {
        materialReference.reset();
        vertexFormat.reset();
        fontAtlas.reset();
    }

#define vert(x_, y_, u_, v_, texsel_, rgba_) do {\
        p_vertices->pos[0] = (short)(x_);\
        p_vertices->pos[1] = (short)(y_);\
        p_vertices->uv[0] = (short)(u_);\
        p_vertices->uv[1] = (short)(v_);\
        p_vertices->texsel = texsel_;\
        memcpy(p_vertices->rgba, &rgba_[0], 4);\
        p_vertices++;\
    } while (false)

    void RKUIPainter::DrawFilledRectangle(const Short2 pos, const Short2 size, const Byte4 colour)
    {
        UIVertex_t* p_vertices = reinterpret_cast<UIVertex_t*>(rm->VertexCacheAlloc(vertexFormat.get(), material, RK_TRIANGLES, 6));

        vert(pos.x,             pos.y,          0, 0, -1, colour);
        vert(pos.x,             pos.y + size.y, 0, 0, -1, colour);
        vert(pos.x + size.x,    pos.y,          0, 0, -1, colour);
        vert(pos.x + size.x,    pos.y,          0, 0, -1, colour);
        vert(pos.x,             pos.y + size.y, 0, 0, -1, colour);
        vert(pos.x + size.x,    pos.y + size.y, 0, 0, -1, colour);
    }

    void RKUIPainter::DrawFilledRectangle(const Short2 pos, const Short2 size, const Byte4 colours[4])
    {
        UIVertex_t* p_vertices = reinterpret_cast<UIVertex_t*>(rm->VertexCacheAlloc(vertexFormat.get(), material, RK_TRIANGLES, 6));

        vert(pos.x,             pos.y,          0, 0, -1, colours[0]);
        vert(pos.x,             pos.y + size.y, 0, 0, -1, colours[2]);
        vert(pos.x + size.x,    pos.y,          0, 0, -1, colours[1]);
        vert(pos.x + size.x,    pos.y,          0, 0, -1, colours[1]);
        vert(pos.x,             pos.y + size.y, 0, 0, -1, colours[2]);
        vert(pos.x + size.x,    pos.y + size.y, 0, 0, -1, colours[3]);
    }

    void RKUIPainter::DrawFilledTriangle(const Short2 abc[3], const Byte4 colours[3])
    {
        UIVertex_t* p_vertices = reinterpret_cast<UIVertex_t*>(rm->VertexCacheAlloc(vertexFormat.get(), material, RK_TRIANGLES, 3));

        vert(abc[0].x,  abc[0].y,   0, 0, -1, colours[0]);
        vert(abc[1].x,  abc[1].y,   0, 0, -1, colours[2]);
        vert(abc[2].x,  abc[2].y,   0, 0, -1, colours[1]);
    }

    void RKUIPainter::DrawRectangleShadow(const Short2 pos, const Short2 size, int flags)
    {
        const static uint8_t shadowColour = 0;
        const static uint8_t shadowAlpha = 48;

        if ((flags & SHADOW_FULL) == SHADOW_FULL)
        {
            // Top Right
            {
                static const Byte4 colours[4] = {
                    RGBA_GREY(shadowColour, 0),             RGBA_GREY(shadowColour, 0),
                    RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, 0)
                };

                DrawFilledRectangle(pos + Short2(size.x, 0), Short2(4, 4), colours);
            }

            // Right
            {
                static const Byte4 colours[4] = {
                    RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, 0),
                    RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, 0)
                };

                DrawFilledRectangle(pos + Short2(size.x, 4), Short2(4, size.y - 4), colours);
            }

            // Bottom Right
            {
                static const Byte4 colours[4] = {
                    RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, 0),
                    RGBA_GREY(shadowColour, 0),             RGBA_GREY(shadowColour, 0)
                };

                DrawFilledRectangle(pos + Short2(size.x, size.y), Short2(4, 4), colours);
            }

            // Bottom
            {
                static const Byte4 colours[4] = {
                    RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, shadowAlpha),
                    RGBA_GREY(shadowColour, 0),             RGBA_GREY(shadowColour, 0)
                };

                DrawFilledRectangle(pos + Short2(4, size.y), Short2(size.x - 4, 4), colours);
            }

            // Bottom Left
            {
                static const Byte4 colours[4] = {
                    RGBA_GREY(shadowColour, 0),         RGBA_GREY(shadowColour, shadowAlpha),
                    RGBA_GREY(shadowColour, 0),         RGBA_GREY(shadowColour, 0)
                };

                DrawFilledRectangle(pos + Short2(0, size.y), Short2(4, 4), colours);
            }
        }
        else if (flags & SHADOW_BOTTOM)
        {
            static const Byte4 colours[4] = {
                RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, shadowAlpha),
                RGBA_GREY(shadowColour, 0),             RGBA_GREY(shadowColour, 0)
            };

            DrawFilledRectangle(pos + Short2(0, size.y), Short2(size.x, 4), colours);
        }
        else if (flags & SHADOW_RIGHT)
        {
            static const Byte4 colours[4] = {
                RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, 0),
                RGBA_GREY(shadowColour, shadowAlpha),   RGBA_GREY(shadowColour, 0)
            };

            DrawFilledRectangle(pos + Short2(size.x, 0), Short2(4, size.y), colours);
        }
    }

    int RKUIPainter::OnFontQuad(const Float3& pos, const Float2& size, const Float2 uv[2], Byte4 colour)
    {
        static const int TexIndex = 0;

        UIVertex_t* p_vertices = reinterpret_cast<UIVertex_t*>(rm->VertexCacheAlloc(vertexFormat.get(), material, RK_TRIANGLES, 6));

        vert(pos.x,             pos.y,          uv[0].x * 32768, uv[0].y * 32768, TexIndex, colour);
        vert(pos.x,             pos.y + size.y, uv[0].x * 32768, uv[1].y * 32768, TexIndex, colour);
        vert(pos.x + size.x,    pos.y,          uv[1].x * 32768, uv[0].y * 32768, TexIndex, colour);
        vert(pos.x + size.x,    pos.y,          uv[1].x * 32768, uv[0].y * 32768, TexIndex, colour);
        vert(pos.x,             pos.y + size.y, uv[0].x * 32768, uv[1].y * 32768, TexIndex, colour);
        vert(pos.x + size.x,    pos.y + size.y, uv[1].x * 32768, uv[1].y * 32768, TexIndex, colour);

        return 0;
    }

    RKUIThemer::RKUIThemer()
    {
        eb = nullptr;
    }

    RKUIThemer::~RKUIThemer()
    {
        painter.reset();

        DropResources();
    }

#if ZOMBIE_API_VERSION >= 201901
    bool RKUIThemer::Init(zfw::IEngine* sys, IRenderingManager* rm, IResourceManager2* res)
    {
        SetEssentials(sys->GetEssentials());

        this->eb = GetErrorBuffer();
        this->sys = sys;
        this->rm = rm;

        painter.reset(new RKUIPainter(rm, res));
        return true;
    }
#elif ZOMBIE_API_VERSION >= 201701
    bool RKUIThemer::Init(zfw::IEngine* sys, IRenderingKit* rk, IResourceManager2* res)
    {
        SetEssentials(sys->GetEssentials());

        this->eb = GetErrorBuffer();
        this->sys = sys;
        this->rm = rk->GetRenderingManager();

        painter.reset(new RKUIPainter(rm, res));
        return true;
    }
#else
    void RKUIThemer::Init(zfw::IEngine* sys, IRenderingKit* rk, IResourceManager* res)
    {
        SetEssentials(sys->GetEssentials());

        this->eb = GetErrorBuffer();
        this->sys = sys;
        this->rm = rk->GetRenderingManager();
        this->res = res;

        painter.reset(new RKUIPainter(rm, res));
    }
#endif

    bool RKUIThemer::AcquireFontResources(FontEntry& entry)
    {
        entry.font = rm->CreateFontFace(entry.path);
        entry.font->SetTextureAtlas(painter->GetFontAtlas());
            
        bool opened;

        if (entry.size > 0)
            opened = entry.font->Open(entry.path, entry.size, entry.flags);
        else
            opened = entry.font->Open(entry.path, 1, entry.flags);

        if (!opened)
            return false;

        entry.font->SetQuadSink(painter.get());

        return true;
    }

    bool RKUIThemer::AcquireResources()
    {
        if (!painter->AcquireResources())
            return false;

        for (auto& entry : fonts)
        {
            ErrorCheck(AcquireFontResources(entry));
        }

        /*iterate2 (i, fonts)
        {
            FontEntry& entry = i;
            
            //if (entry.flags != 0)
            //    Sys::RaiseException(EX_INVALID_ARGUMENT, "UIResManager::ReloadMedia", "Unsupported flag %d for font `%s` @ %d", entry.flags, entry.path.c_str(), entry.size);
            
            
        }*/

        return true;
    }
    
    void RKUIThemer::DropResources()
    {
    }

    IButtonThemer* RKUIThemer::CreateButtonThemer(Int3 pos, Int2 size, const char* label)
    {
        return new RKButtonThemer(this, pos, size, label);
    }
    
    ICheckBoxThemer* RKUIThemer::CreateCheckBoxThemer(Int3 pos, Int2 size, const char* label, bool checked)
    {
        return new RKCheckBoxThemer(this, pos, size, label, checked);
    }

    IMenuBarThemer* RKUIThemer::CreateMenuBarThemer()
    {
        return new RKMenuBarThemer(this);
    }

    IPanelThemer* RKUIThemer::CreatePanelThemer(Int3 pos, Int2 size)
    {
        return new RKPanelThemer(this, pos, size);
    }

    ISliderThemer* RKUIThemer::CreateSliderThemer()
    {
        return new RKSliderThemer(this);
    }

    IStaticImageThemer* RKUIThemer::CreateStaticImageThemer(Int3 pos, Int2 size, const char* path)
    {
        return new RKStaticImageThemer(this, pos, size, path);
    }

    IStaticTextThemer* RKUIThemer::CreateStaticTextThemer(Int3 pos, Int2 size, const char* label)
    {
        return new RKStaticTextThemer(this, pos, size, label);
    }

    ITextBoxThemer* RKUIThemer::CreateTextBoxThemer(Int3 pos, Int2 size)
    {
        return new RKTextBoxThemer(this, pos, size);
    }

    ITreeBoxThemer* RKUIThemer::CreateTreeBoxThemer(Int3 pos, Int2 size)
    {
        return new RKTreeBoxThemer(this, pos, size);
    }
    
    IWindowThemer* RKUIThemer::CreateWindowThemer(IPanelThemer* thm, int flags)
    {
        auto pthm = static_cast<RKPanelThemer*>(thm);
        pthm->SetTypeToWindow(flags);
        return pthm;
    }

    IFontFace* RKUIThemer::GetFont(intptr_t fontIndex)
    {
        if (fontIndex >= 0 && (uintptr_t) fontIndex < fonts.getLength())
            return fonts[fontIndex].font.get();
		else {
			zombie_assert(!fonts.isEmpty());
			return fonts[0].font.get();
		}
    }

    intptr_t RKUIThemer::GetFontId(const char* font)
    {
        FontEntry entry;

        entry.font = nullptr;

        if (UIThemerUtil::IsFontParams(font, entry.path, entry.size, entry.flags))
        {
            iterate2 (i, fonts)
            {
                FontEntry& current = i;
            
                if (current.path == entry.path && current.size == entry.size && current.flags == entry.flags)
                    return i.getIndex();
            }

            ZFW_ASSERT(AcquireFontResources(entry))

            return fonts.add(entry);
        }
        else
        {
            iterate2 (i, fonts)
            {
                FontEntry& current = i;
            
                if (current.name == font)
                    return i.getIndex();
            }

            return -1;
        }
    }

    intptr_t RKUIThemer::PreregisterFont(const char* name, const char* params)
    {
        FontEntry entry;

        ZFW_ASSERT(UIThemerUtil::IsFontParams(params, entry.path, entry.size, entry.flags))

        entry.name = name;
        entry.font = nullptr;

        return fonts.add(entry);
    }
}
