
#include <gameui/gameui.hpp>

#include <framework/event.hpp>
#include <framework/framework.hpp>

namespace gameui
{
    Slider::Slider(UIThemer* themer)
    {
        thm.reset(themer->CreateSliderThemer());

        value = 0.0f;
        SetRange(0.0f, 1.0f, 0.0f);
        snap = SNAP_NONE;

        mouseDown = false;
    }

    Slider::~Slider()
    {
    }

    void Slider::Draw()
    {
        thm->Draw(enabled, value);
    }

    Int2 Slider::GetMinSize()
    {
        return glm::max(thm->GetMinSize(), minSize);
    }

    void Slider::Layout()
    {
        Widget::Layout();
        thm->SetArea(pos, size);
    }

    void Slider::Move(Int3 vec)
    {
        Widget::Move(vec);
        thm->SetArea(pos, size);
    }

    int Slider::OnMouseMove(int h, int x, int y)
    {
        if (mouseDown)
        {
            ZFW_ASSERT(size.x > 0)

            const float t = (x - pos.x) / (float) size.x;            
            SetValue(min + t * (max - min), true);

            return 0;
        }

        return h;
    }

    int Slider::OnMousePrimary(int h, int x, int y, bool pressed)
    {
        if (h < 0 && visible && enabled && IsInsideMe(x, y))
        {
            mouseDown = pressed;
            return 0;
        }
        else
            mouseDown = false;

        return h;
    }

    void Slider::SetRange(float min, float max, float tickStep)
    {
        this->min = min;
        this->max = max;
        this->tick = tickStep;
        value = glm::clamp(value, min, max);

        thm->SetRange(min, max, tickStep);
    }

    void Slider::SetValue(float value, bool fireEvents)
    {
        const float oldValue = this->value;

        float valueInRangeSpace = value - min;

        if (snap == SNAP_ALWAYS)
            valueInRangeSpace = glm::round(valueInRangeSpace / tick) * tick;

        value = glm::clamp(min + valueInRangeSpace, min, max);

        if (fireEvents && fabs(oldValue - value) > 1.0e-4f)
            FireValueChangeEvent((float) value);

        this->value = value;
    }
}