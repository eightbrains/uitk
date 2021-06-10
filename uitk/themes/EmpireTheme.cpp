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

#include "EmpireTheme.h"

namespace uitk {

Theme::Params EmpireTheme::defaultParams()
{
    Theme::Params params;
    params.nonEditableBackgroundColor = Color(0.4f, 0.4f, 0.4f);
    params.editableBackgroundColor = Color(0.1f, 0.1f, 0.1f);
    params.disabledBackgroundColor = Color(0.3f, 0.3f, 0.3f);
    params.borderColor = Color(0.2f, 0.2f, 0.2f);
    params.textColor = Color(0.875f, 0.875f, 0.875f);
    params.accentedBackgroundTextColor = params.textColor;
    params.disabledTextColor = Color(0.6f, 0.6f, 0.6f);
    params.accentColor = Color(0.22f, 0.45f, 0.90f);
    params.selectionColor = Color(0.11f, 0.23f, 0.45f);
    params.labelFont = Font("Arial", PicaPt::fromPixels(12.0f, 72.0f));
    return params;
}

EmpireTheme::EmpireTheme()
    : EmpireTheme(defaultParams())
{
}

EmpireTheme::EmpireTheme(const Params& params)
    : VectorBaseTheme(params,
                      PicaPt::fromPixels(0.5, 72),  // borderWidth
                      PicaPt(4)                     // borderRadius
                     )
{
}


}  // namespace uitk
