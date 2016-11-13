
#include "gameui.hpp"

#include <superscanf.h>

namespace gameui
{
    static const Flag align_flags[] = {
        {"LEFT",        Widget::LEFT},
        {"TOP",         Widget::TOP},
        {"HCENTER",     Widget::HCENTER},
        {"RIGHT",       Widget::RIGHT},
        {"VCENTER",     Widget::VCENTER},
        {"BOTTOM",      Widget::BOTTOM},
        {nullptr,       0}
    };

    static const Flag window_flags[] = {
        {"HEADER",      Window::HEADER},
        {"RESIZABLE",   Window::RESIZABLE},
        {nullptr,       0}
    };

    static const Flag font_flags[] = {
        {"BOLD",        render::FONT_BOLD},
        {"ITALIC",      render::FONT_ITALIC},
        {nullptr,       0}
    };

    static const Flag image_mappings[] = {
        {"NORMAL",      IMAGEMAP_NORMAL},
        {"HSLICE",      IMAGEMAP_HSLICE},
        {"VSLICE",      IMAGEMAP_VSLICE},
        {nullptr,       0}
    };

    UIResManager::UIResManager()
            : buttonColour(0.3f, 0.3f, 0.3f, 0.7f),
            panelColour(0.0f, 0.0f, 0.0f, 0.7f),
            shadowColour(0.0f, 0.0f, 0.0f, 0.4f),
            textColour(COLOUR_WHITE),
            customLoaderCb(nullptr)
    {
        ui_tex = nullptr;

        // 0 NORMAL
        AddCustomFont("fontcache/gameui_normal", 0, 0);
        // 1 BOLD
        AddCustomFont("fontcache/gameui_bold", 0, 0);
        // 2 RESERVED
        // 3 RESERVED
    }

    UIResManager::~UIResManager()
    {
        ReleaseMedia();

        iterate2 (i, textures)
            i->release();
    }

    size_t UIResManager::AddCustomFont(const char* path, int size, int flags)
    {
        FontEntry entry = { String(), size, flags, nullptr };

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
    }

    void UIResManager::DrawRoundRect(CShort2& a, CShort2& b, CByte4 colour)
    {
        //if (glm::min(b.x - a.x, b.y - a.y) >= 2 * round_rect_r)
        {
            zr::R::SetTexture(nullptr);

            zr::R::DrawFilledRect(Short2(a.x, a.y + round_rect_r),
                    Short2(b.x, b.y - round_rect_r), colour);

            zr::R::DrawFilledRect(Short2(a.x + round_rect_r, a.y),
                    Short2(b.x - round_rect_r, a.y + round_rect_r), colour);

            zr::R::DrawFilledRect(Short2(a.x + round_rect_r, b.y - round_rect_r),
                    Short2(b.x - round_rect_r, b.y), colour);

            zr::R::SetTexture(ui_tex);

            zr::R::DrawFilledRect(a,
                    Short2(a.x + round_rect_r, a.y + round_rect_r),
                    CFloat2(0.0f, 0.0f), CFloat2(16.0f / 128.0f, 16.0f /128.0f), colour);

            zr::R::DrawFilledRect(Short2(a.x, b.y - round_rect_r),
                    Short2(a.x + round_rect_r, b.y),
                    CFloat2(0.0f, 16.0f / 128.0f), CFloat2(16.0f / 128.0f, 0.0f), colour);

            zr::R::DrawFilledRect(Short2(b.x - round_rect_r, b.y - round_rect_r),
                    b,
                    CFloat2(16.0f / 128.0f, 16.0f / 128.0f), CFloat2(0.0f, 0.0f), colour);

            zr::R::DrawFilledRect(Short2(b.x - round_rect_r, a.y),
                    Short2(b.x, a.y + round_rect_r),
                    CFloat2(16.0f / 128.0f, 0.0f), CFloat2(0.0f, 16.0f / 128.0f), colour);
        }
        /*else
        {
            const Int2 visible(b.x - a.x, b.y - a.y);
        }*/
    }

    render::ITexture* UIResManager::GetTexture(const char* path, bool required)
    {
//        String normalPath = Util::NormalizePath(path);

        /*TexEntry* e = textures.find(path);

        if (e != nullptr)
        {
            e->referenceCount++;
            return e->tex;
        }
        else
        {
            render::ITexture* tex = render::Loader::LoadTexture(path, required);

            if (tex == nullptr)
                return nullptr;

            TexEntry entry = { tex, 1 };
            textures.set((String&&) path, (TexEntry&&) entry);

            return tex;
        }*/

        zr::ITexture* tex = textures.get(path);

        if (tex == nullptr)
        {
            tex = render::Loader::LoadTexture(path, required);

            if (tex == nullptr)
                return nullptr;

            textures.set((String&&) String(path), (zr::ITexture*&&) tex);
        }

        return tex->reference();
    }

    Widget* UIResManager::Load(const char* fileName)
    {
        Reference<SeekableInputStream> input = Sys::OpenInput(fileName);

        if (input == nullptr)
            Sys::RaiseException(EX_ASSET_OPEN_ERR, "UIResManager::Load", "Failed to open '%s'.", fileName);

        cfx2::Document doc;
        doc.loadFrom(input);

        if (doc.getNumChildren() > 0)
            return Load(fileName, doc[0]);
        else
            return nullptr;
    }

    size_t UIResManager::Load(const char* fileName, WidgetContainer* container)
    {
        Reference<SeekableInputStream> input = Sys::OpenInput(fileName);

        if (input == nullptr)
            Sys::RaiseException(EX_ASSET_OPEN_ERR, "UIResManager::Load", "Failed to open '%s'.", fileName);

        cfx2::Document doc;
        doc.loadFrom(input);

        iterate2(i, doc)
            container->Add(Load(fileName, i));

        return doc.getNumChildren();
    }

    static int ParseGrowableColumnsCallback(const char* value, const char** end_value, void *user)
    {
        auto sizer = reinterpret_cast<TableSizer*>(user);
        sizer->SetColumnGrowable(strtol(value, (char **) end_value, 0), true);

        return 0;
    }

    static int ParseGrowableRowsCallback(const char* value, const char** end_value, void *user)
    {
        auto sizer = reinterpret_cast<TableSizer*>(user);
        sizer->SetRowGrowable(strtol(value, (char **) end_value, 0), true);

        return 0;
    }

    Widget* UIResManager::Load(const char* fileName, cfx2::Node node)
    {
        String type = node.getName();
        Widget* widget = nullptr;
        WidgetContainer* container = nullptr;

        Int2 pos(0, 0), size(0, 0);

        bool havePos = (Util::ParseIntVec(node.getAttrib("pos"), &pos[0], 2, 2) > 0);
        bool haveSize = (Util::ParseIntVec(node.getAttrib("size"), &size[0], 2, 2) > 0);

        if (type == "Button")
        {
            Button* button = havePos ? new Button(this, node.getAttrib("label"), pos, size)
                : new Button(this, node.getAttrib("label"));

            glm::vec4 colour = COLOUR_BLACK;

            if (Util::ParseVec(node.getAttrib("colour"), &colour[0], 3, 4) > 0)
                button->SetColour(colour);

            int font = ParseFont(node.getAttrib("font"));

            if (font >= 0)
                button->SetFont(font);

            widget = button;
        }
        else if ( type == "ComboBox" )
        {
            ComboBox* comboBox = new ComboBox(this);

            widget = comboBox;
            container = comboBox;
        }
        /*else if ( type == "Input" )
        {
            Input::Type inputType = Input::plain;

            if ( ( String ) node.getAttrib( "type" ) == "password" )
                inputType = Input::password;

            widget = new Input( styler, Vector<>( node.getAttrib( "pos" ) ), Vector<>( node.getAttrib( "size" ) ), inputType );
        }*/
        else if ( type == "ListBox" )
        {
            ListBox* listBox = new ListBox(this);

            widget = listBox;
            container = listBox;
        }
        else if ( type == "MenuItem" )
        {
            widget = new MenuItem(this, node.getAttrib("label"), node.getAttrib("iconAtlas"), node.getAttribInt("iconIndex"));
        }
        else if ( type == "Panel" )
        {
            Panel* panel = havePos ? new Panel(this, pos, size) : new Panel(this);

            glm::vec4 colour = COLOUR_BLACK;
            glm::ivec2 padding;

            if (Util::ParseVec(node.getAttrib("colour"), &colour[0], 3, 4) > 0)
                panel->SetColour(colour);

            if (Util::ParseIntVec(node.getAttrib("padding"), &padding[0], 2, 2) > 0)
                panel->SetPadding(padding);

            panel->SetTransparent(node.getAttribInt("transparent", panel->GetTransparent()) > 0);

            widget = panel;
            container = panel;
        }
        else if ( type == "Popup" )
        {
            Popup* popup = havePos ? new Popup(this, pos, size) : new Popup(this);

            widget = popup;
            container = popup;
        }
        else if ( type == "Spacer" )
        {
            widget = new Spacer(this, size);
        }
        else if ( type == "StaticImage" )
        {
            StaticImage* image = havePos ? new StaticImage(this, node.getAttrib("path"), pos, size)
                : new StaticImage(this, node.getAttrib("path"));

            int mapping;

            if (Util::ParseEnum(node.getAttrib("mapping"), mapping, image_mappings) > 0)
                image->SetMapping((ImageMapping) mapping);

            widget = image;
        }
        else if ( type == "StaticText" )
        {
            StaticText* text = havePos ? new StaticText(this, node.getAttrib("label"), pos, size)
                : new StaticText(this, node.getAttrib("label"));

            int font = ParseFont(node.getAttrib("font"));

            if (font >= 0)
                text->SetFont(font);

            widget = text;
        }
        else if ( type == "TableSizer" )
        {
            TableSizer* sizer = new TableSizer(this, node.getAttribInt("numColumns", 1));

            Int2 spacing;

            Util::ParseVec(node.getAttrib("growableColumns"), ParseGrowableColumnsCallback, sizer);
            Util::ParseVec(node.getAttrib("growableRows"), ParseGrowableRowsCallback, sizer);

            if (Util::ParseIntVec(node.getAttrib("spacing"), &spacing[0], 2, 2) > 0)
                sizer->SetSpacing(spacing);

            widget = sizer;
            container = sizer;
        }
        else if ( type == "TreeBox" )
        {
            TreeBox* treeBox = new TreeBox(this);

            int font = ParseFont(node.getAttrib("font"));

            if (font >= 0)
                treeBox->SetFont(font);

            widget = treeBox;
            container = treeBox;
        }
        else if ( type == "Window" )
        {
            int flags = 0;

            Util::ParseFlagVec(node.getAttrib("flags"), flags, window_flags);

            Window* window = havePos ? new Window(this, node.getAttrib("title"), flags, pos, size)
                : new Window(this, node.getAttrib("title"), flags);

            glm::vec4 colour = COLOUR_BLACK;

            if (Util::ParseVec(node.getAttrib("colour"), &colour[0], 3, 4) > 0)
                window->SetColour(colour);

            widget = window;
            container = window;
        }
        else if (customLoaderCb != nullptr)
        {
            WidgetLoadProperties properties = {pos, size, havePos, haveSize};

            (customLoaderCb)(this, fileName, node, properties, widget, container);
        }

        if (widget == nullptr)
        {
            Sys::DisplayError(SysException(EX_INVALID_ARGUMENT, "gameui::UIResManager::Load", "Unknown widget type '" + type + "' in file '" + fileName + "'"), ERR_DISPLAY_ERROR);
            return nullptr;
        }

        const char* name = node.getText();

        if (name != nullptr)
            widget->SetName(String(name));

        int align = 0;
        glm::ivec2 minSize, maxSize;

        if (Util::ParseFlagVec(node.getAttrib("align"), align, align_flags))
            widget->SetAlign(align);

        widget->SetExpands(node.getAttribInt("expands", widget->GetExpands()) > 0);
        widget->SetFreeFloat(node.getAttribInt("freeFloat", widget->GetFreeFloat()) > 0);
        widget->SetVisible(node.getAttribInt("visible", widget->GetVisible()) > 0);

        if (Util::ParseIntVec(node.getAttrib("minSize"), &minSize[0], 2, 2) >= 0)
            widget->SetMinSize(minSize);

        if (Util::ParseIntVec(node.getAttrib("maxSize"), &maxSize[0], 2, 2) >= 0)
            widget->SetMaxSize(maxSize);

        if (container != nullptr)
        {
            iterate2(i, node)
                container->Add(Load(fileName, i));
        }

        //if (haveArea && !haveSize)
            //widget->(widget->GetMinSize());

        return widget;
    }

    int UIResManager::ParseFont(const char *font)
    {
        if (font != nullptr)
        {
            char font_name[FILENAME_MAX], flags_list[100];
            int font_size;
            int flags = 0;

            int count = ssscanf(font, 0, "%S;%i;%S", font_name, sizeof(font_name), &font_size, flags_list, sizeof(flags_list));

            if (count == 1)
                return AddCustomFont(font_name, -1, 0);
            else
            {
                ZFW_ASSERT(count >= 3)

                Util::ParseFlagVec(flags_list, flags, font_flags);

                return AddCustomFont(font_name, font_size, flags);
            }
        }
        else
            return -1;
    }

    void UIResManager::ReleaseMedia()
    {
        iterate2 (i, fonts)
        {
            FontEntry& entry = i;
            li::destroy(entry.font);
        }

        li::release(ui_tex);
    }

    void UIResManager::ReleaseTexture(render::ITexture *tex)
    {
        li::release(tex);

        /*if (tex == nullptr)
            return;

        TexEntry* e = textures.find(tex->GetFileName());

        if (e != nullptr)
        {
            ZFW_ASSERT(e->tex == tex)

            if (--e->referenceCount == 0)
            {
                ZFW_ASSERT(textures.unset(tex->GetFileName()))
                li::release(tex);
            }
        }*/
    }

    void UIResManager::ReloadMedia()
    {
        ui_tex = render::Loader::LoadTexture("media/gameui/ui_tex.png", true);

        iterate2 (i, fonts)
        {
            FontEntry& entry = i;

            //if (entry.flags != 0)
            //    Sys::RaiseException(EX_INVALID_ARGUMENT, "UIResManager::ReloadMedia", "Unsupported flag %d for font `%s` @ %d", entry.flags, entry.path.c_str(), entry.size);

            if (entry.size > 0)
                entry.font = zr::Font::Open(entry.path, entry.size);
            else
                entry.font = zr::Font::Open(entry.path);
        }
    }
}
