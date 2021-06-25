//-----------------------------------------------------------------------------
// Copyright 2021 Eight Brains Studios, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "Slider.h"

#include "Events.h"
#include "UIContext.h"
#include "Window.h"
#include "themes/Theme.h"

namespace uitk {

namespace {

static const PicaPt kInvalid(-10000);

class SliderThumb : public Widget
{
    using Super = Widget;
public:
    void draw(UIContext& context)
    {
        Super::draw(context);
        context.theme.drawSliderThumb(context, bounds(), style(state()), state());
    }
};

}  // namespace

struct Slider::Impl {
    // double can represent up to 24-bit ints, which should be enough for
    // a slider. (It's good enough for JavaScript...)
    double value = 0.0;
    double minValue = 0.0;
    double maxValue = 100.0;
    double increment = 1.0;
    std::function<void(Slider*)> onValueChanged;

    SliderThumb *thumb = nullptr;

    PicaPt mouseDownX = kInvalid;
    PicaPt mouseDownThumbMidX;

    double calcValue(const Rect& trackFrame)
    {
        auto trackStart = trackFrame.x + 0.5f * this->thumb->frame().width;
        auto trackEnd = trackFrame.maxX() - 0.5f * this->thumb->frame().width;
        auto amount = (this->thumb->frame().midX() - trackStart) / (trackEnd - trackStart);  // float in [0, 1]
        return this->minValue + double(amount) * (this->maxValue - this->minValue);
    }

    Rect calcThumbFrame(const Rect& trackFrame)
    {
        auto &thumbFrame = this->thumb->frame();
        auto trackStart = trackFrame.x + 0.5f * thumbFrame.width;
        auto trackEnd = trackFrame.maxX() - 0.5f * thumbFrame.width;
        double amount = (this->value - this->minValue) / (this->maxValue - this->minValue);
        auto x = trackStart + float(amount) * (trackEnd - trackStart);
        return Rect(x - 0.5f * thumbFrame.width, thumbFrame.y, thumbFrame.width, thumbFrame.height);
    }
};

Slider::Slider()
    : mImpl(new Impl())
{
    mImpl->thumb = new SliderThumb();
    addChild(mImpl->thumb);
}

Slider::~Slider()
{
    delete removeChild(mImpl->thumb);
    mImpl->thumb = nullptr;
}

int Slider::intValue() const { return int(mImpl->value); }

Slider* Slider::setValue(int val) {
    setValue(double(val));
}

double Slider::doubleValue() const { return mImpl->value; }

Slider* Slider::setValue(double val)
{
    val = mImpl->increment * std::floor(val / mImpl->increment);
    val = std::min(mImpl->maxValue, std::max(mImpl->minValue, val));
    mImpl->value = val;
    mImpl->thumb->setFrame(mImpl->calcThumbFrame(bounds()));
    setNeedsDraw();
}

void Slider::setLimits(int minVal, int maxVal, int inc /*= 1*/)
{
    setLimits(double(minVal), double(maxVal), double(std::max(1, inc)));
}

void Slider::setLimits(double minVal, double maxVal, double inc /*= 1.0*/)
{
    if (minVal < maxVal - inc) {
        mImpl->minValue = minVal;
        mImpl->maxValue = maxVal;
        mImpl->increment = std::max(0.0001, inc);
        setValue(mImpl->value);
    }
}

Slider* Slider::setOnValueChanged(std::function<void(Slider*)> onChanged)
{
    mImpl->onValueChanged = onChanged;
    return this;
}

Size Slider::preferredSize(const LayoutContext& context) const
{
    auto thumbSize = context.theme.calcPreferredSliderThumbSize(context.dc);
    return Size(kDimGrow, thumbSize.height);
}

void Slider::layout(const LayoutContext& context)
{
    auto thumbSize = context.theme.calcPreferredSliderThumbSize(context.dc);
    mImpl->thumb->setFrame(Rect(mImpl->thumb->frame().x, mImpl->thumb->frame().y,
                                thumbSize.width, thumbSize.height));  // set size first
    mImpl->thumb->setFrame(mImpl->calcThumbFrame(bounds()));
    Super::layout(context);
}

Widget::EventResult Slider::mouse(const MouseEvent &e)
{
    Super::mouse(e);

    switch (e.type) {
        case MouseEvent::Type::kButtonDown:
            if (e.button.button != MouseButton::kLeft) {
                return;
            }
            window()->setMouseGrab(this);  // take over from thumb
            mImpl->mouseDownX = e.pos.x;
            if (!mImpl->thumb->frame().contains(e.pos)) {
                mImpl->mouseDownThumbMidX = mImpl->mouseDownX;
            } else {
                mImpl->mouseDownThumbMidX = mImpl->thumb->frame().midX();
            }
            break;
        case MouseEvent::Type::kDrag:
            // If dragging with the left mouse or we didn't start the drag in the widget, return
            if ((e.drag.buttons != int(MouseButton::kLeft)) || mImpl->mouseDownX == kInvalid) {
                return;
            }
            break;
        case MouseEvent::Type::kButtonUp:
            if (e.button.button != MouseButton::kLeft) {
                return;
            }
            mImpl->mouseDownX = kInvalid;
            break;
        default:
            return;
    }

    if (e.type == MouseEvent::Type::kButtonDown || e.type == MouseEvent::Type::kDrag) {
        auto x = mImpl->mouseDownThumbMidX + e.pos.x - mImpl->mouseDownX;
        mImpl->thumb->setFrame(Rect(x - 0.5f * mImpl->thumb->frame().width,
                                    mImpl->thumb->frame().y,
                                    mImpl->thumb->frame().width, mImpl->thumb->frame().height));
        auto lastValue = mImpl->value;
        setValue(mImpl->calcValue(bounds()));
        if (lastValue != mImpl->value && mImpl->onValueChanged) {
            mImpl->onValueChanged(this);
        }

    }
}

void Slider::draw(UIContext& context)
{
    context.theme.drawSliderTrack(context, bounds(), mImpl->thumb->frame().midX(), style(state()), state());
    Super::draw(context);
}

}  // namespace uitk

