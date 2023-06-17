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

#ifndef UITK_LENGTH_H
#define UITK_LENGTH_H

#include <nativedraw.h>

namespace uitk
{

struct LayoutContext;
struct UIContext;
class Theme;

class Length
{
public:
    enum class Units {
        kPicaPt = 0,
        kEm = 1,
        kPercent = 2
    };

public:
    Length(const PicaPt& pica);  // NOT explict (can auto-convert)!
    /// Note that Length(fVal, Units::kPicaPt) is invalid!
    Length(const float amount, Units units);

    PicaPt toPicaPt(const LayoutContext& context,
                    const PicaPt& hundredPercentLength);
    PicaPt toPicaPt(const UIContext& context,
                    const PicaPt& hundredPercentLength);
    PicaPt toPicaPt(const DrawContext& dc, const Theme& theme,
                    const PicaPt& hundredPercentLength);

private:
    // Not using PImpl pattern since
    //  a) help with cache locality in objects (avoid yet another pointer
    //     dereference)
    //  b) this class is not likely to change frequently
    union Value {
        // PicaPt has no default value, so we need to provide constructors
        Value(float f) : floatVal(f) {};
        Value(const PicaPt& p) : picaVal(p) {};

        float floatVal;
        PicaPt picaVal;
    } mValue;
    Units mUnits;
};

}  // namespace uitk
#endif // UITK_LENGTH_H
