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

#include "VectorBaseTheme.h"

#include "../UIContext.h"

#include <nativedraw.h>

namespace uitk {

VectorBaseTheme::VectorBaseTheme(const Params& params, const PicaPt& borderWidth,
                                 const PicaPt& borderRadius)
    : mParams(params), mBorderWidth(borderWidth), mBorderRadius(borderRadius)
{
    // Cannot call setParams() since it is virtual and we are in the constructor
    setVectorParams(params);
}

void VectorBaseTheme::setVectorParams(const Params &params)
{
    mParams = params;

    const int NORMAL = int(WidgetState::kNormal);
    const int DISABLED = int(WidgetState::kDisabled);
    const int OVER = int(WidgetState::kMouseOver);
    const int DOWN = int(WidgetState::kMouseDown);

    // Check text color to determine dark mode; params.windowBackgroundColor
    // may be transparent.
    bool isDarkMode = (params.textColor.toGrey().red() > 0.5f);

    // Normal button
    mButtonStyles[NORMAL].bgColor = params.nonEditableBackgroundColor;
    mButtonStyles[NORMAL].fgColor = params.textColor;
    mButtonStyles[NORMAL].borderColor = params.borderColor;
    mButtonStyles[NORMAL].borderWidth = mBorderWidth;
    mButtonStyles[NORMAL].borderRadius = mBorderRadius;
    mButtonStyles[DISABLED] = mButtonStyles[NORMAL];
    mButtonStyles[DISABLED].bgColor = params.disabledBackgroundColor;
    mButtonStyles[DISABLED].fgColor = params.disabledTextColor;
    mButtonStyles[OVER] = mButtonStyles[NORMAL];
    mButtonStyles[OVER].bgColor = mButtonStyles[NORMAL].bgColor.lighter();
    mButtonStyles[DOWN] = mButtonStyles[NORMAL];
    mButtonStyles[DOWN].bgColor = params.accentColor;
    mButtonStyles[DOWN].fgColor = params.accentedBackgroundTextColor;

    // Button that is ON
    mButtonOnStyles[NORMAL] = mButtonStyles[NORMAL];
    mButtonOnStyles[DISABLED] = mButtonStyles[DISABLED];
    mButtonOnStyles[OVER] = mButtonStyles[OVER];
    mButtonOnStyles[DOWN] = mButtonStyles[DOWN];

    mButtonOnStyles[NORMAL].bgColor = params.accentColor.darker(0.2f);
    mButtonOnStyles[NORMAL].fgColor = params.accentedBackgroundTextColor;
    mButtonOnStyles[DISABLED].bgColor = mButtonStyles[DISABLED].bgColor.blend(params.accentColor, 0.333f);
    mButtonOnStyles[DISABLED].fgColor = params.disabledTextColor;
    mButtonOnStyles[OVER].bgColor = params.accentColor;
    mButtonOnStyles[OVER].fgColor = params.accentedBackgroundTextColor;
    mButtonOnStyles[DOWN].bgColor = params.accentColor.lighter();
    mButtonOnStyles[DOWN].fgColor = params.accentedBackgroundTextColor;

    // Checkbox
    mCheckboxStyles[NORMAL] = mButtonStyles[NORMAL];
    mCheckboxStyles[DISABLED] = mButtonStyles[DISABLED];
    mCheckboxStyles[OVER] = mButtonStyles[OVER];
    mCheckboxStyles[DOWN] = mButtonStyles[DOWN];
    mCheckboxStyles[DOWN].bgColor = mCheckboxStyles[OVER].bgColor;
    if (isDarkMode) {
        mCheckboxStyles[DOWN].bgColor = mCheckboxStyles[OVER].bgColor.lighter(0.2f);
    } else {
        mCheckboxStyles[DOWN].bgColor = mCheckboxStyles[OVER].bgColor.darker(0.2f);
    }

    mCheckboxOnStyles[NORMAL] = mButtonOnStyles[NORMAL];
    mCheckboxOnStyles[NORMAL].bgColor = params.accentColor;
    mCheckboxOnStyles[DISABLED] = mButtonOnStyles[DISABLED];
    mCheckboxOnStyles[OVER] = mButtonOnStyles[OVER];
    if (isDarkMode) {
        mCheckboxOnStyles[OVER].bgColor = params.accentColor.lighter(0.2f);
    } else {
        mCheckboxOnStyles[OVER].bgColor = params.accentColor.darker(0.2f);
    }
    mCheckboxOnStyles[DOWN] = mButtonOnStyles[DOWN];
}

const Theme::Params& VectorBaseTheme::params() const { return mParams; }

void VectorBaseTheme::setParams(const Params& params)
{
    setVectorParams(params);
}

void VectorBaseTheme::drawWindowBackground(UIContext& ui, const Size& size) const
{
    if (mParams.windowBackgroundColor.alpha() > 0.0f) {
        ui.dc.fill(mParams.windowBackgroundColor);
    }
}

void VectorBaseTheme::drawFrame(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style) const
{
    bool hasBG = (style.bgColor.alpha() > 0.0f);
    bool hasBorder = (style.borderWidth > PicaPt(0) && style.borderColor.alpha() > 0.0f);
    if (!hasBG && !hasBorder) {
        return;
    }

    // Lines are stroked along the middle of the path, but we want the
    // border to be completely inside the frame.
    Rect r = frame;
    if (hasBorder) {
        r.x += 0.5f * style.borderWidth;
        r.y += 0.5f * style.borderWidth;
        r.width -= style.borderWidth;
        r.height -= style.borderWidth;
    }

    PaintMode mode;
    if (hasBG) {
        mode = kPaintFill;
        ui.dc.setFillColor(style.bgColor);
    }
    if (hasBorder) {
        if (hasBG) {
            mode = kPaintStrokeAndFill;
        } else {
            mode = kPaintStroke;
        }
        ui.dc.setStrokeWidth(style.borderWidth);
        ui.dc.setStrokeColor(style.borderColor);
    }

    if (style.borderRadius > PicaPt(0)) {
        ui.dc.drawRoundedRect(r, style.borderRadius, mode);
    } else {
        ui.dc.drawRect(r, mode);
    }
}

Size VectorBaseTheme::calcPreferredButtonSize(const LayoutContext& ui, const Font& font,
                                              const std::string& text) const
{
    auto fm = ui.dc.fontMetrics(font);
    auto tm = ui.dc.textMetrics(text.c_str(), font, kPaintFill);
    auto em = tm.height;
    auto margin = ui.dc.ceilToNearestPixel(1.5 * fm.descent);
    // Height works best if the descent is part of the bottom margin,
    // because it looks visually empty even if there are a few descenders.
    // Now the ascent can be anything the font designer want it to be,
    // which is not helpful for computing accurate margins. But cap-height
    // is well-defined, so use that instead.
    return Size(ui.dc.ceilToNearestPixel(2.0f * em + tm.width) + 2.0f * margin,
                ui.dc.ceilToNearestPixel(fm.capHeight) + 2.0f * margin);
}

Size VectorBaseTheme::calcPreferredCheckboxSize(const LayoutContext& ui,
                                                const Font& font) const
{
    auto size = calcPreferredButtonSize(ui, font, "Ag");
    return Size(size.height, size.height);
}

void VectorBaseTheme::drawButton(UIContext& ui, const Rect& frame,
                                 const WidgetStyle& style, WidgetState state,
                                 bool isOn) const
{
    const WidgetStyle *bs;
    if (isOn) {
        bs = &mButtonOnStyles[int(state)];
    } else {
        bs = &mButtonStyles[int(state)];
    }
    drawFrame(ui, frame, bs->merge(style));
}

const Theme::WidgetStyle& VectorBaseTheme::buttonTextStyle(WidgetState state, bool isOn) const
{
    if (isOn) {
        return mButtonOnStyles[int(state)];
    } else {
        return mButtonStyles[int(state)];
    }
}

void VectorBaseTheme::drawCheckbox(UIContext& ui, const Rect& frame,
                                   const WidgetStyle& style, WidgetState state,
                                   bool isOn) const
{
    const WidgetStyle *bs;
    if (isOn) {
        bs = &mCheckboxOnStyles[int(state)];
    } else {
        bs = &mCheckboxStyles[int(state)];
    }
    drawFrame(ui, frame, bs->merge(style));

    if (isOn) {
        auto margin = ui.dc.ceilToNearestPixel(0.25f * frame.width);
        auto thirdW = (frame.width - 2.0f * margin) / 3.0f;
        auto thirdH = (frame.height - 2.0f * margin) / 3.0f;
        ui.dc.save();
        ui.dc.setStrokeColor(bs->fgColor);
        ui.dc.setStrokeWidth(PicaPt(2));
        ui.dc.setStrokeEndCap(kEndCapRound);
        ui.dc.setStrokeJoinStyle(kJoinRound);
        Point p1(frame.x + margin, frame.y + margin + 2.0f * thirdH);
        Point p2(frame.x + margin + thirdW, frame.y + margin + 3.0f * thirdH);
        Point p3(frame.x + margin + 3.0f * thirdW, frame.y + margin);
        ui.dc.drawLines({ p1, p2, p3 });
        ui.dc.restore();
    }
}

}  // namespace uitk
