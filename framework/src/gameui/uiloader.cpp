
#include <gameui/gameui.hpp>

#include <framework/colorconstants.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/system.hpp>

namespace gameui
{
    using namespace li;

    static const Flag align_flags[] = {
        {"LEFT",        ALIGN_LEFT},
        {"TOP",         ALIGN_TOP},
        {"HCENTER",     ALIGN_HCENTER},
        {"RIGHT",       ALIGN_RIGHT},
        {"VCENTER",     ALIGN_VCENTER},
        {"BOTTOM",      ALIGN_BOTTOM},
        {nullptr,       0}
    };

    static const Flag window_flags[] = {
        //{"NO_HEADER",   Window::NO_HEADER},
        {"RESIZABLE",   Window::RESIZABLE},
        {nullptr,       0}
    };

    static const Flag font_flags[] = {
        {"BOLD",        FONT_BOLD},
        {"ITALIC",      FONT_ITALIC},
        {nullptr,       0}
    };

    static const Flag image_mappings[] = {
        {"NORMAL",      IMAGEMAP_NORMAL},
        {"HSLICE",      IMAGEMAP_HSLICE},
        {"VSLICE",      IMAGEMAP_VSLICE},
        {nullptr,       0}
    };

    static int ParseGrowableColumnsCallback(const char* value, const char** end_value, void *user)
    {
        auto sizer = reinterpret_cast<TableLayout*>(user);
        sizer->SetColumnGrowable(strtol(value, (char **) end_value, 0), true);

        return 0;
    }

    static int ParseGrowableRowsCallback(const char* value, const char** end_value, void *user)
    {
        auto sizer = reinterpret_cast<TableLayout*>(user);
        sizer->SetRowGrowable(strtol(value, (char **) end_value, 0), true);

        return 0;
    }

    static void ParseMenuBarSubMenu(MenuBar* menuBar, MenuItem_t* parent, cfx2::Node node)
    {
        for (auto itemNode : node)
        {
            if (strcmp(itemNode.getName(), "Item") != 0)
                continue;

            const char* name = itemNode.getText();

            const int flags = (name != nullptr) ? MENU_ITEM_SELECTABLE : 0;

            MenuItem_t* item = menuBar->Add(parent, itemNode.getAttrib("label"), name, flags);

            if (itemNode.getNumChildren() > 0)
                ParseMenuBarSubMenu(menuBar, item, itemNode);
        }
    }

    UILoader::UILoader(ISystem* sys, UIContainer* uiBase, UIThemer* themer)
            : sys(sys), uiBase(uiBase), themer(themer)
    {
        customLoaderCb = nullptr;
        defaultFont = 0;
    }

    /*Widget* UILoader::Load(const char* fileName)
    {
        Reference<InputStream> input = Sys::OpenInput(fileName);

        if (input == nullptr)
            return ErrorBuffer::SetError(eb, EX_ASSET_OPEN_ERR,
                    "desc", (const char*) sprintf_t<255>("Failed to open user interface file '%s'.", fileName),
                    "function", li_functionName,
                    nullptr),
                    nullptr;

        cfx2::Document doc;
        doc.loadFrom(input);

        if (doc.getNumChildren() > 0)
            return Load(fileName, doc[0]);
        else
            return ErrorBuffer::SetError(eb, EX_NO_ERROR, nullptr), nullptr;
    }*/

    bool UILoader::Load(const char* fileName, WidgetContainer* container, bool acquireResources)
    {
        unique_ptr<InputStream> input(sys->OpenInput(fileName));

        if (input == nullptr)
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", (const char*) sprintf_t<255>("Failed to open user interface file '%s'.", fileName),
                    "function", li_functionName
                    ), false;

        cfx2::Document doc;
        doc.loadFrom(input.get());

        for (auto i : doc)
        {
            Widget* child = Load(fileName, i, acquireResources);

            if (child == nullptr)
                return false;

            container->Add(child);
        }

        return true;
    }

    Widget* UILoader::Load(const char* fileName, cfx2::Node node, bool acquireResources)
    {
        String type = node.getName();
        Widget* widget = nullptr;
        WidgetContainer* container = nullptr;

        Int3 pos(0, 0, 0);
        Int2 size(0, 0);

        bool havePos = (Util::ParseIntVec(node.getAttrib("pos"), &pos[0], 2, 3) > 0);
        bool haveSize = (Util::ParseIntVec(node.getAttrib("size"), &size[0], 2, 2) > 0);

        if (type == "Button")
        {
            Button* button = havePos ? new Button(themer, node.getAttrib("label"), pos, size)
                : new Button(themer, node.getAttrib("label"));

            /*glm::vec4 colour = COLOUR_BLACK;

            if (Util::ParseVec(node.getAttrib("colour"), &colour[0], 3, 4) > 0)
                button->SetColour(colour);*/

            intptr_t fontid = GetFontId(node.getAttrib("font"));
            button->SetFont((fontid >= 0) ? fontid : defaultFont);

            widget = button;
        }
        else if ( type == "CheckBox" )
        {
            CheckBox* checkBox = havePos ? new CheckBox(themer, node.getAttrib("label"), node.getAttribInt("checked") > 0, pos, size)
                : new CheckBox(themer, node.getAttrib("label"), node.getAttribInt("checked") > 0);

            intptr_t fontid = GetFontId(node.getAttrib("font"));
            checkBox->SetFont((fontid >= 0) ? fontid : defaultFont);

            widget = checkBox;
        }
        else if ( type == "ComboBox" )
        {
            ComboBox* comboBox = new ComboBox(themer, uiBase);

            widget = comboBox;
        }
        /*else if ( type == "Input" )
        {
            Input::Type inputType = Input::plain;

            if ( ( String ) node.getAttrib( "type" ) == "password" )
                inputType = Input::password;

            widget = new Input( styler, Vector<>( node.getAttrib( "pos" ) ), Vector<>( node.getAttrib( "size" ) ), inputType );
        }*/
        else if ( type == "ListCtrl" )
        {
            ListCtrl* listCtrl= new ListCtrl(themer);

            widget = listCtrl;
            container = listCtrl;
        }
        else if ( type == "MenuBar" )
        {
            MenuBar* menuBar = new MenuBar(themer);

            //int font = ParseFont(node.getAttrib("font"));

            //if (font >= 0)
            //    treeBox->SetFont(font);

            ParseMenuBarSubMenu(menuBar, nullptr, node);

            widget = menuBar;
        }
        /*else if ( type == "MenuItem" )
        {
            widget = new MenuItem(this, node.getAttrib("label"), node.getAttrib("iconAtlas"), node.getAttribInt("iconIndex"));
        }*/
        else if ( type == "Panel" )
        {
            Panel* panel = havePos ? new Panel(themer, pos, size) : new Panel(themer);

            Float4 colour = COLOUR_BLACK;
            Int2 padding;

            if (Util::ParseVec(node.getAttrib("colour"), &colour[0], 3, 4) > 0)
                panel->SetColour(colour);

            if (Util::ParseIntVec(node.getAttrib("padding"), &padding[0], 2, 2) > 0)
                panel->SetPadding(padding);

            //panel->SetTransparent(node.getAttribInt("transparent", panel->GetTransparent()) > 0);

            widget = panel;
            container = panel;
        }
        /*else if ( type == "Popup" )
        {
            Popup* popup = havePos ? new Popup(themer, pos, size) : new Popup(themer);

            widget = popup;
            container = popup;
        }*/
        else if (type == "Slider")
        {
            Slider* slider = new Slider(themer);

            Float2 minMax;

            if (Util::ParseVec(node.getAttrib("minMax"), &minMax[0], 2, 2) > 0)
                slider->SetRange(minMax.x, minMax.y, 0.0f);

            widget = slider;
        }
        else if ( type == "Spacer" )
        {
            widget = new Spacer(themer, size);
        }
        else if ( type == "StaticImage" )
        {
            StaticImage* image = new StaticImage(themer, node.getAttrib("path"));

            //int mapping;

            //if (Util::ParseEnum(node.getAttrib("mapping"), mapping, image_mappings) > 0)
            //    image->SetMapping((ImageMapping) mapping);

            widget = image;
        }
        else if ( type == "StaticText" )
        {
            StaticText* text = havePos ? new StaticText(themer, node.getAttrib("label"), pos, size)
                : new StaticText(themer, node.getAttrib("label"));

            intptr_t fontid = GetFontId(node.getAttrib("font"));
            text->SetFont((fontid >= 0) ? fontid : defaultFont);

            widget = text;
        }
        else if ( type == "TableLayout" || type == "Table" )
        {
            TableLayout* table = new TableLayout(node.getAttribInt("numColumns", 1));

            Int2 spacing;

            Util::ParseVec(node.getAttrib("growableColumns"), ParseGrowableColumnsCallback, table);
            Util::ParseVec(node.getAttrib("growableRows"), ParseGrowableRowsCallback, table);

            if (Util::ParseIntVec(node.getAttrib("spacing"), &spacing[0], 2, 2) > 0)
                table->SetSpacing(spacing);

            widget = table;
            container = table;
        }
        else if ( type == "TextBox" )
        {
            TextBox* textBox = new TextBox(themer);

            //int font = ParseFont(node.getAttrib("font"));

            //if (font >= 0)
            //    treeBox->SetFont(font);

            textBox->SetText(node.getAttrib("text"));

            widget = textBox;
        }
        else if ( type == "TreeBox" )
        {
            TreeBox* treeBox = new TreeBox(themer);

            //int font = ParseFont(node.getAttrib("font"));

            //if (font >= 0)
            //    treeBox->SetFont(font);

            widget = treeBox;
        }
        else if ( type == "Window" )
        {
            int flags = 0;

            Util::ParseFlagVec(node.getAttrib("flags"), flags, window_flags);

            Window* window = havePos ? new Window(themer, node.getAttrib("title"), flags, pos, size)
                : new Window(themer, node.getAttrib("title"), flags);

            glm::vec4 colour = COLOUR_BLACK;

            if (Util::ParseVec(node.getAttrib("colour"), &colour[0], 3, 4) > 0)
                window->SetColour(colour);

            widget = window;
            container = window;
        }
        else if (customLoaderCb != nullptr)
        {
            const int flags = (havePos ? WidgetLoadProperties::POS_VALID : 0)
                    | (haveSize ? WidgetLoadProperties::SIZE_VALID : 0);

            WidgetLoadProperties properties = {flags, pos, size};

            (customLoaderCb)(this, fileName, node, properties, widget, container);
        }

        if (widget == nullptr)
            return ErrorBuffer::SetError3(EX_OBJECT_UNDEFINED, 3,
                    "desc", sprintf_255("Unknown UI widget type '%s'. Incompatible engine version?", type.c_str()),
                    "path", fileName,
                    "function", li_functionName
                    ), nullptr;

        widget->SetOnlineUpdate(false);

        const char* name = node.getText();

        if (name != nullptr)
            widget->SetName(String(name));

        int align = 0;
        glm::ivec2 minSize, maxSize;

        if (Util::ParseFlagVec(node.getAttrib("align"), align, align_flags))
            widget->SetAlign(align);

        widget->SetEnabled(node.getAttribInt("enabled", widget->GetEnabled()) > 0, false);
        widget->SetExpands(node.getAttribInt("expands", widget->GetExpands()) > 0);
        widget->SetFreeFloat(node.getAttribInt("freeFloat", widget->GetFreeFloat()) > 0);
        widget->SetVisible(node.getAttribInt("visible", widget->GetVisible()) > 0);

        if (Util::ParseIntVec(node.getAttrib("minSize"), &minSize[0], 2, 2) >= 0)
            widget->SetMinSize(minSize);

        if (Util::ParseIntVec(node.getAttrib("maxSize"), &maxSize[0], 2, 2) >= 0)
            widget->SetMaxSize(maxSize);

        if (acquireResources && !widget->AcquireResources())
            return nullptr;

        if (container != nullptr)
        {
            for (auto i : node)
            {
                Widget* child = Load(fileName, i, acquireResources);

                if (child == nullptr)
                {
                    delete widget;
                    return nullptr;
                }

                container->Add(child);
            }
        }

        //if (haveArea && !haveSize)
            //widget->(widget->GetMinSize());

        widget->SetOnlineUpdate(true);
        return widget;
    }

    intptr_t UILoader::GetFontId(const char* fontOrNull)
    {
        if (fontOrNull != nullptr)
            return themer->GetFontId(fontOrNull);
        else
            return -1;
    }
}
