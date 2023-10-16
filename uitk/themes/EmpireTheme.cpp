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

namespace {
static const float kBorderWidthStdPx = 0.5f;
static const float kBorderRadiusStdPx = 3.0f;
}

Theme::Params EmpireTheme::defaultParams()
{
    return EmpireTheme::darkModeParams(Color(0.22f, 0.45f, 0.90f));
}

Theme::Params EmpireTheme::EmpireTheme::darkModeParams(const Color& accent)
{
    bool accentIsDark = (accent.toGrey().red() < 0.5f);

    Theme::Params params;
    params.windowBackgroundColor = Color(0.176f, 0.176f, 0.176f);
    params.nonEditableBackgroundColor = Color(0.4f, 0.4f, 0.4f);
    params.editableBackgroundColor = Color(0.4f, 0.4f, 0.4f);
    params.disabledBackgroundColor = Color(0.3f, 0.3f, 0.3f);
    params.borderColor = Color(1.0f, 1.0f, 1.0f, 0.2f);
    params.borderWidth = PicaPt::fromStandardPixels(kBorderWidthStdPx);
    params.borderRadius = PicaPt::fromStandardPixels(kBorderRadiusStdPx); 
    params.textColor = Color(0.875f, 0.875f, 0.875f);
    if (accentIsDark) {
        params.accentedBackgroundTextColor = params.textColor;
    } else {
        params.accentedBackgroundTextColor = Color(0.0f, 0.0f, 0.0f);
    }
    params.disabledTextColor = Color(0.6f, 0.6f, 0.6f);
    params.accentColor = accent;
    params.keyFocusColor = Color(accent, 0.5f);
    //params.selectionColor = Color(0.11f, 0.23f, 0.45f);
    params.selectionColor = accent.darker();
    params.splitterColor = Color::kBlack;
    params.nonNativeMenuSeparatorColor = params.disabledTextColor;
    params.nonNativeMenuBackgroundColor = Color(0.225f, 0.225f, 0.225f);
    params.nonNativeMenubarBackgroundColor = Color(0.275f, 0.275f, 0.275f);
    params.labelFont = Font("Arial", PicaPt::fromPixels(10.0f, 96.0f));  // Linux/Win defaults to 96 dpi
    params.nonNativeMenubarFont = params.labelFont;
    params.useClearTextButton = false;  // not really appropriate for desktops
    params.useClearTextButtonForSearch = true;  // varies, but seems typical
    return params;
}

Theme::Params EmpireTheme::lightModeParams(const Color& accent)
{
    bool accentIsDark = (accent.toGrey().red() < 0.5f);

    Theme::Params params;
    params.windowBackgroundColor = Color(1.0f, 1.0f, 1.0f);
    params.nonEditableBackgroundColor = Color(0.975f, 0.975f, 0.975f);
    params.editableBackgroundColor = Color(1.0f, 1.0f, 1.0f);
    params.disabledBackgroundColor = Color(0.85f, 0.85f, 0.85f);
    params.borderColor = Color(0.0f, 0.0f, 0.0f, 0.2f);
    params.borderWidth = PicaPt::fromStandardPixels(kBorderWidthStdPx);
    params.borderRadius = PicaPt::fromStandardPixels(kBorderRadiusStdPx); 
    params.textColor = Color(0.1f, 0.1f, 0.1f);
    if (accentIsDark) {
        params.accentedBackgroundTextColor = Color(1.0f, 1.0f, 1.0f);
    } else {
        params.accentedBackgroundTextColor = params.textColor;
    }
    params.disabledTextColor = Color(0.4f, 0.4f, 0.4f);
    params.accentColor = accent;
    params.keyFocusColor = Color(accent, 0.5f);
    params.selectionColor = accent.lighter();
    params.splitterColor = Color(0.870f, 0.870f, 0.870f);
    params.nonNativeMenuSeparatorColor = Color(0.75f, 0.75f, 0.75f);
    params.nonNativeMenuBackgroundColor = Color(0.975f, 0.975f, 0.975f);
    params.nonNativeMenubarBackgroundColor = Color(1.0f, 1.0f, 1.0f);
    params.labelFont = Font("Arial", PicaPt::fromPixels(10.0f, 96.0f));  // Linux/Win defaults to 96 dpi
    params.nonNativeMenubarFont = params.labelFont;
    params.useClearTextButton = false;  // not really appropriate for desktops
    params.useClearTextButtonForSearch = true;  // varies, but seems typical
    return params;
}

Theme::Params EmpireTheme::customParams(const Color& bgColor,
                                        const Color& fgColor,
                                        const Color& accent)
{
    auto bgGrey = bgColor.toGrey().red();
    auto fgGrey = fgColor.toGrey().red();
    auto contrast = std::abs(bgGrey - fgGrey);
    bool accentIsDark = (accent.toGrey().red() < 0.5f);

    Theme::Params params;
    params.windowBackgroundColor = bgColor;
    // The button (and similar) backgrounds should be lighter,
    // except if the background is near white, since they cannot
    // really get whiter in that case.
    if (bgGrey >= 0.9975f) {
        params.nonEditableBackgroundColor = bgColor.darker();
    } else if (bgGrey  >= 0.8f) {
        params.nonEditableBackgroundColor = Color::kWhite;
    } else {
        params.nonEditableBackgroundColor = bgColor.lighter(0.2f);
    }
    // Normally we want to lighten the text editing backgrounds,
    // but darken if lightening reduces the contrast with the
    // text too much (such as with a window background that is a
    // saturated color that needs light text, but there isn't
    // much contrast to begin with). Also, since blending 50% is
    // less visible if the bgColor is already close to white,
    // just set to white at a certain point.
    if (bgGrey > 0.9f) {
        params.editableBackgroundColor = Color::kWhite;
    } else {
        params.editableBackgroundColor = bgColor.blend(Color::kWhite, 0.5f);
    if (std::abs(fgGrey - params.editableBackgroundColor.toGrey().red()) < 0.5f) {
        params.editableBackgroundColor = bgColor.blend(Color::kBlack, 0.5f);
        }
    }
    params.disabledBackgroundColor = params.nonEditableBackgroundColor.blend(fgColor, 0.1667);
    params.borderColor = fgColor.colorWithAlpha(0.2f);
    params.borderWidth = PicaPt::fromStandardPixels(kBorderWidthStdPx);
    params.borderRadius = PicaPt::fromStandardPixels(kBorderRadiusStdPx); 
    params.textColor = fgColor;
    if (accentIsDark) {
        params.accentedBackgroundTextColor = Color(1.0f, 1.0f, 1.0f);
    } else {
        params.accentedBackgroundTextColor = params.textColor;
    }
    params.disabledTextColor = fgColor.blend(bgColor, 0.333f);
    params.accentColor = accent;
    params.keyFocusColor = Color(accent, 0.5f);
    params.selectionColor = accent.lighter();
    params.splitterColor = bgColor.blend(fgColor, 0.15f);
    params.nonNativeMenuSeparatorColor = bgColor.blend(fgColor, 0.2f);
    Color menuBlendColor = fgColor;
    if (contrast <= 0.6f) {
        menuBlendColor = Color::kWhite;
        if (fgGrey >= 0.5f) {  // go away from fg for more contrast
            menuBlendColor = Color::kBlack;
        }
    }
    params.nonNativeMenuBackgroundColor = bgColor.blend(menuBlendColor, 0.05f);
    params.nonNativeMenubarBackgroundColor = bgColor.blend(menuBlendColor, 0.1f);
    params.labelFont = Font("Arial", PicaPt::fromPixels(10.0f, 96.0f));  // Linux/Win defaults to 96 dpi
    params.nonNativeMenubarFont = params.labelFont;
    params.useClearTextButton = false;  // not really appropriate for desktops
    params.useClearTextButtonForSearch = true;  // varies, but seems typical
    return params;
}

EmpireTheme::EmpireTheme()
    : EmpireTheme(defaultParams())
{
}

EmpireTheme::EmpireTheme(const Params& params)
    : VectorBaseTheme(params)
{
}

}  // namespace uitk
