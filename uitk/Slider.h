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

#ifndef UITK_SLIDER_H
#define UITK_SLIDER_H

#include "Widget.h"

namespace uitk {

class Slider : public Widget {
    using Super = Widget;
public:
    Slider();
    ~Slider();

    int intValue() const;
    Slider* setValue(int val);

    double doubleValue() const;
    Slider* setValue(double val);

    /// Sets the upper, lower, and increment values. Increment must be 1 or larger
    /// for integer sliders.
    void setLimits(int minVal, int maxVal, int inc = 1);

    /// Sets the upper, lower, and increment values. Increment of 0
    /// is continuous (no increment).
    void setLimits(double minVal, double maxVal, double inc = 1.0f);

    /// Called when value changes due to mouse movement; is not called
    /// as a result of setValue() or setLimits().
    Slider* setOnValueChanged(std::function<void(Slider*)> onChanged);

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    Widget::EventResult mouse(const MouseEvent &e) override;

    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_SLIDER_H
