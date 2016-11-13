
#include "gameui.hpp"

namespace gameui
{
    Popup::Popup(UIResManager* res)
            : Window(res, "", 0), painter(res)
    {
        this->SetExpands(false);
    }
    
    Popup::Popup(UIResManager* res, Int2 pos, Int2 size)
            : Window(res, "", 0, pos, size), painter(res)
    {
    }
    
    void Popup::Draw()
    {
        if (!visible)
            return;
        
        painter.Draw(pos, size);

        WidgetContainer::Draw();
    }
}