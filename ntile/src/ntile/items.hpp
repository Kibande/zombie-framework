#pragma once

#include <cstdlib>

namespace ntile
{
    enum ItemClass
    {
        ITEM_EQUIPMENT,
        ITEM_KEY
    };

    struct Item_t
    {
        ItemClass cls;
        int itemid;
    };

    class ItemContainer
    {
        protected:
            size_t capacity;
            Item_t* items;

        public:

    };
}
