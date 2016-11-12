
#include "gameui.hpp"

#include <framework/framework.hpp>

namespace gameui
{
    StaticText::StaticText(UIThemer* themer, const char* label, size_t font)
    {
        thm.reset(themer->CreateStaticTextThemer(pos, size, label));
        thm->SetFont(font);
    }

    StaticText::StaticText(UIThemer* themer, const char* label, Int3 pos, Int2 size, size_t font)
            : Widget(pos, size)
    {
        thm.reset(themer->CreateStaticTextThemer(pos, size, label));
        thm->SetFont(font);
    }
    
    StaticText::~StaticText()
    {
    }
    
    bool StaticText::AcquireResources()
    {
        if (!thm->AcquireResources())
            return false;

        minSize = thm->GetMinSize();
        return true;
    }

    void StaticText::Draw()
    {
        if (!visible)
            return;

        thm->Draw();
    }

    Int2 StaticText::GetMinSize()
    {
        return glm::max(thm->GetMinSize(), minSize);
    }

    void StaticText::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    void StaticText::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);
    }

    void StaticText::SetLabel(const char* label)
    {
        thm->SetLabel(label);

        if (parentContainer != nullptr)
            parentContainer->OnWidgetMinSizeChange(this);
    }
}
