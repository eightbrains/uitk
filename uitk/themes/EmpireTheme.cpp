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
    return EmpireTheme::darkModeParams(Color(0.22f, 0.45f, 0.90f));
}

Theme::Params EmpireTheme::EmpireTheme::darkModeParams(const Color& accent)
{
    Theme::Params params;
    params.windowBackgroundColor = Color(0.176f, 0.176f, 0.176f);
    params.nonEditableBackgroundColor = Color(0.4f, 0.4f, 0.4f);
    params.editableBackgroundColor = Color(0.1f, 0.1f, 0.1f);
    params.disabledBackgroundColor = Color(0.3f, 0.3f, 0.3f);
    params.textColor = Color(0.875f, 0.875f, 0.875f);
    params.accentedBackgroundTextColor = params.textColor;
    params.disabledTextColor = Color(0.6f, 0.6f, 0.6f);
    params.accentColor = accent;
    //params.selectionColor = Color(0.11f, 0.23f, 0.45f);
    params.selectionColor = accent.darker();
    params.nonNativeMenuSeparatorColor = params.disabledTextColor;
    params.nonNativeMenuBackgroundColor = Color(0.225f, 0.225f, 0.225f);
    params.nonNativeMenubarBackgroundColor = Color(0.275f, 0.275f, 0.275f);
    params.labelFont = Font("Arial", PicaPt::fromPixels(10.0f, 96.0f));  // Linux/Win defaults to 96 dpi
    params.nonNativeMenubarFont = params.labelFont;
    return params;
}

Theme::Params EmpireTheme::lightModeParams(const Color& accent)
{
    Theme::Params params;
    params.windowBackgroundColor = Color(1.0f, 1.0f, 1.0f);
    params.nonEditableBackgroundColor = Color(0.975f, 0.975f, 0.975f);
    params.editableBackgroundColor = Color(1.0f, 1.0f, 1.0f);
    params.disabledBackgroundColor = Color(0.85f, 0.85f, 0.85f);
    params.textColor = Color(0.1f, 0.1f, 0.1f);
    params.accentedBackgroundTextColor = Color(1.0f, 1.0f, 1.0f);
    params.disabledTextColor = Color(0.4f, 0.4f, 0.4f);
    params.accentColor = accent;
    params.selectionColor = accent.lighter();
    params.nonNativeMenuSeparatorColor = Color(0.75f, 0.75f, 0.75f);
    params.nonNativeMenuBackgroundColor = Color(0.975f, 0.975f, 0.975f);
    params.nonNativeMenubarBackgroundColor = Color(1.0f, 1.0f, 1.0f);
    params.labelFont = Font("Arial", PicaPt::fromPixels(10.0f, 96.0f));  // Linux/Win defaults to 96 dpi
    params.nonNativeMenubarFont = params.labelFont;
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
