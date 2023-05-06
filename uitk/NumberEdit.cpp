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

#include "NumberEdit.h"

#include "IncDecWidget.h"
#include "NumericModel.h"
#include "StringEdit.h"
#include "UIContext.h"
#include "themes/Theme.h"

#include <cmath>
#include <limits>

namespace uitk {

namespace {
std::string convertDoubleToString(double x, int nFormatDigits)
{
    char formatStr[16] = "%g";
    char numberStr[64];
    // If x is really large, force using %g, otherwise we could overflow
    // our 64 bytes of string (DBL_MAX is 1.8e308), and also at some
    // point showing all the digits is not helpful. Also, check that some
    // hotshot didn't specifically set nFormatDigits to something huge.
    if (nFormatDigits >= 0 && nFormatDigits < sizeof(numberStr) - 1 && std::abs(x) < 1.e13) {
        snprintf(formatStr, sizeof(formatStr), "%%.%df", nFormatDigits);
    }
    snprintf(numberStr, sizeof(numberStr), formatStr, x);
    return numberStr;
}

}  // namespace

struct NumberEdit::Impl
{
    NumericModel model;
    int nFormatDigits = 0;
    bool userHasSetFormatDigits = false;
    StringEdit *stringEdit;  // owned by Super as a child
    IncDecWidget *incDec;    // owned by Super as a child
    std::function<void(NumberEdit*)> onChanged;

    double lastDrawnValue = std::numeric_limits<double>::quiet_NaN();
};

NumberEdit::NumberEdit()
    : mImpl(new Impl())
{
    mImpl->stringEdit = new StringEdit();
    mImpl->stringEdit->setAlignment(Alignment::kRight | Alignment::kVCenter);
    mImpl->stringEdit->setUseClearButton(StringEdit::UseClearButton::kNo);
    mImpl->stringEdit->setShowFocusRingOnParent(true);
    mImpl->stringEdit->setOnValueChanged([this](StringEdit *se) {
        // Force resetting the text, in case a float is entered but not enough format digits
        // to display what was entered.
        mImpl->lastDrawnValue = std::numeric_limits<double>::quiet_NaN();

        double newVal = std::atof(se->text().c_str());
        if (!std::isnan(newVal)) {
            mImpl->model.setValue(newVal);
            if (mImpl->onChanged) {
                mImpl->onChanged(this);
            }
        }
    });
    addChild(mImpl->stringEdit);

    mImpl->incDec = new IncDecWidget();
    mImpl->incDec->setOnClicked([this](IncDecWidget*, int dir) {
        if (dir > 0) {
            mImpl->model.setValue(mImpl->model.doubleValue() + mImpl->model.doubleIncrement());
        } else if (dir < 0) {
            mImpl->model.setValue(mImpl->model.doubleValue() - mImpl->model.doubleIncrement());
        }
        if (dir != 0 && mImpl->onChanged) {
            mImpl->onChanged(this);
        }
    });
    addChild(mImpl->incDec);
}

NumberEdit::~NumberEdit()
{
}

int NumberEdit::intValue() const { return mImpl->model.intValue(); }

NumberEdit* NumberEdit::setValue(int val)
{
    return setValue(double(val));
}

double NumberEdit::doubleValue() const { return mImpl->model.doubleValue(); }

NumberEdit* NumberEdit::setValue(double val)
{
    mImpl->model.setValue(val);
    setNeedsDraw();
    return this;
}

NumberEdit* NumberEdit::setLimits(int minVal, int maxVal, int inc /*= 1*/)
{
    setLimits(double(minVal), double(maxVal), double(inc));
    if (!mImpl->userHasSetFormatDigits) {
        mImpl->nFormatDigits = 0;
    }
    return this;
}

NumberEdit* NumberEdit::setLimits(double minVal, double maxVal, double inc /*= 1.0f*/)
{
    if (mImpl->model.setLimits(minVal, maxVal, inc)) {
        setNeedsDraw();
    }
    if (!mImpl->userHasSetFormatDigits) {
        if (inc >= 1.0f) {
            mImpl->nFormatDigits = -1;
        } else {
            // This finds the smallest piece that will change, so inc = 0.01 and 0.21
            // are equvalent as far as number of digits to display is concerned.
            double remainder = inc;
            mImpl->nFormatDigits = 1;
            while (mImpl->nFormatDigits < 1e6) {
                auto thisDigit = std::floor(remainder * 10.0);
                remainder = remainder * 10.0 - thisDigit;
                // Handle floating point imprecision. We might be just a little over zero
                // or just a little under one (for example, (double)0.01f = 0.0099999...,
                // so passing a float value for 'inc' to this function can trigger this)
                if (remainder < 1e-6 || remainder > 0.999999) {
                    break;
                }
                mImpl->nFormatDigits += 1;
            }
        }
    }
    return this;
}

int NumberEdit::intMinLimit() const { return mImpl->model.intMinLimit(); }

int NumberEdit::intMaxLimit() const { return mImpl->model.intMaxLimit(); }

int NumberEdit::intIncrement() const { return mImpl->model.intIncrement(); }

double NumberEdit::doubleMinLimit() const { return mImpl->model.doubleMinLimit(); }

double NumberEdit::doubleMaxLimit() const { return mImpl->model.doubleMaxLimit(); }

double NumberEdit::doubleIncrement() const { return mImpl->model.doubleIncrement(); }

int NumberEdit::decimalDigits() const { return mImpl->nFormatDigits; }

NumberEdit* NumberEdit::setDecimalDigits(int nDigits)
{
    mImpl->nFormatDigits = nDigits;
    mImpl->userHasSetFormatDigits = true;
    setNeedsDraw();
    return this;
}

void NumberEdit::performIncrement()
{
    if (double(int(mImpl->model.doubleIncrement())) == mImpl->model.doubleIncrement()) {
        setValue(intValue() + intIncrement());
    } else {
        setValue(doubleValue() + doubleIncrement());
    }
    if (mImpl->onChanged) {
        mImpl->onChanged(this);
    }
}

void NumberEdit::performDecrement()
{
    if (double(int(mImpl->model.doubleIncrement())) == mImpl->model.doubleIncrement()) {
        setValue(intValue() - intIncrement());
    } else {
        setValue(doubleValue() - doubleIncrement());
    }
    if (mImpl->onChanged) {
        mImpl->onChanged(this);
    }
}

NumberEdit* NumberEdit::setOnValueChanged(std::function<void(NumberEdit*)> onChanged)
{
    mImpl->onChanged = onChanged;
    return this;
}

AccessibilityInfo NumberEdit::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    auto textInfo = mImpl->stringEdit->accessibilityInfo();
    auto incDecInfo = mImpl->incDec->accessibilityInfo();
    incDecInfo.type = AccessibilityInfo::Type::kIncDec;
    if (double(int(mImpl->model.doubleIncrement())) == mImpl->model.doubleIncrement()) {
        info.value = mImpl->model.intValue();
    } else {
        info.value = mImpl->model.doubleValue();
    }
    incDecInfo.performIncrementNumeric = [this]() { performIncrement(); };
    incDecInfo.performDecrementNumeric = [this]() { performDecrement(); };
    info.children = { textInfo, incDecInfo };
    return info;
}

Size NumberEdit::preferredSize(const LayoutContext& context) const
{
    double minVal = mImpl->model.doubleMinLimit();
    double maxVal = mImpl->model.doubleMaxLimit();
    double longestValue = std::max(std::abs(minVal), maxVal);
    if (std::abs(minVal) > maxVal) {
        longestValue = -longestValue;
    }
    // "1" may size smaller than other digits, but a max of 1.xxx may display 0.xxx,
    // which will then get cut off a bit. So if the first digit is one, add one to it
    // to make it "2", which should size okay.
    auto firstDigit = longestValue / std::pow(10.0, std::floor(std::log10(longestValue)));
    if (firstDigit >= 1.0 && firstDigit < 2.0) {
        longestValue += std::pow(10.0, std::floor(std::log10(longestValue)));
    }

    auto longestText = convertDoubleToString(longestValue, mImpl->nFormatDigits);
    auto textWidth = context.dc.textMetrics(longestText.c_str(), context.theme.params().labelFont).width;

    auto prefIncDec = context.theme.calcPreferredIncDecSize(context.dc);
    return Size(textWidth + prefIncDec.height, prefIncDec.height);
}

void NumberEdit::layout(const LayoutContext& context)
{
    auto prefIncDec = context.theme.calcPreferredIncDecSize(context.dc);
    auto r = bounds();
    auto spacing = context.dc.ceilToNearestPixel(0.1f * prefIncDec.width);
    Rect stringRect(r.x, r.y, r.width - prefIncDec.width - spacing, r.height);
    Rect incDecRect(r.maxX() - prefIncDec.width, r.y, prefIncDec.width, r.height);

    mImpl->stringEdit->setFrame(stringRect);
    mImpl->incDec->setFrame(incDecRect);

    Super::layout(context);
}

void NumberEdit::draw(UIContext& context)
{
    if (mImpl->lastDrawnValue != mImpl->model.doubleValue()) {
        std::string text;
        if (mImpl->nFormatDigits == 0) {
            text = convertDoubleToString(mImpl->model.intValue(), 0);
        } else {
            text = convertDoubleToString(mImpl->model.doubleValue(), mImpl->nFormatDigits);
        }
        mImpl->stringEdit->setText(text);
        mImpl->lastDrawnValue = mImpl->model.doubleValue();
    }
    Super::draw(context);
}

}  // namespace uitk
