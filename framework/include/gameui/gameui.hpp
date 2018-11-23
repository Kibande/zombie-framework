#pragma once
 
#include "gameui_base.hpp"
#include "uithemer.hpp"

namespace gameui
{
    using li::String;

    class ComboBoxPopup;
    class UIContainer;

    // FIXME: This totally needs to go
    static const int scrollbar_pad = 2;

    class Widget
    {
        protected:
            String name;

            Widget* parent;
            WidgetContainer *parentContainer;

            bool freeFloat, expands, visible, enabled, onlineUpdate;
            int align;
            Int3 areaPos, pos;
            Int2 areaSize, size, minSize, maxSize;

            // doesn't really belong here but it's the best way
            Short2 posInTable;

            MessageQueue* msgQueue;

            //ClickEventHandler clickEventHandler;
            //MouseDownEventHandler mouseDownEventHandler;

            void FireClickEvent(int x, int y);
            void FireItemSelectedEvent(int index);
            void FireMenuItemSelectedEvent(MenuItem_t* item);
            void FireMouseDownEvent(int button, int x, int y);
            void FireTreeItemSelectedEvent(TreeItem_t* item);
            void FireValueChangeEvent(int value);
            void FireValueChangeEvent(float floatValue);
            bool IsInsideMe(int x, int y) { return (visible && x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y); }

        private:
            Widget(const Widget&);

        protected:
            Widget();
            Widget(Int3 pos, Int2 size);

        public:
            virtual ~Widget();

            virtual bool AcquireResources() { return true; }
            virtual void DropResources() {}

            virtual Widget* Find(const char* widgetName) { return nullptr; }
            bool GetEnabled() const { return enabled; }
            bool GetExpands() const { return expands; }
            bool GetFreeFloat() const { return freeFloat; }
            virtual Int2 GetMaxSize() { return maxSize; }
            virtual Int2 GetMinSize() { return minSize; }
            const char* GetName() const { return name.c_str(); }
            const String& GetNameString() const { return name; }
            Widget* GetParent() {return parent;}
            WidgetContainer* GetParentContainer() {return parentContainer;}
            Int3 GetPos() const { return pos; }
            Short2 GetPosInTable() const { return posInTable; }
            Int2 GetSize() const { return size; }
            bool GetVisible() const { return visible; }
            virtual void Layout();
            virtual void Move(Int3 vec) { pos += vec; areaPos += vec; }
            virtual int OnCharacter(int h, UnicodeChar c) { return h; }
            virtual int OnMouseMove(int h, int x, int y) { return h; }
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) { return h; }
            virtual void SetAlign(int align) { this->align = align; }
            virtual void SetArea(Int3 pos, Int2 size);
            virtual void SetEnabled(bool enabled, bool recursive = true);
            virtual void SetExpands(bool expands) { this->expands = expands; }
            virtual void SetFreeFloat(bool freeFloat) { this->freeFloat = freeFloat; }
            virtual void SetMaxSize(Int2 maxSize) { this->maxSize = maxSize;/* size = glm::min(size, maxSize);*/ }
            virtual void SetMinSize(Int2 minSize) { this->minSize = minSize;/* size = glm::max(size, minSize);*/ }
            Widget* SetName(String&& name) { this->name = (String&&) name; return this; }
            Widget* SetName(const char* name) { this->name = name; return this; }
            //void SetClickEventHandler(ClickEventHandler handler) {clickEventHandler = handler;}
            //void SetMousePrimaryDownEventHandler(MouseDownEventHandler handler) {mouseDownEventHandler = handler;}
            virtual void SetOnlineUpdate(bool enabled) { onlineUpdate = enabled; }
            void SetParent(Widget* parent) {this->parent = parent;}
            void SetParentContainer(WidgetContainer* parentContainer) {this->parentContainer = parentContainer;}
            virtual void SetPos(Int3 pos) { Move(pos - this->pos); }
            virtual void SetPosInTable(Short2 posInTable) { this->posInTable = posInTable; }
            void SetPrivateMessageQueue(MessageQueue* queue);
            virtual void SetSize(Int2 size) { this->size = size; }
            virtual void SetVisible(bool visible) { this->visible = visible; }
            void ShowAt(Int3 pos) { SetPos(pos); SetVisible(true); }

            static MessageQueue* GetMessageQueue();
            static void SetMessageQueue(MessageQueue* queue);

            virtual void Draw() {} // { DrawAt(pos); }
            //virtual void DrawAt(Int2 pos) {}
            virtual void OnFrame( double delta ) {}
    };

    class WidgetContainer
    {
        protected:
            Int3 ctnAreaPos;
            Int2 ctnAreaSize;
            bool ctnOnlineUpdate;

            li::List<Widget*> widgets;

            size_t numTopLevelWidgets;

        private:
            WidgetContainer(const WidgetContainer&);

        public:
            WidgetContainer();
            ~WidgetContainer();

            virtual void Add(Widget* widget);
            virtual void CtnSetArea(Int3 pos, Int2 size);
            virtual void CtnSetOnlineUpdate(bool enabled) { ctnOnlineUpdate = enabled; }
            virtual void DoFocusWidget(Widget* widget);
            virtual bool DoRemoveWidget(Widget* widget);
            virtual void OnWidgetMinSizeChange(Widget* widget) { widget->Layout(); }

            bool AcquireResources();
            void DelayedFocusWidget(Widget* widget);
            void DelayedRemoveWidget(Widget* widget, bool release);
            void Draw();
            Widget* Find(const char* widgetName);
            auto GetWidgetList() -> decltype(widgets)& { return widgets; }
            void Layout();
            void Move(Int3 vec);
            void OnFrame(double delta);
            int OnCharacter(int h, UnicodeChar c);
            int OnMouseMove(int h, int x, int y);
            int OnMousePrimary(int h, int x, int y, bool pressed);
            void RemoveAll();

            template <typename WidgetClass> WidgetClass* FindWidget(const char* widgetName) {
                return dynamic_cast<WidgetClass*>(Find(widgetName));
            }
    };

    // FIXME: Get rid of this
    class ScrollBarV
    {
        public:
            unique_ptr<IScrollBarThemer> thm;
        
            Int2 pos, size;
            int y, h;
        
            int ydiff;
        
            bool dragging;
            int yOrig;
        
        public:
            ScrollBarV(UIThemer* themer);
        
            void Draw();
            int OnMouseMove(int h, int x, int y);
            int OnMousePrimary(int h, int x, int y, bool pressed);
        
            int GetWidth() const { return thm->GetWidth(); }
    };

    class Button : public Widget
    {
        private:
            unique_ptr<IButtonThemer> thm;

            Button(const Button&);

        public:
            Button(UIThemer* themer, const char* label);
            Button(UIThemer* themer, const char* label, Int3 pos, Int2 size);
            virtual ~Button();

            virtual bool AcquireResources() override;

            virtual void Draw() override;
            virtual Int2 GetMinSize() override { return thm->GetMinSize(); }
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void OnFrame(double delta) override;

            const char* GetLabel() { return thm->GetLabel(); }
            //void SetColour(const Float4& colour) { painter.SetColour(colour); }
            void SetFont(intptr_t fontid) { thm->SetFont(fontid); }
            void SetLabel(const char* label);
            bool WasClicked() const { return wasClicked; }

        private:
            bool wasClicked = false;
            bool wasClickedThisFrame = false;
    };

    class CheckBox : public Widget
    {
        private:
            unique_ptr<ICheckBoxThemer> thm;

            bool checked;

            CheckBox(const CheckBox&);

        public:
            CheckBox(UIThemer* themer, const char* label, bool checked);
            CheckBox(UIThemer* themer, const char* label, bool checked, Int3 pos, Int2 size);
            virtual ~CheckBox();

            virtual bool AcquireResources() override;

            virtual void Draw() override;
            virtual Int2 GetMinSize() override { return thm->GetMinSize(); }
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void OnFrame(double delta) override;

            bool GetChecked() { return checked; }
            void SetChecked(bool checked);
            //void SetColour(const Float4& colour) { painter.SetColour(colour); }
            void SetFont(intptr_t fontid) { thm->SetFont(fontid); }
    };

    class ComboBox : public Widget
    {
        friend class ComboBoxPopup;

        protected:
            unique_ptr<IComboBoxThemer> thm;
            ComboBoxPopup* popup;

            UIContainer* uiBase;

        protected:
            bool isExpanded;
            int selection;
            Widget* selectedItem;

            bool rebuildNeeded;
            Int2 contentsSize, padding;

            void ClosePopup();

        private:
            ComboBox(const ComboBox&);

        public:
            ComboBox(UIThemer* themer, UIContainer* uiBase);
            virtual ~ComboBox();

            void Add(Widget* widget);
            virtual void Draw() override;
            virtual Int2 GetMaxSize() override;
            virtual Int2 GetMinSize() override;
            Widget* GetSelectedItem() { return selectedItem; }
            //virtual void InvalidateLayout() override { rebuildNeeded = true; if (parent != nullptr) parent->InvalidateLayout(); }
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            //virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            //virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            /*virtual void Relayout() override;
            virtual void ReloadMedia() override;*/
            void SetSelection(int selection);
    };
    
    class ListCtrl : public Widget, public WidgetContainer
    {
        protected:
            unique_ptr<IListBoxThemer> thm;
        
            int spacing;
        
            ScrollBarV scrollbar;

            bool rebuildNeeded;
            int contents_w, contents_h;
            li::Array<int> rowHeights;

        public:
            ListCtrl(UIThemer* themer);

            virtual bool AcquireResources() override;

            virtual void Add(Widget* widget) override;
            virtual void Draw() override;
            virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            virtual void Move(Int3 vec) override;
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void Layout() override;
            virtual void SetArea(Int3 pos, Int2 size) override;
            //virtual void SetSpacing(Int2 spacing) {this->spacing = spacing;}
    };

    class Graphics : public Widget
    {
        public:
            Graphics(UIThemer* themer, shared_ptr<IGraphics> g);
            virtual ~Graphics();

            virtual bool AcquireResources() override { return thm->AcquireResources(); }

            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual void Layout() override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

        protected:
            unique_ptr<IGraphicsThemer> thm;

            shared_ptr<IGraphics> g;

            bool wasMouseInsideMe;
    };

    class MenuBar : public Widget
    {
        public:
            MenuBar(UIThemer* themer);
            virtual ~MenuBar();

            virtual bool AcquireResources() override { return thm->AcquireResources(); }

            virtual MenuItem_t* Add(MenuItem_t* parent, const char* label, const char* name, int flags);
            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual void Layout() override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void SetArea(Int3 pos, Int2 size) override;

            MenuItem_t* GetItemByName(const char* name) { return pr_GetItemByName(firstItem, name); }
            const char* GetItemLabel(MenuItem_t* item);
            void SetDragsAppWindow(bool dragsAppWindow) { this->dragsAppWindow = dragsAppWindow; }
            void SetItemFlags(MenuItem_t* item, int flags);

        protected:
            unique_ptr<IMenuBarThemer> thm;

            bool dragsAppWindow;
            Short2 barItemPadding, menuItemPadding;

            MenuItem_t *firstItem, *lastItem, *expandedItem, *selectedItem;
            bool minSizeValid, layoutValid, isDraggingAppWindow;
            Int2 appWindowDragOrigin;

            void LayoutItem(Int2 posInWidgetSpace, MenuItem_t* item, MenuItem_t* parent);
            int OnMouseMoveTo(int h, Int2 pos, MenuItem_t* item, int level);
            int OnMousePress(int h, Int2 pos, MenuItem_t* item, bool isTopLevel);
            void p_LayoutMenu(Int2 posInWidgetSpace, MenuItem_t* item);
            void p_LayoutSubMenus(int x, MenuItem_t* item, MenuItem_t* parent);
            void p_SetExpandedItem(MenuItem_t* item);
            void pr_DrawItem(MenuItem_t* item, bool isTopLevel);
            MenuItem_t* pr_GetItemByName(MenuItem_t* item, const char* name);
            void pr_ReleaseItem(MenuItem_t* item);

        private:
            MenuBar(const MenuBar&);
    };

    struct MenuItem_t
    {
        String name;

        IMenuBarThemer::Item_t* ti;
        MenuItem_t *prev, *next, *first_child, *last_child;

        bool expanded;
        int flags, thmFlags;
        Short2 pos, size, iconPos, iconSize, labelPos, labelSize, menuPos, menuSize;
    };

    class Panel : public Widget, public WidgetContainer
    {
        protected:
            unique_ptr<IPanelThemer> thm;

            Int2 padding;
            bool transparent;

            bool minSizeValid, layoutValid;

            void OnlineUpdateForWidget(Widget* widget);

        private:
            Panel(const Panel&);

        public:
            Panel(UIThemer* themer);
            Panel(UIThemer* themer, Int3 pos, Int2 size);
            virtual ~Panel();

            virtual bool AcquireResources() override;
            virtual void Add(Widget* widget) override;
            virtual void Draw() override;
            virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            virtual int OnCharacter(int h, UnicodeChar c) override;
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void OnWidgetMinSizeChange(Widget* widget) override;
            virtual void SetArea(Int3 pos, Int2 size) override;
            virtual void SetOnlineUpdate(bool enabled) override { Widget::SetOnlineUpdate(enabled); CtnSetOnlineUpdate(enabled); }
            virtual void SetSize(Int2 size) override;

            void InvalidateLayout() { this->layoutValid = false; }
            void SetColour(const Float4& colour);
            void SetPadding(const Int2 padding) { this->padding = padding; }
    };

    class Slider : public Widget
    {
        public:
            Slider(UIThemer* themer);
            virtual ~Slider();

            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            float GetValue() const { return value; }
            void SetRange(float min, float max, float tickStep);
            void SetSnap(SnapMode_t snap) { this->snap = snap; }
            void SetValue(float value, bool fireEvents = false);

        protected:
            unique_ptr<ISliderThemer> thm;

            float min, max, tick, value;
            SnapMode_t snap;

            bool mouseDown;

        private:
            Slider(const Slider&);
    };

    class Spacer : public Widget
    {
        private:
            Spacer(const Spacer&);

        public:
            Spacer(UIThemer* themer, Int2 size) : Widget() { minSize = size; }
    };

    class StaticImage : public Widget
    {
        protected:
            unique_ptr<IStaticImageThemer> thm;

        private:
            StaticImage(const StaticImage&);

        public:
            StaticImage(UIThemer* themer, const char* path);
            virtual ~StaticImage();

            virtual bool AcquireResources() override { return thm->AcquireResources(); }
            virtual void DropResources() override { thm->DropResources(); }

            virtual void Draw() override;
            virtual Int2 GetMinSize() override { return thm->GetMinSize(); }
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            //void SetColour(const glm::vec4& colour) { painter.SetColour(colour); }
            //void SetMapping(ImageMapping mapping) { painter.SetMapping(mapping); }
    };

    class StaticText : public Widget
    {
        protected:
            unique_ptr<IStaticTextThemer> thm;

        private:
            StaticText(const StaticText&);

        public:
            StaticText(UIThemer* themer, const char* label, size_t font = 0);
            StaticText(UIThemer* themer, const char* label, Int3 pos, Int2 size, size_t font = 0);
            virtual ~StaticText();

            virtual bool AcquireResources() override;
            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;

            void SetColour(const Float4& colour) { thm->SetColour(colour); }
            const char* GetLabel() { return thm->GetLabel(); }
            void SetFont(intptr_t fontid) { thm->SetFont(fontid); }
            void SetLabel(const char* label);

            /*virtual void DrawAt(const Int2& pos) override;
            virtual void ReloadMedia() override;
            void SetColour(const glm::vec4& colour) { painter.SetColour(colour); }*/
    };

    class TableLayout : public Widget, public WidgetContainer
    {
        protected:
            struct RowCol_t
            {
                int minSize;            // minimal size (determined by contents)
                int offset;             // offset from Widget::pos
                int actualSize;         // actual size (padded for growable rows/columns)
                int pad_to_16_bytes;    // filler yo
            };

            unsigned int numColumns, numRows;
            Int2 spacing;

            bool* columnsGrowable;
            li::Array<bool> rowsGrowable;

            // Calculated by Layout(0) (GetMinSize)
            bool minSizeValid, layoutValid;
            unsigned int numGrowableColumns, numGrowableRows;
            li::Array<RowCol_t, uint32_t> columnMetrics, rowMetrics;

            void OnlineUpdateForWidget(Widget* widget);

        private:
            TableLayout(const TableLayout&);

        public:
            TableLayout(unsigned int numColumns);
            virtual ~TableLayout();

            virtual bool AcquireResources() override;
            virtual void Add(Widget* widget) override;
            virtual void Draw() override { WidgetContainer::Draw(); }
            virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            virtual void Move(Int3 vec) override;
            virtual int OnCharacter(int h, UnicodeChar c) override { return WidgetContainer::OnCharacter(h, c); }
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override { return WidgetContainer::OnMouseMove(h, x, y); }
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override { return WidgetContainer::OnMousePrimary(h, x, y, pressed); }
            virtual void OnWidgetMinSizeChange(Widget* widget) override;
            /*virtual void Relayout() override;
            virtual void ReloadMedia() override;*/
            virtual void Layout() override;
            virtual void SetArea(Int3 pos, Int2 size) override;
            virtual void SetOnlineUpdate(bool enabled) override { Widget::SetOnlineUpdate(enabled); CtnSetOnlineUpdate(enabled); }

            unsigned int GetNumRows() const;
            void InvalidateLayout() { layoutValid = false; }
            void RemoveAll();

            // These COULD be done cheaper, but nobody uses them after initialization anyways
            void SetColumnGrowable( size_t column, bool growable ) { if (column < numColumns) { columnsGrowable[column] = growable; minSizeValid = false; } }
            void SetRowGrowable( size_t row, bool growable ) { rowsGrowable[row] = growable; minSizeValid = false; }

            void SetNumColumns(unsigned int numColumns); 
            void SetSpacing(Int2 spacing) { this->spacing = spacing; minSizeValid = false; layoutValid = false; }
    };

    class TextBox : public Widget
    {
        protected:
            unique_ptr<ITextBoxThemer> thm;

            bool active;
            String text;
            size_t cursorPos;

        private:
            TextBox(const TextBox&);

        public:
            TextBox(UIThemer* themer);
            ~TextBox();

            virtual bool AcquireResources() override { return thm->AcquireResources(); }
            virtual void Draw() override;
            virtual Int2 GetMinSize() override { return thm->GetMinSize(); }
            virtual void Layout() override;
            virtual void Move(Int3 vec) override;
            virtual int OnCharacter(int h, UnicodeChar c) override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            const char* GetText() const { return thm->GetText(); }
            void SetText(const char* text);
    };

    class TreeBox : public Widget
    {
        protected:
            unique_ptr<ITreeBoxThemer> thm;

            TreeItem_t *firstItem, *lastItem, *selectedItem;
            Int2 arrowSize, contentsSize;
            int label_x_offset, y_spacing;

            bool minSizeValid, layoutValid;

            void DrawItem(TreeItem_t* item);
            void LayoutItem(Int2& pos, TreeItem_t* item);
            void MeasureItem(Int2& pos, Int2& size, const TreeItem_t* item);
            bool OnMousePress(Int2 pos, TreeItem_t* item);
            void pr_ReleaseItem(TreeItem_t* item);
            bool pr_RemoveItem(TreeItem_t* lookFor, TreeItem_t* item, TreeItem_t* parent);
            void pr_SetExpanded(TreeItem_t* item, bool expanded);

        private:
            TreeBox(const TreeBox&);

        public:
            TreeBox(UIThemer* themer);
            virtual ~TreeBox();

            virtual bool AcquireResources() override;

            virtual void Draw() override;
            virtual Int2 GetMinSize() override;
            virtual void Layout() override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            TreeItem_t* Add(const char* label, TreeItem_t* parent = nullptr);
            void Clear();
            void CollapseAll();
            void ExpandAll();
            const char* GetItemLabel(TreeItem_t* item);
            TreeItem_t* GetSelection() { return selectedItem; }
            bool HasChildren(TreeItem_t* parentOrNull);
            void RemoveItem(TreeItem_t* item, bool selectNext);
            void SetItemLabel(TreeItem_t* item, const char* label);
            void SetSelection(TreeItem_t* item, bool fireEvents = false);
    };

    struct TreeItem_t
    {
        ITreeBoxThemer::Item_t* ti;
        TreeItem_t *prev, *next, *first_child, *last_child;

        bool expanded;
        Int2 arrowPos, labelPos, labelSize;
        int height;
    };

    class Window : public Panel
    {
        protected:
            IWindowThemer* wthm;

            int flags;

            int state;
            Int2 titleSize;
            Int2 dragFrom;

        private:
            Window(const Window&);

        public:
            enum { NO_HEADER = 1, RESIZABLE = 2 };

            Window(UIThemer* themer, const char* title, int flags);
            Window(UIThemer* themer, const char* title, int flags, Int3 pos, Int2 size);

            virtual bool AcquireResources() override;
            virtual Int2 GetMinSize() override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            void Close();
            void Focus();
            const char* GetTitle() { return wthm->GetTitle(); }
    };

    class Popup : public Window
    {
        protected:
            bool easyDismiss;

        private:
            Popup(const Popup&);

        public:
            Popup(UIThemer* themer);
            //Popup(UIThemer* themer, Int3 pos, Int2 size);

            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            void SetEasyDismiss(bool enable) { easyDismiss = enable; }
    };

    class PopupMenu : public Popup
    {
        public:
            enum PopupMenuEntryFlags
            {
                ENTRY_SELECTABLE = 1,
                ENTRY_HAS_ICON = 2
            };
        
            struct PopupMenuEntry
            {
                String label, name;
                int flags;
            };

        protected:
            li::List<PopupMenuEntry> entries;

        private:
            PopupMenu(const PopupMenu&);

        public:
            PopupMenu(UIThemer* themer);
            PopupMenu(UIThemer* themer, Int3 pos, Int2 size);

            void AddEntry(const char* label, const char* name);
            virtual void Draw() override;
            virtual void Layout() override;
    };

    class ComboBoxPopup : public Window
    {
        friend class ComboBox;

        protected:
            ComboBox* comboBox;
            IComboBoxThemer* thm;
            TableLayout* table;

            int mouseOver;

            int GetItemAt(int x, int y);
            Widget* GetItemByIndex(int index);

        private:
            ComboBoxPopup(const ComboBoxPopup&);

        protected:
            ComboBoxPopup(UIThemer* themer, ComboBox* comboBox);

        public:
            virtual void Draw() override;
            virtual void Layout() override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
    };

    class UIContainer : public WidgetContainer
    {
        protected:
            IEngine* sys;

            Int3 pos;
            Int2 size;

            li::List<Widget*> modalStack;
            Int2 mousePos;

            int OnMousePrimary(int h, int x, int y, bool pressed);

        private:
            UIContainer(const UIContainer&) = delete;

        public:
            UIContainer(IEngine* sys) : sys(sys) {}
            ~UIContainer();

            virtual bool DoRemoveWidget(Widget* widget) override;
            //virtual void OnWidgetMinSizeChange(Widget* widget) override;

            void CenterWidget(Widget* widget);
            void Draw();
            Widget* Find(const char* widgetName);
            //Widget* Find(const char* widgetName, Uniq_t widgetType);
            void OnFrame(double delta);
            void OnLoopEvent(EventLoopEvent* payload);
            int OnCharacter(int h, UnicodeChar c);
            int OnMouseButton(int h, int button, bool pressed, int x, int y);
            int OnMouseMove(int h, int x, int y);
            void PopModal(bool release);
            void PushModal(Widget * modal);
            bool RestoreUIState(const char* path);
            bool SaveUIState(const char* path);
            void SetArea(Int3 pos, Int2 size);
            void SetOnlineUpdate(bool enabled) { CtnSetOnlineUpdate(enabled); }

            int HandleMessage(int h, MessageHeader* msg);
    };

    class UILoader
    {
        IEngine* sys;

        UIContainer* uiBase;
        UIThemer* themer;

        UILoaderCallback customLoaderCb;
        intptr_t defaultFont;

        protected:
            Widget* Load(const char* fileName, cfx2::Node node, bool acquireResources);

        private:
            UILoader(const UILoader&);

        public:
            UILoader(IEngine* sys, UIContainer* uiBase, UIThemer* themer);
           
            void SetCustomLoaderCallback(UILoaderCallback callback) { customLoaderCb = callback; }
            void SetDefaultFont(intptr_t defaultFont) { this->defaultFont = defaultFont; }

            //Widget* Load(const char* fileName);
            bool Load(const char* fileName, WidgetContainer* container, bool acquireResources);

            intptr_t GetFontId(const char* fontOrNull);     // name (pre-defined within themer) or params
            UIThemer* GetThemer() { return themer; }
    };

    // Compatibility
    typedef TableLayout Table;

#ifdef ZOMBIE_NEW_EVENT
    enum { MOUSE_BTN_LEFT, MOUSE_BTN_RIGHT, MOUSE_BTN_MIDDLE, MOUSE_BTN_WHEELUP, MOUSE_BTN_WHEELDOWN, MOUSE_BTN_OTHER = 10 };
#endif
}
