//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#include "Length.h"

#include "UIContext.h"
#include "themes/Theme.h"

#include <assert.h>

namespace uitk
{

Length::Length(const PicaPt& pica)
    : mValue(pica), mUnits(Units::kPicaPt)
{
}

Length::Length(const float amount, Units units)
    : mValue(amount), mUnits(units)
{
    assert(units != Units::kPicaPt);
}

PicaPt Length::toPicaPt(const LayoutContext& context,
                        const PicaPt& hundredPercentLength)
{
    return toPicaPt(context.dc, context.theme, hundredPercentLength);
}

PicaPt Length::toPicaPt(const UIContext& context,
                        const PicaPt& hundredPercentLength)
{
    return toPicaPt(context.dc, context.theme, hundredPercentLength);
}

PicaPt Length::toPicaPt(const DrawContext& dc, const Theme& theme,
                        const PicaPt& hundredPercentLength)
{
    switch (mUnits) {
        case Units::kPicaPt:
            return mValue.picaVal;
        case Units::kEm:
            return dc.roundToNearestPixel(mValue.floatVal * theme.params().labelFont.pointSize());
        case Units::kPercent:
            return dc.roundToNearestPixel(mValue.floatVal * hundredPercentLength);
    }
    return PicaPt::kZero;  // some compilers cannot figure out we do not get here
}

}  // namespace uitk
