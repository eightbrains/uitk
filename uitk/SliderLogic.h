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

#ifndef UITK_SLIDER_LOGIC_H
#define UITK_SLIDER_LOGIC_H

#include "Global.h"
#include "Widget.h"

#include <functional>

namespace uitk {

class SliderLogic : public Widget {
    using Super = Widget;
public:
    explicit SliderLogic(SliderDir dir);
    ~SliderLogic();

    SliderDir direction() const;

    int intValue() const;
    SliderLogic* setValue(int val);

    double doubleValue() const;
    SliderLogic* setValue(double val);

    /// Sets the upper, lower, and increment values. Increment must be 1 or larger
    /// for integer sliders.
    SliderLogic* setLimits(int minVal, int maxVal, int inc = 1);

    /// Sets the upper, lower, and increment values. Increment of 0
    /// is continuous (no increment).
    SliderLogic* setLimits(double minVal, double maxVal, double inc = 1.0f);

    int intMinLimit() const;
    int intMaxLimit() const;
    int intIncrement() const;
    double doubleMinLimit() const;
    double doubleMaxLimit() const;
    double doubleIncrement() const;

    /// Increments the control as if the user did it (that is, the on value
    /// changed callback is called);
    void performIncrement();

    /// Deccrements the control as if the user did it (that is, the on value
    /// changed callback is called);
    void performDecrement();

    /// Called when value changes due to mouse movement; is not called
    /// as a result of setValue() or setLimits().
    SliderLogic* setOnValueChanged(std::function<void(SliderLogic*)> onChanged);

    bool acceptsKeyFocus() const override;

    AccessibilityInfo accessibilityInfo() override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    Widget::EventResult mouse(const MouseEvent &e) override;
    Widget::EventResult key(const KeyEvent &e) override;

    void draw(UIContext& context) override;

protected:
    virtual Size preferredThumbSize(const LayoutContext& context) const = 0;
    virtual void drawTrack(UIContext& context, const Point& thumbMid) = 0;
    // Coordinate system is in Slider's coordinate system, not the thumbs'
    // (so thumb->frame() is the correct rectangle to draw into).
    virtual void drawThumb(UIContext& context, Widget *thumb) = 0;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_SLIDER_LOGIC_H


