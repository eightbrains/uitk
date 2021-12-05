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

#ifndef UITK_NUMERIC_MODEL_H
#define UITK_NUMERIC_MODEL_H

#include <functional>
#include <memory>

namespace uitk {

class NumericModel
{
public:
    NumericModel();
    ~NumericModel();

    int intValue() const;
    NumericModel* setValue(int val);

    double doubleValue() const;
    NumericModel* setValue(double val);

    /// Sets the upper, lower, and increment values. Increment must be 1 or
    /// larger for integer sliders. Returns true if min/max changes resulted in
    /// changes to value, false otherwise.
    bool setLimits(int minVal, int maxVal, int inc = 1);

    /// Sets the upper, lower, and increment values. Increment of 0
    /// is continuous (no increment). Returns true if min/max changes resulted in
    /// changes to value, false otherwise.
    bool setLimits(double minVal, double maxVal, double inc = 1.0f);

    int intMinLimit() const;
    int intMaxLimit() const;
    int intIncrement() const;
    double doubleMinLimit() const;
    double doubleMaxLimit() const;
    double doubleIncrement() const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_NUMERIC_MODEL_H
