#pragma once

#include <littl/cfx2.hpp>
#include <littl/List.hpp>
#include <littl/String.hpp>

namespace gameui
{
    using namespace li;

    struct TreeBoxNode;

    class Widget;
    class WidgetContainer;

    struct WidgetLoadProperties;

    enum ImageMapping {
        IMAGEMAP_NORMAL,
        IMAGEMAP_HSLICE,
        IMAGEMAP_VSLICE
    };

    class Command {
        public:
            enum {
                ID_UNDEF,
                ID_OTHER,
                ID_BUMP_WIDGET,
                ID_REMOVE_WIDGET
            };

            int cmd_id;
            const char *cmd_name;

            union {
                Widget *widget_ptr;
            } data;

        public:
            Command(int cmd_id = ID_UNDEF, Widget *widget = nullptr)
                : cmd_id(cmd_id), cmd_name(nullptr)
            {
                data.widget_ptr = widget;
            }

            Command(const char *cmd_name) : cmd_id(ID_OTHER), cmd_name(cmd_name)
            {
                data.widget_ptr = nullptr;
            }
    };
}

#include "gameui_defaultskin.hpp"

namespace gameui
{
    struct WidgetLoadProperties
    {
        Int2 pos, size;
        bool havePos, haveSize;
    };

    typedef void (*ClickEventHandler)(Widget* widget, int x, int y);
    typedef void (*MousePrimaryDownEventHandler)(Widget* widget, int x, int y);

    // for mouse, keyboard events:
    //  return < 0: don't care, pass
    //  return 0:   this event was reacted to, pass
    //  return 1:   stop propagating this event

    class Widget
    {
        public:
            enum { LEFT = 0, TOP = 0, HCENTER = 1, RIGHT = 2, VCENTER = 4, BOTTOM = 8 };

        protected:
            UIResManager* res;
            String name;

            Widget* parent;
            WidgetContainer *parentContainer;
    
            bool freeFloat, expands, visible;
            int align;
            Int2 pos, size, minSize, maxSize;

            ClickEventHandler clickEventHandler;
            MousePrimaryDownEventHandler mouseDownEventHandler;

            void FireClickEvent(int x, int y) { if (clickEventHandler != nullptr) clickEventHandler(this, x, y); else Event::PushUIMouseEvent(EV_UICLICK, this, x, y); }
            void FireItemSelectedEvent(int index) { Event::PushUIItemEvent(EV_UIITEMSELECTED, this, index); }
            void FireMouseDownEvent(int x, int y) { if (mouseDownEventHandler != nullptr) mouseDownEventHandler(this, x, y); else Event::PushUIMouseEvent(EV_UIMOUSEDOWN, this, x, y); }
            bool IsInsideMe(int x, int y) { return (x >= pos.x && y >= pos.y && x < pos.x + size.x && y < pos.y + size.y); }

        private:
            Widget(const Widget&);

        protected:
            Widget(UIResManager* res);
            Widget(UIResManager* res, Int2 pos, Int2 size);

        public:
            virtual ~Widget();

            const String& GetName() const { return name; }
            void SetName(String&& name) { this->name = (String&&) name; }
            void SetName(const char* name) { this->name = name; }

            virtual void ReleaseMedia() {}
            virtual void ReloadMedia() {}

            virtual Widget* Find(const char* widgetName) { return nullptr; }
            bool GetExpands() const { return expands; }
            bool GetFreeFloat() const { return freeFloat; }
            virtual Int2 GetMaxSize() { return maxSize; }
            virtual Int2 GetMinSize() { return minSize; }
            Widget* GetParent() {return parent;}
            WidgetContainer* GetParentContainer() {return parentContainer;}
            CInt2& GetPos() const { return pos; }
            CInt2& GetSize() const { return size; }
            bool GetVisible() const { return visible; }
            virtual void Move(Int2 vec) { pos += vec; }
            virtual int OnMouseMove(int h, int x, int y) { return h; }
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) { return h; }
            virtual void Relayout() {}
            virtual void SetAlign(int align) { this->align = align; }
            virtual void SetArea(Int2 pos, Int2 size);
            virtual void SetExpands(bool expands) { this->expands = expands; }
            virtual void SetFreeFloat(bool freeFloat) { this->freeFloat = freeFloat; }
            virtual void SetMaxSize(Int2 maxSize) { this->maxSize = maxSize; size = glm::min(size, maxSize); }
            virtual void SetMinSize(Int2 minSize) { this->minSize = minSize; size = glm::max(size, minSize); }
            void SetClickEventHandler(ClickEventHandler handler) {clickEventHandler = handler;}
            void SetMousePrimaryDownEventHandler(MousePrimaryDownEventHandler handler) {mouseDownEventHandler = handler;}
            void SetParent(Widget* parent) {this->parent = parent;}
            void SetParentContainer(WidgetContainer* parentContainer) {this->parentContainer = parentContainer;}
            virtual void SetPos(Int2 pos) { Move(pos - this->pos); }
            //virtual void SetSize(Int2 size) {this->size = size;}
            virtual void SetVisible(bool visible) {this->visible = visible;}
            void ShowAt(CInt2& pos) {SetPos(pos); SetVisible(true);}

            virtual void Draw() { DrawAt(pos); }
            virtual void DrawAt(const Int2& pos) {}
            virtual void OnFrame( double delta ) {}
    };

    class WidgetContainer
    {
        protected:
            List<Widget*> widgets;
            List<Command> commands;

        private:
            WidgetContainer(const WidgetContainer&);

        public:
            WidgetContainer();
            ~WidgetContainer();

            virtual void Add(Widget* widget);
            //virtual void Bump(Widget *widget);
            void Draw();
            Widget* Find(const char* widgetName);
            void Move(Int2 vec);
            void OnFrame(double delta);
            int OnMouseMove(int h, int x, int y);
            int OnMousePrimary(int h, int x, int y, bool pressed);
            void PushCommand(const Command& cmd) {commands.add(cmd);}
            void Relayout();
            void ReleaseMedia();
            void ReloadMedia();
            //virtual void Remove(Widget *widget);
            void SetArea(Int2 pos, Int2 size);
    };

    class ScrollBarV
    {
        public:
            ScrollBarPainter painter;
        
            Int2 pos, size;
            int y, h;
        
            int ydiff;
        
            bool dragging;
            int yOrig;
        
        public:
            ScrollBarV();
        
            void Draw();
            int OnMouseMove(int h, int x, int y);
            int OnMousePrimary(int h, int x, int y, bool pressed);
        
            int GetWidth() const { return painter.GetScrollbarWidth(); }
    };

    class Button : public Widget
    {
        ButtonPainter painter;

        private:
            Button(const Button&);

        public:
            Button(UIResManager* res, const char* label);
            Button(UIResManager* res, const char* label, Int2 pos, Int2 size);

            virtual void Draw() override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void OnFrame(double delta) override;

            ButtonPainter& GetPainter() { return painter; }
            void SetColour(const Float4& colour) { painter.SetColour(colour); }
            void SetFont(size_t font) { painter.SetFont(font); }

            virtual void ReloadMedia() override;
    };

    class ComboBox : public Widget, public WidgetContainer
    {
        protected:
            ComboBoxPainter painter;
            bool is_open;
            int selection, mouseOver;

            bool rebuildNeeded;
            Int2 contentsSize, padding;
            int iconSize;
            Array<int> rowHeights;

            int GetItemAt(int x, int y);
            void Layout();

        private:
            ComboBox(const ComboBox&);

        public:
            ComboBox(UIResManager* res);

            virtual void Add(Widget* widget) override;
            virtual void Draw() override;
            virtual Int2 GetMaxSize() override;
            virtual Int2 GetMinSize() override;
            virtual void Move(Int2 vec) override;
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void Relayout() override;
            virtual void ReloadMedia() override;
            virtual void SetArea(Int2 pos, Int2 size) override;

            Widget* GetSelectedItem() { return selection >= 0 ? WidgetContainer::widgets[selection] : nullptr; }
    };
    
    class ListBox : public Widget, public WidgetContainer
    {
        protected:
            ListBoxPainter painter;
        
            int spacing;
        
            ScrollBarV scrollbar;

            bool rebuildNeeded;
            int contents_w, contents_h;
            Array<int> rowHeights;

            void Layout();

        private:
            ListBox(const ListBox&);

        public:
            ListBox(UIResManager* res);

            virtual void Add(Widget* widget) override;
            virtual void Draw() override;
            virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            virtual void Move(Int2 vec) override;
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void Relayout() override;
            virtual void ReloadMedia() override;
            virtual void SetArea(Int2 pos, Int2 size) override;
            //virtual void SetSpacing(Int2 spacing) {this->spacing = spacing;}
    };

    class MenuItem : public Widget
    {
        enum {ICON_NONE, ICON_ATLAS, ICON_SCALE};
        
        String label;
        
        int iconMode, iconIndex;
        String iconPath;
        Int2 iconMaxSize;

        Byte4 colour;

        size_t font;
        render::ITexture *tex;
        Float2 iconSize, uv[2];

        bool mouseOver;
        float mouseOverAmt;

        private:
            MenuItem(const MenuItem&);

        public:
            MenuItem(UIResManager* res, const char* label, const char* iconAtlas, int iconIndex);
            MenuItem(UIResManager* res, const char* label);
            virtual ~MenuItem();

            virtual void DrawAt(const Int2& pos) override;
            virtual void OnFrame(double delta) override;
            virtual int OnMouseMove(int h, int x, int y) override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;

            const String& GetLabel() const { return label; }
            void SetColour(const glm::vec4& colour) { this->colour = colour; }
            void SetIconScale(const char* path, CInt2& maxSize);
    };

    class Spacer : public Widget
    {
        private:
            Spacer(const Spacer&);

        public:
            Spacer(UIResManager* res, Int2 size) : Widget(res) { minSize = size; }
    };

    class StaticImage : public Widget
    {
        StaticImagePainter painter;

        private:
            StaticImage(const StaticImage&);

        public:
            StaticImage(UIResManager* res, const char* path);
            StaticImage(UIResManager* res, const char* path, Int2 pos, Int2 size);
            virtual ~StaticImage();

            virtual void Draw() override;
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;

            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;

            void SetColour(const glm::vec4& colour) { painter.SetColour(colour); }
            void SetMapping(ImageMapping mapping) { painter.SetMapping(mapping); }
    };

    class StaticText : public Widget
    {
        StaticTextPainter painter;

        private:
            StaticText(const StaticText&);

        public:
            StaticText(UIResManager* res, const char* label, size_t font = 0);
            StaticText(UIResManager* res, const char* label, Int2 pos, Int2 size, size_t font = 0);

            virtual void DrawAt(const Int2& pos) override;

            virtual void ReloadMedia() override;

            const char* GetLabel() { return painter.GetLabel(); }
            void SetColour(const glm::vec4& colour) { painter.SetColour(colour); }
            void SetFont(size_t font) { painter.SetFont(font); }
            void SetLabel(const char* label) { painter.SetLabel(label); }
    };

    class TableSizer : public Widget, public WidgetContainer
    {
        protected:
            size_t numColumns;
            Int2 spacing;

            bool rebuildNeeded;

            Array<bool> columnsGrowable, rowsGrowable;

            // Calculated by GetMinSize()
            size_t numGrowableColumns, numGrowableRows;
            Array<int> rowHeights, columnWidths;

            void Layout();

        private:
            TableSizer(const TableSizer&);

        public:
            TableSizer(UIResManager* res, size_t numColumns);

            virtual void Add(Widget* widget) override;
            virtual void Draw() override { WidgetContainer::Draw(); }
            virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            virtual void Move(Int2 vec) override;
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override { return WidgetContainer::OnMouseMove(h, x, y); }
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override { return WidgetContainer::OnMousePrimary(h, x, y, pressed); }
            virtual void Relayout() override;
            virtual void ReloadMedia() override;
            virtual void SetArea(Int2 pos, Int2 size) override;
            virtual void SetColumnGrowable( size_t column, bool growable ) { columnsGrowable[column] = growable; rebuildNeeded = true; }
            virtual void SetRowGrowable( size_t row, bool growable ) { rowsGrowable[row] = growable; rebuildNeeded = true; }
            virtual void SetSpacing(Int2 spacing) {this->spacing = spacing;}
    };

    class TextBox : public Widget
    {
        protected:
            TextBoxPainter painter;

            String value;

        private:
            TextBox(const TextBox&);

        public:
            TextBox(UIResManager* res);

            virtual void Draw() override;

            virtual const String& GetValue() const { return value; }
    };

    struct TreeBoxNode {
        String label;

        TreeBoxNode *prev, *children, *children_last, *next;
        //size_t num_children;

        bool open;
        glm::ivec2 pos, size, arrow_pos, arrow_size, label_pos, label_size;
    };

    class TreeBox : public Widget, public WidgetContainer
    {
        protected:
            TreeBoxPainter painter;

            TreeBoxNode *contents, *contents_last, *selected_node;
            int contents_x, contents_y, contents_w, contents_h, scrollbar_x, scrollbar_y;

            bool rebuildNeeded;
            //Array<int> rowHeights;

            void DrawNode(TreeBoxNode* node);
            void Layout();
            void LayoutNode(Int2& pos, TreeBoxNode* node);
            //void MoveNode(Int2& vec, TreeBoxNode* node);
            void OnMousePress(glm::ivec2 mouse, TreeBoxNode *node);

        private:
            TreeBox(const TreeBox&);

        public:
            TreeBox(UIResManager* res);

            virtual TreeBoxNode* Add(const char* label, TreeBoxNode* parent = nullptr);
            virtual void Draw() override;
            TreeBoxNode *GetSelection() { return selected_node; }
            void SetFont(size_t font) { painter.SetFont(font); }

            //virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            //virtual void Move(Int2 vec) override;
            virtual void OnFrame(double delta) override;
            virtual void Relayout() override;
            virtual void ReloadMedia() override;
            virtual void SetArea(Int2 pos, Int2 size) override;
            //virtual void SetSpacing(Int2 spacing) {this->spacing = spacing;}*/
    };

    class Panel : public Widget, public WidgetContainer
    {
        protected:
            PanelPainter painter;

            Int2 padding;
            bool transparent;

            bool minSizeValid;

        private:
            Panel(const Panel&);

        public:
            Panel(UIResManager* res);
            Panel(UIResManager* res, Int2 pos, Int2 size);

            virtual void Add(Widget* widget) override;
            void Layout();

            virtual void Draw() override;
            virtual Widget* Find(const char* widgetName) override { return WidgetContainer::Find(widgetName); }
            virtual Int2 GetMinSize() override;
            bool GetTransparent() const { return transparent; }
            virtual void Move(Int2 vec) override;
            virtual void OnFrame(double delta) override { WidgetContainer::OnFrame(delta); }
            virtual int OnMouseMove(int h, int x, int y) override { return WidgetContainer::OnMouseMove(h, x, y); }
            virtual int OnMousePrimary(int h, int x, int y, bool pressed) override;
            virtual void Relayout() override { WidgetContainer::Relayout(); }
            virtual void ReloadMedia() override { WidgetContainer::ReloadMedia(); }
            virtual void SetArea(Int2 pos, Int2 size) override;
            void SetColour(const glm::vec4& colour) { painter.SetColour(colour); }
            void SetPadding(const Int2 padding) { this->padding = padding; }
            void SetTransparent(bool transparent) { this->transparent = transparent; }
    };

    class Window : public Panel
    {
        protected:
            WindowPainter painter;

            String title;
            int flags;

            Int2 dragFrom;

            enum { DRAG = 64, RESIZE = 128 };

        private:
            Window(const Window&);

        public:
            enum { HEADER = 1, RESIZABLE = 2 };

            Window(UIResManager* res, const char* title, int flags);
            Window(UIResManager* res, const char* title, int flags, Int2 pos, Int2 size);

            virtual void Draw() override;
            virtual void OnFrame(double delta) override;
            virtual void ReloadMedia() override;

            void Close();
            void Focus();
    };

    class Popup : public Window
    {
        public:
            enum PopupEntryFlags {
                POPUP_ENTRY_SELECTABLE = 1,
                POPUP_ENTRY_HAS_ICON = 2
            };
        
            struct PopupEntry
            {
                int flags;
                String label;
            };
        
        protected:
            PopupPainter painter;
        
        private:
            Popup(const Popup&);

        public:
            Popup(UIResManager* res);
            Popup(UIResManager* res, Int2 pos, Int2 size);
            
            virtual void Draw() override;
    };

    class UIContainer : public WidgetContainer
    {
        protected:
            UIResManager* res;
            Int2 pos, size;

            List<Widget*> modalStack;

        private:
            UIContainer(const UIContainer&);

        public:
            UIContainer(UIResManager* res);

            void Draw();
            void OnFrame(double delta);
            int OnMouseMove(int h, int x, int y);
            int OnMousePrimary(int h, int x, int y, bool pressed);
            void PushModal(Widget * modal);
            void SetArea(const Int2& pos, const Int2& size);

            void PushMessageBox(const char* message);
    };
}