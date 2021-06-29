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

#include "SliderLogic.h"

#include "Events.h"
#include "UIContext.h"
#include "Window.h"

namespace uitk {

namespace {

static const Point kInvalid(PicaPt(-100000.0f), PicaPt(100000.0f));

class SliderThumb : public Widget
{
    using Super = Widget;
public:
    void draw(UIContext& context) override
    {
        // Do nothing, parent draws. This object exists to make mousing easier.
    }
};

}  // namespace

struct SliderLogic::Impl {
    SliderDir dir;
    // double can represent up to 24-bit ints, which should be enough for
    // a slider. (It's good enough for JavaScript...)
    double value = 0.0;
    double minValue = 0.0;
    double maxValue = 100.0;
    double increment = 1.0;
    std::function<void(SliderLogic*)> onValueChanged;

    SliderThumb *thumb = nullptr;

    Point mouseDownPos = kInvalid;
    Point mouseDownThumbMid;

    double calcValue(const Rect& trackFrame)
    {
        PicaPt trackStart, trackEnd, thumbMid;
        switch (this->dir) {
            case SliderDir::kHoriz:
                trackStart = trackFrame.x + 0.5f * this->thumb->frame().width;
                trackEnd = trackFrame.maxX() - 0.5f * this->thumb->frame().width;
                thumbMid = this->thumb->frame().midX();
                break;
            case SliderDir::kVertZeroAtTop:
                trackStart = trackFrame.y + 0.5f * this->thumb->frame().height;
                trackEnd = trackFrame.maxY() - 0.5f * this->thumb->frame().height;
                thumbMid = this->thumb->frame().midY();
                break;
            case SliderDir::kVertZeroAtBottom:
                trackEnd = trackFrame.y + 0.5f * this->thumb->frame().height;
                trackStart = trackFrame.maxY() - 0.5f * this->thumb->frame().height;
                thumbMid = this->thumb->frame().midY();
                break;
        }
        auto amount = (thumbMid - trackStart) / (trackEnd - trackStart);  // float in [0, 1]
        return this->minValue + double(amount) * (this->maxValue - this->minValue);
    }

    Rect calcThumbFrame(const Rect& trackFrame)
    {
        auto &thumbFrame = this->thumb->frame();
        switch (this->dir) {
            case SliderDir::kHoriz: {
                auto trackStart = trackFrame.x + 0.5f * this->thumb->frame().width;
                auto trackEnd = trackFrame.maxX() - 0.5f * this->thumb->frame().width;
                double amount = (this->value - this->minValue) / (this->maxValue - this->minValue);
                auto x = trackStart + float(amount) * (trackEnd - trackStart);
                return Rect(x - 0.5f * thumbFrame.width, thumbFrame.y, thumbFrame.width, thumbFrame.height);
            }
            case SliderDir::kVertZeroAtTop: {
                auto trackStart = trackFrame.y + 0.5f * this->thumb->frame().height;
                auto trackEnd = trackFrame.maxY() - 0.5f * this->thumb->frame().height;
                double amount = (this->value - this->minValue) / (this->maxValue - this->minValue);
                auto y = trackStart + float(amount) * (trackEnd - trackStart);
                return Rect(thumbFrame.x, y - 0.5f * thumbFrame.height, thumbFrame.width, thumbFrame.height);
            }
            case SliderDir::kVertZeroAtBottom: {
                auto trackEnd = trackFrame.y + 0.5f * this->thumb->frame().height;
                auto trackStart = trackFrame.maxY() - 0.5f * this->thumb->frame().height;
                double amount = (this->value - this->minValue) / (this->maxValue - this->minValue);
                auto y = trackStart + float(amount) * (trackEnd - trackStart);
                return Rect(thumbFrame.x, y - 0.5f * thumbFrame.height, thumbFrame.width, thumbFrame.height);
            }
        }
        // Can't happen, but MSVC apparently doesn't know this.
        return Rect(PicaPt::kZero, PicaPt::kZero, PicaPt::kZero, PicaPt::kZero);
    }
};

SliderLogic::SliderLogic(SliderDir dir)
    : mImpl(new Impl())
{
    mImpl->dir = dir;
    mImpl->thumb = new SliderThumb();
    addChild(mImpl->thumb);
}

SliderLogic::~SliderLogic()
{
    delete removeChild(mImpl->thumb);
    mImpl->thumb = nullptr;
}

SliderDir SliderLogic::direction() const { return mImpl->dir; }

int SliderLogic::intValue() const { return int(mImpl->value); }

SliderLogic* SliderLogic::setValue(int val) {
    setValue(double(val));
    return this;
}

double SliderLogic::doubleValue() const { return mImpl->value; }

SliderLogic* SliderLogic::setValue(double val)
{
    val = mImpl->increment * std::floor(val / mImpl->increment);
    val = std::min(mImpl->maxValue, std::max(mImpl->minValue, val));
    mImpl->value = val;
    mImpl->thumb->setFrame(mImpl->calcThumbFrame(bounds()));
    setNeedsDraw();
    return this;
}

void SliderLogic::setLimits(int minVal, int maxVal, int inc /*= 1*/)
{
    setLimits(double(minVal), double(maxVal), double(std::max(1, inc)));
}

void SliderLogic::setLimits(double minVal, double maxVal, double inc /*= 1.0*/)
{
    if (minVal < maxVal - inc) {
        mImpl->minValue = minVal;
        mImpl->maxValue = maxVal;
        mImpl->increment = std::max(0.0001, inc);
        setValue(mImpl->value);
    }
}

int SliderLogic::intMinLimit() const { return int(mImpl->minValue); }

int SliderLogic::intMaxLimit() const { return int(mImpl->maxValue); }

double SliderLogic::doubleMinLimit() const { return mImpl->minValue; }

double SliderLogic::doubleMaxLimit() const { return mImpl->maxValue; }

SliderLogic* SliderLogic::setOnValueChanged(std::function<void(SliderLogic*)> onChanged)
{
    mImpl->onValueChanged = onChanged;
    return this;
}

Size SliderLogic::preferredSize(const LayoutContext& context) const
{
    auto thumbSize = preferredThumbSize(context);
    return Size(kDimGrow, thumbSize.height);
}

void SliderLogic::layout(const LayoutContext& context)
{
    auto thumbSize = preferredThumbSize(context);
    mImpl->thumb->setFrame(Rect(mImpl->thumb->frame().x, mImpl->thumb->frame().y,
                                thumbSize.width, thumbSize.height));  // set size first
    mImpl->thumb->setFrame(mImpl->calcThumbFrame(bounds()));
    Super::layout(context);
}

Widget::EventResult SliderLogic::mouse(const MouseEvent &e)
{
    Super::mouse(e);

    switch (e.type) {
        case MouseEvent::Type::kButtonDown:
            if (e.button.button != MouseButton::kLeft) {
                return Widget::EventResult::kIgnored;
            }
            window()->setMouseGrab(this);  // take over from thumb
            mImpl->mouseDownPos = e.pos;
            if (!mImpl->thumb->frame().contains(e.pos)) {
                mImpl->mouseDownThumbMid = mImpl->mouseDownPos;
            } else {
                mImpl->mouseDownThumbMid = mImpl->thumb->frame().center();
            }
            break;
        case MouseEvent::Type::kDrag:
            // If dragging with the left mouse or we didn't start the drag in the widget, return
            if ((e.drag.buttons != int(MouseButton::kLeft)) || mImpl->mouseDownPos == kInvalid) {
                return Widget::EventResult::kIgnored;
            }
            break;
        case MouseEvent::Type::kButtonUp:
            if (e.button.button != MouseButton::kLeft) {
                return Widget::EventResult::kIgnored;
            }
            mImpl->mouseDownPos = kInvalid;
            break;
        default:
            return Widget::EventResult::kIgnored;
    }

    if (e.type == MouseEvent::Type::kButtonDown || e.type == MouseEvent::Type::kDrag) {
        if (mImpl->dir == SliderDir::kHoriz) {
            auto x = mImpl->mouseDownThumbMid.x + e.pos.x - mImpl->mouseDownPos.x;
            mImpl->thumb->setFrame(Rect(x - 0.5f * mImpl->thumb->frame().width,
                                        mImpl->thumb->frame().y,
                                        mImpl->thumb->frame().width, mImpl->thumb->frame().height));
        } else {
            auto y = mImpl->mouseDownThumbMid.y + e.pos.y - mImpl->mouseDownPos.y;
            mImpl->thumb->setFrame(Rect(mImpl->thumb->frame().x,
                                        y - 0.5f * mImpl->thumb->frame().height,
                                        mImpl->thumb->frame().width, mImpl->thumb->frame().height));
        }
        auto lastValue = mImpl->value;
        setValue(mImpl->calcValue(bounds()));
        if (lastValue != mImpl->value && mImpl->onValueChanged) {
            mImpl->onValueChanged(this);
        }
    }

    return Widget::EventResult::kConsumed;
}

void SliderLogic::draw(UIContext& context)
{
    drawTrack(context, mImpl->thumb->frame().center());
    drawThumb(context, mImpl->thumb);
    Super::draw(context);
}

}  // namespace uitk

