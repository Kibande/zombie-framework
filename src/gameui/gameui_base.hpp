#pragma once

#include <framework/base.hpp>
#include <framework/datamodel.hpp>

#include <littl/cfx2.hpp>

namespace gameui
{
    using namespace zfw;

    // Forward declarations

    class MenuBar;
    class TreeBox;
    class UILoader;
    class Widget;
    class WidgetContainer;

    struct MenuItem_t;
    struct TreeItem_t;
    struct WidgetLoadProperties;

    // Typedefs

    typedef char32_t UnicodeChar;

    enum ImageMapping {
        IMAGEMAP_NORMAL,
        IMAGEMAP_HSLICE,
        IMAGEMAP_VSLICE
    };

    enum
    {
        MENU_ITEM_SELECTABLE = 1
    };

    enum SnapMode_t
    {
        SNAP_NONE = 0,
        SNAP_NEAR = 1,
        SNAP_ALWAYS = 2
    };

    struct WidgetLoadProperties
    {
        enum { POS_VALID = 1, SIZE_VALID = 2 };

        int flags;
        Int3 pos;
        Int2 size;
    };

    // Messages from gameui Widgets ($3000..37FF)
    enum {
        GAMEUI_DUMMY_MSG = (0x3000 - 1),

        // In

        // Out
        EVENT_CONTROL_USED,
        EVENT_ITEM_SELECTED,
        EVENT_LOOP_EVENT,
        EVENT_MENU_ITEM_SELECTED,
        EVENT_MOUSE_DOWN,
        EVENT_TREE_ITEM_SELECTED,
        EVENT_VALUE_CHANGED,

        // Private

        GAMEUI_MAX_MSG
    };

    DECL_MESSAGE(EventControlUsed, EVENT_CONTROL_USED)
    {
        Widget* widget;
    };

    DECL_MESSAGE(EventItemSelected, EVENT_ITEM_SELECTED)
    {
        Widget* widget;
        int itemIndex;
    };

    DECL_MESSAGE(EventLoopEvent, EVENT_LOOP_EVENT)
    {
        enum Type
        {
            FOCUS_WIDGET_IN_CONTAINER,
            POP_MODAL,
            REMOVE_WIDGET_FROM_CONTAINER,
        };

        enum
        {
            FLAG_RELEASE = 1
        };

        Type type;
        int flags;

        Widget* widget;
        WidgetContainer* container;
    };

    DECL_MESSAGE(EventMenuItemSelected, EVENT_MENU_ITEM_SELECTED)
    {
        MenuBar* menuBar;
        MenuItem_t* item;
    };

    DECL_MESSAGE(EventMouseDown, EVENT_MOUSE_DOWN)
    {
        Widget* widget;
        int button, x, y;
    };

    DECL_MESSAGE(EventTreeItemSelected, EVENT_TREE_ITEM_SELECTED)
    {
        TreeBox* treeBox;
        TreeItem_t* item;
    };

    DECL_MESSAGE(EventValueChanged, EVENT_VALUE_CHANGED)
    {
        Widget* widget;
        int value;
        float floatValue;
    };

    // Callbacks

    typedef void (*ClickEventHandler)(Widget* widget, int x, int y);
    typedef void (*MouseDownEventHandler)(Widget* widget, int button, int x, int y);

    typedef void (*UILoaderCallback)(UILoader* ldr, const char* fileName, cfx2::Node node, const WidgetLoadProperties& properties, Widget*& widget, WidgetContainer*& container);
}
