#pragma once

#include "gameui_base.hpp"

#ifdef GAMEUI_THEMER_DEFAULT_METHODS
#define GAMEUI_THEMER_METHOD {}
#define TM(m_) virtual m_ {}
#define TM_RETVAL(m_, value_) virtual m_ { return (value_); }
#else
#define GAMEUI_THEMER_METHOD = 0;
#define TM(m_) virtual m_ = 0;
#define TM_RETVAL(m_, value_) virtual m_ = 0;
#endif

#define TM_REQ(m_) virtual m_ = 0;

namespace gameui
{
    enum
    {
        FONT_BOLD,
        FONT_ITALIC
    };

    class IButtonThemer
    {
        public:
            virtual ~IButtonThemer() {}

            virtual bool AcquireResources() = 0;

            virtual const char* GetLabel() = 0;
            virtual Int2 GetMinSize() = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
            virtual void SetFont(intptr_t fontid) = 0;
            virtual void SetMouseOver(bool isOver) = 0;
            virtual void SetLabel(const char* label) = 0;

            virtual void Draw() = 0;
    };

    class ICheckBoxThemer
    {
        public:
            virtual ~ICheckBoxThemer() {}

            TM(bool AcquireResources())

            virtual Int2 GetMinSize() GAMEUI_THEMER_METHOD
            virtual void SetArea(Int3 pos, Int2 size) GAMEUI_THEMER_METHOD
            virtual void SetChecked(bool isChecked) GAMEUI_THEMER_METHOD
            virtual void SetFont(intptr_t fontid) GAMEUI_THEMER_METHOD
            virtual void SetMouseOver(bool isOver) GAMEUI_THEMER_METHOD

            virtual void Draw() {}
    };

    class IComboBoxThemer
    {
        public:
            virtual ~IComboBoxThemer() {}

            virtual bool AcquireResources() = 0;

            virtual Int2 GetMinSize(Int2 contentsMinSize) = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
            virtual void SetContentsSize(Int2 size) = 0;
            virtual void SetExpanded(bool isExpanded) = 0;
            virtual void SetPopupArea(Int3 pos, Int2 size) = 0;
            virtual void SetPopupSelectionArea(bool enabled, Int3 pos, Int2 size) = 0;

            virtual void Draw() {}
            virtual void DrawPopup() {}
    };

    class IListBoxThemer
    {
        public:
            virtual ~IListBoxThemer() {}

            virtual bool AcquireResources() = 0;

            virtual void Draw() = 0;
    };

    class IGraphicsThemer
    {
        public:
            virtual ~IGraphicsThemer() {}

            virtual bool AcquireResources() = 0;

            virtual void Draw() = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
    };

    class IMenuBarThemer
    {
        public:
            struct Item_t;
            enum { ITEM_ENABLED = 1, ITEM_EXPANDED = 2, ITEM_HAS_PARENT = 4, ITEM_HAS_CHILDREN = 8, ITEM_SELECTED = 16 };

            virtual ~IMenuBarThemer() {}

            virtual bool AcquireResources() = 0;

            virtual Item_t* AllocItem(Item_t* parent, const char* label) = 0;
            virtual void Draw() = 0;
            virtual void DrawItem(Item_t* item, const Int3& pos, const Int2& size, const Int3& labelPos, const Int2& labelSize) = 0;
            virtual void DrawMenu(const Int3& pos, const Int2& size) = 0;
            virtual const char* GetItemLabel(Item_t* item) = 0;
            virtual Int2 GetItemLabelSize(Item_t* item) = 0;
            virtual void ReleaseItem(Item_t* item) = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
            virtual void SetItemFlags(Item_t* item, int flags) = 0;            
    };

    class IPanelThemer
    {
        public:
            virtual ~IPanelThemer() {}

            TM(bool AcquireResources())

            TM(void SetArea(Int3 pos, Int2 size))
            TM(void SetColour(const Float4& colour))

            TM(void Draw())
    };

    class IScrollBarThemer
    {
        // FIXME: Totally needs clean-up

        public:
            virtual ~IScrollBarThemer() {}

            virtual bool AcquireResources() = 0;

            virtual void DrawVScrollBar(Int2 pos, Int2 size, int y, int h) = 0;
            virtual int GetWidth() = 0;
    };

    class ISliderThemer
    {
        public:
            virtual ~ISliderThemer() {}

            virtual bool AcquireResources() = 0;

            virtual void Draw(bool enabled, float sliderValue) = 0;
            virtual Int2 GetMinSize() = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
            virtual void SetRange(float min, float max, float tickStep) = 0;            
    };

    class IStaticImageThemer
    {
        public:
            virtual ~IStaticImageThemer() {}

            virtual bool AcquireResources() = 0;
            virtual void DropResources() = 0;

            virtual void Draw() = 0;
            virtual Int2 GetMinSize() = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
    };

    class IStaticTextThemer
    {
        public:
            virtual ~IStaticTextThemer() {}
        
            virtual bool AcquireResources() = 0;

            virtual const char* GetLabel() = 0;
            virtual Int2 GetMinSize() = 0;
            virtual void SetArea(Int3 pos, Int2 size) = 0;
            virtual void SetColour(const Float4& colour) = 0;
            virtual void SetFont(intptr_t fontid) = 0;
            virtual void SetLabel(const char* label) = 0;

            virtual void Draw() = 0;
    };

    class ITextBoxThemer
    {
        public:
            virtual ~ITextBoxThemer() {}

            virtual bool AcquireResources() = 0;

            virtual void Draw() = 0;
            virtual Int2 GetMinSize() GAMEUI_THEMER_METHOD
            virtual const char* GetText() = 0;
            virtual size_t InsertCharacter(size_t pos, UnicodeChar c) = 0;
            virtual void SetActive(bool active) GAMEUI_THEMER_METHOD
            virtual void SetArea(Int3 pos, Int2 size) GAMEUI_THEMER_METHOD
            virtual void SetCursorPos(Int3 pos, size_t* followingCharIndex_out) = 0;
            virtual void SetFont(intptr_t fontid) GAMEUI_THEMER_METHOD
            virtual void SetMouseOver(bool isOver) GAMEUI_THEMER_METHOD
            virtual void SetText(const char* text) GAMEUI_THEMER_METHOD
    };

    class ITreeBoxThemer
    {
        public:
            struct Item_t;

            virtual ~ITreeBoxThemer() {}

            TM(bool AcquireResources())

            TM_RETVAL(Item_t* AllocItem(Item_t* parent, const char* label), nullptr)
            TM(void DrawItem(Item_t* item, const Int3& arrowPos, const Int3& labelPos))
            TM_RETVAL(Int2 GetArrowSize(), Int2())
            TM_RETVAL(Int2 GetItemLabelSize(Item_t* item), Int2())
            virtual const char* GetItemLabel(Item_t* item) = 0;
            virtual void ReleaseItem(Item_t* item) = 0;
            TM(void SetItemExpanded(Item_t* item, bool expanded))
            TM(void SetItemHasChildren(Item_t* item, bool hasChildren))
            virtual void SetItemLabel(Item_t* item, const char* label) = 0;
            TM(void SetItemSelected(Item_t* item, bool selected))
    };

    class IWindowThemer
    {
        public:
            virtual ~IWindowThemer() {}

            virtual const char* GetTitle() = 0;
            virtual Int2 GetTitleMinSize() = 0;
            virtual void SetTitle(const char* title) = 0;
    };

    class UIThemerUtil
    {
        public:
            static bool TryParseFontParams(const char* font, li::String& path_out, int& size_out, int& flags_out);

            // TODO[C++14]: mark as [[deprecated]]
            static bool IsFontParams(const char* font, li::String& path_out, int& size_out, int& flags_out) {
                return TryParseFontParams(font, path_out, size_out, flags_out);
            }
    };

    class UIThemer
    {
        public:
            virtual ~UIThemer() {}

            virtual bool AcquireResources() = 0;
            virtual void DropResources() = 0;

            virtual intptr_t PreregisterFont(const char* name, const char* params) = 0;
            virtual intptr_t GetFontId(const char* font) = 0;       // params or name

            virtual IButtonThemer*      CreateButtonThemer(Int3 pos, Int2 size, const char* label) = 0;
            virtual ICheckBoxThemer*    CreateCheckBoxThemer(Int3 pos, Int2 size, const char* label, bool checked) = 0;
            virtual IComboBoxThemer*    CreateComboBoxThemer(Int3 pos, Int2 size) = 0;
            virtual IGraphicsThemer*    CreateGraphicsThemer(shared_ptr<IGraphics> g) = 0;
            virtual IListBoxThemer*     CreateListBoxThemer() = 0;
            virtual IMenuBarThemer*     CreateMenuBarThemer() = 0;
            virtual IPanelThemer*       CreatePanelThemer(Int3 pos, Int2 size) = 0;
            virtual IScrollBarThemer*   CreateScrollBarThemer(bool horizontal) = 0;
            virtual ISliderThemer*      CreateSliderThemer() = 0;
            virtual IStaticImageThemer* CreateStaticImageThemer(Int3 pos, Int2 size, const char* path) = 0;
            virtual IStaticTextThemer*  CreateStaticTextThemer(Int3 pos, Int2 size, const char* label) = 0;
            virtual ITextBoxThemer*     CreateTextBoxThemer(Int3 pos, Int2 size) = 0;
            virtual ITreeBoxThemer*     CreateTreeBoxThemer(Int3 pos, Int2 size) = 0;
            virtual IWindowThemer*      CreateWindowThemer(IPanelThemer* thm, int flags) = 0;
    };
}

#undef TM
