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

#ifndef UITK_NUMERIC_EDIT_H
#define UITK_NUMERIC_EDIT_H

#include "Widget.h"

#include <functional>

namespace uitk {

class NumberEdit : public Widget
{
    using Super = Widget;
public:
    NumberEdit();
    ~NumberEdit();

    int intValue() const;
    NumberEdit* setValue(int val);

    double doubleValue() const;
    /// Sets the value to the argument truncated to the nearest increment.
    NumberEdit* setValue(double val);

    /// Sets the upper, lower, and increment values. Increment must be 1 or
    /// larger for integer sliders.
    NumberEdit* setLimits(int minVal, int maxVal, int inc = 1);

    /// Sets the upper, lower, and increment values. Increment of 0
    /// is continuous (no increment). The default limits are 0, 100, 1, which
    /// represents an integer range of [0, 100].
    NumberEdit* setLimits(double minVal, double maxVal, double inc = 1.0f);

    int intMinLimit() const;
    int intMaxLimit() const;
    int intIncrement() const;
    double doubleMinLimit() const;
    double doubleMaxLimit() const;
    double doubleIncrement() const;

    int decimalDigits() const;
    /// Sets the number of fractional digits displayed. -1 is equivalent to
    /// "%g" and is the default if the double version of setLimits is called.
    /// It should not normally be necessary to set this, as setLimits() will
    /// set it to 0 for the integer version and -1 for the double version.
    /// The default value is 0.
    NumberEdit* setDecimalDigits(int nDigits);

    /// Increments the control as if the user did it (that is, the on value
    /// changed callback is called);
    void performIncrement();

    /// Deccrements the control as if the user did it (that is, the on value
    /// changed callback is called);
    void performDecrement();
    
    /// Called when value changes due to mouse movement; is not called
    /// as a result of setValue() or setLimits().
    NumberEdit* setOnValueChanged(std::function<void(NumberEdit*)> onChanged);

    AccessibilityInfo accessibilityInfo() override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_NUMERIC_EDIT_H
