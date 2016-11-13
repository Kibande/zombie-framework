
#include "ntile.hpp"

#include <framework/graphics.hpp>
#include <framework/resourcemanager2.hpp>

namespace ntile
{
    // Protip: it might be faster/more effective to put 'implements(gameui::*Themer)' first
    //  (interface methods are more important than ThemerBase which is inline anyway)

    class ThemerBase
    {
        protected:
            NUIThemer* themer;
            Int3 pos;
            Int2 size;

        protected:
            ThemerBase(NUIThemer* themer)
                    : themer(themer)
            {
            }

            ThemerBase(NUIThemer* themer, Int3 pos, Int2 size)
                    : themer(themer), pos(pos), size(size)
            {
            }

            void SetArea(Int3 pos, Int2 size)
            {
                this->pos = pos;
                this->size = size;
            }
    };

    class NButtonThemer : public gameui::IButtonThemer, public ThemerBase
    {
        String label;

        Byte4 colour, colourMouseOver;
        intptr_t fontid;

        IFont* font;
        Int2 minSize;
        bool mouseOver;

        public:
            NButtonThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* label);
            ~NButtonThemer() {}

            virtual bool AcquireResources() override { return true; }

            virtual Int2 GetMinSize() override;
            virtual const char* GetLabel() override { return label.c_str(); }
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetLabel(const char* label) override { this->label = label; }
            virtual void SetMouseOver(bool isOver) override { mouseOver = isOver; }

            virtual void Draw() override;
    };

    class NCheckBoxThemer : public gameui::ICheckBoxThemer
    {
        NUIThemer* themer;
        Int3 pos;
        Int2 size;
        String label;
        bool checked;

        Byte4 colour, colourMouseOver;
        intptr_t fontid;

        IFont* font;
        Int2 minSize;
        bool mouseOver;

        public:
            NCheckBoxThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* label, bool checked);
            ~NCheckBoxThemer() {}

            virtual bool AcquireResources() override { return true; }

            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override;
            virtual void SetChecked(bool isChecked) override { checked = isChecked; }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetMouseOver(bool isOver) override { mouseOver = isOver; }

            virtual void Draw() override;
    };

    class NComboBoxThemer : public gameui::IComboBoxThemer
    {
        NUIThemer* themer;
        Int3 pos;
        Int2 size, contentsSize, buttonSize;
        int selection, mouseOver;
        bool expanded, popupSelectionEnabled;

        struct
        {
            Int3 pos;
            Int2 size;
        }
        popup, popupSelection;

        Byte4 bgColour, popupBgColour, popupHightlightColour;

        public:
            NComboBoxThemer(NUIThemer* themer, Int3 pos, Int2 size);
            ~NComboBoxThemer() {}

            virtual bool AcquireResources() override { return true; }

            virtual Int2 GetMinSize(Int2 contentsMinSize) override;
            virtual void SetArea(Int3 pos, Int2 size) override;
            virtual void SetContentsSize(Int2 size) override { contentsSize = size; }
            virtual void SetExpanded(bool isExpanded) override { expanded = isExpanded; }
            virtual void SetPopupArea(Int3 pos, Int2 size) override;
            virtual void SetPopupSelectionArea(bool enabled, Int3 pos, Int2 size) override;

            virtual void Draw() override;
            virtual void DrawPopup() override;
    };

    class NGraphicsThemer : public gameui::IGraphicsThemer, public ThemerBase
    {
        shared_ptr<IGraphics> g;

        public:
            NGraphicsThemer(NUIThemer* themer, shared_ptr<IGraphics> g);

            virtual bool AcquireResources() override;

            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }

            virtual void Draw() override;
    };

    class NPanelThemer : public gameui::IPanelThemer, public gameui::IWindowThemer
    {
        NUIThemer* themer;
        Int3 pos;
        Int2 size;
        Byte4 colour;

        public:
            NPanelThemer(NUIThemer* themer, Int3 pos, Int2 size);

            virtual bool AcquireResources() override { return true; }

            virtual void SetArea(Int3 pos, Int2 size) override;
            virtual void SetColour(const Float4& colour) override;

            virtual void Draw() override;

            // gameui::IWindowThemer
            virtual const char* GetTitle() override { return ""; }
            virtual Int2 GetTitleMinSize() override { return Int2(); }
            virtual void SetTitle(const char* title) override {}

            virtual void SetTypeToWindow(int flags);
    };

    class NStaticImageThemer : public gameui::IStaticImageThemer
    {
        NUIThemer* themer;
        Int3 pos;
        Int2 size;
        String path;

        Byte4 colour;

        ITexture* texture;

        public:
            NStaticImageThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* path);
            ~NStaticImageThemer() { DropResources(); }

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override;

            virtual void Draw() override;
    };

    class NStaticTextThemer : public gameui::IStaticTextThemer, public ThemerBase
    {
        String label;

        Byte4 colour;
        intptr_t fontid;

        IFont* font;
        Int2 minSize;

        public:
            NStaticTextThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* label);

            virtual bool AcquireResources() override { return true; }

            virtual const char* GetLabel() override { return label; }
            virtual Int2 GetMinSize() override;
            virtual void SetArea(Int3 pos, Int2 size) override { ThemerBase::SetArea(pos, size); }
            virtual void SetColour(const Float4& colour) override { this->colour = Byte4(colour * 255.0f); }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetLabel(const char* label) override { this->label = label; minSize = Int2(-1, -1); }

            virtual void Draw() override;
    };

    class NTextBoxThemer : public gameui::ITextBoxThemer
    {
        NUIThemer* themer;
        Int3 pos;
        Int2 size;
        String text;

        Byte4 colour, colourMouseOver, colourActive;
        intptr_t fontid;

        IFont* font;
        Int2 minSize;
        bool mouseOver, active;

        public:
            NTextBoxThemer(NUIThemer* themer, Int3 pos, Int2 size);
            ~NTextBoxThemer() {}

            virtual bool AcquireResources() override { return true; }

            virtual Int2 GetMinSize() override;
            virtual const char* GetText() override { return text; }
            virtual size_t InsertCharacter(size_t pos, gameui::UnicodeChar c) override { return pos; }
            virtual void SetActive(bool active) override { this->active = active; }
            virtual void SetArea(Int3 pos, Int2 size) override;
            virtual void SetCursorPos(Int3 pos, size_t* followingCharIndex_out) override { *followingCharIndex_out = 0; }
            virtual void SetFont(intptr_t fontid) override;
            virtual void SetMouseOver(bool isOver) override { mouseOver = isOver; }
            virtual void SetText(const char* text) override;

            virtual void Draw() override;
    };

    NButtonThemer::NButtonThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* label)
            : ThemerBase(themer, pos, size), label(label)
    {
        mouseOver = false;

        colour = RGBA_GREY(32);
        colourMouseOver = RGBA_GREY(128);

        SetFont(0);
    }

    void NButtonThemer::Draw()
    {
        font->DrawText(label, pos + Int3(size / 2, 0), mouseOver ? colourMouseOver : colour, ALIGN_HCENTER | ALIGN_VCENTER);
    }

    Int2 NButtonThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = font->MeasureText(label, 0);

        return minSize;
    }

    void NButtonThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    NCheckBoxThemer::NCheckBoxThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* label, bool checked)
            : themer(themer), pos(pos), size(size), label(label), checked(checked)
    {
        mouseOver = false;

        colour = RGBA_GREY(32);
        colourMouseOver = RGBA_GREY(128);

        SetFont(0);
    }

    void NCheckBoxThemer::Draw()
    {
        font->DrawText((checked ? "\x02 " : "\x01 ") +
            label, pos + Int3(size / 2, 0), mouseOver ? colourMouseOver : colour, ALIGN_HCENTER | ALIGN_VCENTER);
    }

    Int2 NCheckBoxThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = font->MeasureText("\x01 " + label, 0);

        return minSize;
    }

    void NCheckBoxThemer::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;
    }

    void NCheckBoxThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    NComboBoxThemer::NComboBoxThemer(NUIThemer* themer, Int3 pos, Int2 size)
            : themer(themer), pos(pos), size(size)
    {
        selection = -1;
        mouseOver = -1;
        expanded = false;
        popupSelectionEnabled = false;

        bgColour = RGBA_GREY(160);
        popupBgColour = RGBA_GREY(192);
        popupHightlightColour = RGBA_GREY(224);
    }

    void NComboBoxThemer::Draw()
    {
        ir->SetColour(bgColour);
        ir->DrawRect(pos, size);

        themer->GetFont(0)->DrawText("\x03", pos + Int3(size.x - buttonSize.x / 2, size.y / 2, 0),
            RGBA_GREY(expanded ? 64 : 0), ALIGN_HCENTER | ALIGN_VCENTER);
    }

    void NComboBoxThemer::DrawPopup()
    {
        ir->SetColour(popupBgColour);
        ir->DrawRect(popup.pos, popup.size);

        if (popupSelectionEnabled)
        {
            ir->SetColour(popupHightlightColour);
            ir->DrawRect(popupSelection.pos, popupSelection.size);
        }
    }

    Int2 NComboBoxThemer::GetMinSize(Int2 contentsMinSize)
    {
        buttonSize = themer->GetFont(0)->MeasureText("\x03", 0);

        return Int2(contentsMinSize.x + buttonSize.x, std::max<int>(contentsMinSize.y, buttonSize.y));
    }

    void NComboBoxThemer::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;
    }

    void NComboBoxThemer::SetPopupArea(Int3 pos, Int2 size)
    {
        popup.pos = pos;
        popup.size = size;
    }

    void NComboBoxThemer::SetPopupSelectionArea(bool enabled, Int3 pos, Int2 size)
    {
        popupSelectionEnabled = enabled;
        popupSelection.pos = pos;
        popupSelection.size = size;
    }

    NGraphicsThemer::NGraphicsThemer(NUIThemer* themer, shared_ptr<IGraphics> g)
            : ThemerBase(themer), g(move(g))
    {
    }

    bool NGraphicsThemer::AcquireResources()
    {
        return true;
    }

    void NGraphicsThemer::Draw()
    {
        g->Draw(nullptr, Float3(pos));
    }

    NPanelThemer::NPanelThemer(NUIThemer* themer, Int3 pos, Int2 size)
        : themer(themer), pos(pos), size(size)
    {
    }

    void NPanelThemer::Draw()
    {
        if (colour.a > 0)
        {
            ir->SetColour(colour);
            ir->DrawRect(pos, size);
        }
    }

    void NPanelThemer::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;
    }

    void NPanelThemer::SetColour(const Float4& colour)
    {
        this->colour = Byte4(colour * 255.0f);
    }

    void NPanelThemer::SetTypeToWindow(int flags)
    {
        SetColour(COLOUR_GREY(0.3f));
    }

    NStaticImageThemer::NStaticImageThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* path) : themer(themer), pos(pos), size(size), path(path)
    {
        texture = nullptr;
    }

    bool NStaticImageThemer::AcquireResources()
    {
        g_res->Resource(&texture, sprintf_255("path=%s", path.c_str()));

        return true;
    }

    void NStaticImageThemer::DropResources()
    {
    }

    void NStaticImageThemer::Draw()
    {
        ir->DrawTexture(texture, Int2(pos), size, Int2(0, 0), texture->GetSize());
    }

    Int2 NStaticImageThemer::GetMinSize()
    {
        return texture->GetSize();
    }

    void NStaticImageThemer::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;
    }

    NStaticTextThemer::NStaticTextThemer(NUIThemer* themer, Int3 pos, Int2 size, const char* label)
            : ThemerBase(themer, pos, size), label(label)
    {
        colour = RGBA_GREY(0);

        SetFont(0);
    }

    void NStaticTextThemer::Draw()
    {
        font->DrawText(label, pos + Int3(size / 2, 0), colour, ALIGN_HCENTER | ALIGN_VCENTER);
    }

    Int2 NStaticTextThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = font->MeasureText(label, 0);

        return minSize;
    }

    void NStaticTextThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    NTextBoxThemer::NTextBoxThemer(NUIThemer* themer, Int3 pos, Int2 size)
            : themer(themer), pos(pos), size(size)
    {
        mouseOver = false;
        active = false;

        colour = RGBA_GREY(32);
        colourMouseOver = RGBA_GREY(64);
        colourActive = RGBA_GREY(128);

        SetFont(0);
    }

    void NTextBoxThemer::Draw()
    {
        // TODO: hardcoded padding
        font->DrawText(text + (active ? "_" : ""), pos + Int3(4, size.y / 2, 0), active ? colourActive : mouseOver ? colourMouseOver : colour, ALIGN_LEFT | ALIGN_VCENTER);
    }

    Int2 NTextBoxThemer::GetMinSize()
    {
        if (minSize.x == -1)
            minSize = font->MeasureText(text + "_", 0);

        return minSize;
    }

    void NTextBoxThemer::SetArea(Int3 pos, Int2 size)
    {
        this->pos = pos;
        this->size = size;
    }

    void NTextBoxThemer::SetFont(intptr_t fontid)
    {
        this->fontid = fontid;
        font = themer->GetFont(fontid);
        minSize = Int2(-1, -1);
    }

    void NTextBoxThemer::SetText(const char* text)
    {
        this->text = text;
        minSize = Int2(-1, -1);
    }

    bool NUIThemer::AcquireResources()
    {
        /*iterate2 (i, fonts)
        {
            FontEntry& entry = i;

            //if (entry.flags != 0)
            //    Sys::RaiseException(EX_INVALID_ARGUMENT, "UIResManager::ReloadMedia", "Unsupported flag %d for font `%s` @ %d", entry.flags, entry.path.c_str(), entry.size);

            if (entry.size > 0)
                entry.font.reset(ir->LoadFont(entry.path, entry.size, 0));
            else
                entry.font.reset(ir->LoadFont(entry.path, 1, 0));

            if (entry.font == nullptr)
                return false;
        }*/

        return true;
    }

    void NUIThemer::DropResources()
    {
    }

    gameui::IButtonThemer* NUIThemer::CreateButtonThemer(Int3 pos, Int2 size, const char* label)
    {
        return new NButtonThemer(this, pos, size, label);
    }

    gameui::ICheckBoxThemer* NUIThemer::CreateCheckBoxThemer(Int3 pos, Int2 size, const char* label, bool checked)
    {
        return new NCheckBoxThemer(this, pos, size, label, checked);
    }

    gameui::IComboBoxThemer* NUIThemer::CreateComboBoxThemer(Int3 pos, Int2 size)
    {
        return new NComboBoxThemer(this, pos, size);
    }

    gameui::IGraphicsThemer* NUIThemer::CreateGraphicsThemer(shared_ptr<IGraphics> g)
    {
        return new NGraphicsThemer(this, move(g));
    }

    gameui::IPanelThemer* NUIThemer::CreatePanelThemer(Int3 pos, Int2 size)
    {
        return new NPanelThemer(this, pos, size);
    }

    gameui::IStaticImageThemer* NUIThemer::CreateStaticImageThemer(Int3 pos, Int2 size, const char* path)
    {
        return new NStaticImageThemer(this, pos, size, path);
    }

    gameui::IStaticTextThemer* NUIThemer::CreateStaticTextThemer(Int3 pos, Int2 size, const char* label)
    {
        return new NStaticTextThemer(this, pos, size, label);
    }

    gameui::ITextBoxThemer* NUIThemer::CreateTextBoxThemer(Int3 pos, Int2 size)
    {
        return new NTextBoxThemer(this, pos, size);
    }

    gameui::IWindowThemer* NUIThemer::CreateWindowThemer(gameui::IPanelThemer* thm, int flags)
    {
        auto pthm = static_cast<NPanelThemer*>(thm);
        pthm->SetTypeToWindow(flags);
        return pthm;
    }

    intptr_t NUIThemer::GetFontId(const char* font)
    {
        FontEntry entry;

        entry.font = nullptr;

        if (gameui::UIThemerUtil::IsFontParams(font, entry.path, entry.size, entry.flags))
        {
            iterate2 (i, fonts)
            {
                FontEntry& current = i;
            
                if (current.path == entry.path && current.size == entry.size && current.flags == entry.flags)
                    return i.getIndex();
            }

            g_res->Resource(&entry.font, font);
            return fonts.add(move(entry));
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

    intptr_t NUIThemer::PreregisterFont(const char* name, const char* params)
    {
        FontEntry entry;

        //zombie_assert(gameui::UIThemerUtil::TryParseFontParams(params, entry.path, entry.size, entry.flags));

        entry.name = name;
        g_res->Resource(&entry.font, params);

        //ZFW_ASSERT(AcquireFontResources(entry))

        return fonts.add(move(entry));
    }
    /*
    intptr_t NUIThemer::GetFontId(const char* path, int size, int fontflags) 
    {
        FontEntry entry = { String(), size, fontflags, nullptr };

        const String fontPrefix;

        if (!Util::StrEmpty(path))
            entry.path = fontPrefix + path;
        else
            entry.path = fonts[0].path;

        if (size <= 0)
            entry.size = fonts[0].size;

        iterate2 (i, fonts)
        {
            FontEntry& current = i;

            if (current.path == entry.path && current.size == entry.size && current.flags == entry.flags)
                return i.getIndex();
        }

        return fonts.add(entry);
    }*/
}
