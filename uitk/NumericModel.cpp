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

#include "NumericModel.h"

#include <algorithm>
#include <cmath>

namespace uitk {

struct NumericModel::Impl
{
    // double is 64-bits, which gives 53-bits for the coefficient (and 11 bits
    // for the exponent). 53-bits of integer, which is a little under 1e16
    // should be enough for any usable slider. (It's good enough for JavaScript
    // integer values...)
    double value = 0.0;
    double minValue = 0.0;
    double maxValue = 100.0;
    double increment = 1.0;
    std::function<void(NumericModel*)> onValueChanged;
};

NumericModel::NumericModel()
    : mImpl(new Impl())
{
}

NumericModel::~NumericModel()
{
}

int NumericModel::intValue() const { return int(mImpl->value); }

NumericModel* NumericModel::setValue(int val) {
    setValue(double(val));
    return this;
}

double NumericModel::doubleValue() const { return mImpl->value; }

NumericModel* NumericModel::setValue(double val)
{
    // Use round(), not floor(), this seems to work better.
    // In particular, 0.01 with an increment of 0.01 gets floor()ed
    // down to 0.0, but 0.01 is not representable exactly, so it is 0.1 - epsilon,
    // while might have a couple conversions to/from double/float, so
    // it is not one increment's worth. This is bad.
    val = mImpl->increment * std::round(val / mImpl->increment);
    val = std::min(mImpl->maxValue, std::max(mImpl->minValue, val));
    mImpl->value = val;
    return this;
}

bool NumericModel::setLimits(int minVal, int maxVal, int inc /*= 1*/)
{
    return setLimits(double(minVal), double(maxVal), double(std::max(1, inc)));
}

bool NumericModel::setLimits(double minVal, double maxVal, double inc /*= 1.0*/)
{
    if (minVal < maxVal - inc) {
        mImpl->minValue = minVal;
        mImpl->maxValue = maxVal;
        mImpl->increment = std::max(0.000001, inc);
        setValue(mImpl->value);
        return true;
    }
    return false;
}

int NumericModel::intMinLimit() const { return int(mImpl->minValue); }

int NumericModel::intMaxLimit() const { return int(mImpl->maxValue); }

int NumericModel::intIncrement() const { return int(mImpl->increment); }

double NumericModel::doubleMinLimit() const { return mImpl->minValue; }

double NumericModel::doubleMaxLimit() const { return mImpl->maxValue; }

double NumericModel::doubleIncrement() const { return mImpl->increment; }

}  // namespace uitk
