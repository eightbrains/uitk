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

#include "../Application.h"
#include "../TextEditorLogic.h"
#include "../UIContext.h"
#include "../Widget.h"

#include <nativedraw.h>

namespace uitk {

Color blend(const Color& top, const Color& bottom)
{
    auto a = top.alpha();
    return Color(a * top.red() + (1.0f - a) * bottom.red(),
                 a * top.green() + (1.0f - a) * bottom.green(),
                 a * top.green() + (1.0f - a) * bottom.blue(),
                 1.0f);
}

bool calcIsDarkMode(const Theme::Params& params)
{
    // Check text color to determine dark mode; params.windowBackgroundColor
    // may be transparent.
    return (params.textColor.toGrey().red() > 0.5f);
}

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
    const int SELECTED = int(WidgetState::kSelected);

    auto copyStyles = [NORMAL, DISABLED, OVER, DOWN, SELECTED]
                      (const WidgetStyle src[], WidgetStyle dest[]) {
        dest[NORMAL] = src[NORMAL];
        dest[DISABLED] = src[DISABLED];
        dest[OVER] = src[OVER];
        dest[DOWN] = src[DOWN];
        dest[SELECTED] = src[SELECTED];
    };

    bool isDarkMode = calcIsDarkMode(params);
    Color borderColor = (isDarkMode ? Color(1.0f, 1.0f, 1.0f, 0.2f)
                                    : Color(0.0f, 0.0f, 0.0f, 0.2f));

    // Labels
    mLabelStyles[NORMAL].bgColor = Color::kTransparent;
    mLabelStyles[NORMAL].fgColor = params.textColor;
    mLabelStyles[NORMAL].borderColor = Color::kTransparent;
    mLabelStyles[NORMAL].borderWidth = PicaPt::kZero;
    mLabelStyles[NORMAL].borderRadius = PicaPt::kZero;
    mLabelStyles[DISABLED] = mLabelStyles[NORMAL];
    mLabelStyles[DISABLED].fgColor = params.disabledTextColor;
    mLabelStyles[OVER] = mLabelStyles[NORMAL];
    mLabelStyles[DOWN] = mLabelStyles[NORMAL];
    mLabelStyles[SELECTED] = mLabelStyles[NORMAL];
    mLabelStyles[SELECTED].bgColor = Color::kTransparent;  // owner will draw bg; selection area might be larger than label
    mLabelStyles[SELECTED].fgColor = params.accentedBackgroundTextColor;

    // Normal button
    mButtonStyles[NORMAL].bgColor = params.nonEditableBackgroundColor;
    mButtonStyles[NORMAL].fgColor = params.textColor;
    mButtonStyles[NORMAL].borderColor = params.nonEditableBackgroundColor.darker(0.2f);
    mButtonStyles[NORMAL].borderWidth = mBorderWidth;
    mButtonStyles[NORMAL].borderRadius = mBorderRadius;
    mButtonStyles[DISABLED] = mButtonStyles[NORMAL];
    mButtonStyles[DISABLED].bgColor = params.disabledBackgroundColor;
    mButtonStyles[DISABLED].fgColor = params.disabledTextColor;
    mButtonStyles[OVER] = mButtonStyles[NORMAL];
    if (isDarkMode) {
        mButtonStyles[OVER].bgColor = mButtonStyles[NORMAL].bgColor.lighter();
    } else {
        mButtonStyles[OVER].bgColor = mButtonStyles[NORMAL].bgColor.darker(0.025f);
    }
    mButtonStyles[DOWN] = mButtonStyles[NORMAL];
    mButtonStyles[DOWN].bgColor = params.accentColor;
    mButtonStyles[DOWN].fgColor = params.accentedBackgroundTextColor;
    mButtonStyles[SELECTED] = mButtonStyles[DOWN];  // not applicable, but mButtonStyles is copied for many styles

    // Button that is ON
    copyStyles(mButtonStyles, mButtonOnStyles);
    mButtonOnStyles[NORMAL].bgColor = params.accentColor.darker(0.2f);
    mButtonOnStyles[NORMAL].fgColor = params.accentedBackgroundTextColor;
    mButtonOnStyles[DISABLED].bgColor = mButtonStyles[DISABLED].bgColor.blend(params.accentColor, 0.333f);
    mButtonOnStyles[DISABLED].fgColor = params.disabledTextColor;
    mButtonOnStyles[OVER].bgColor = params.accentColor;
    mButtonOnStyles[OVER].fgColor = params.accentedBackgroundTextColor;
    mButtonOnStyles[DOWN].bgColor = params.accentColor.lighter();
    mButtonOnStyles[DOWN].fgColor = params.accentedBackgroundTextColor;
    mButtonOnStyles[SELECTED] = mButtonOnStyles[DOWN];

    // Checkbox
    copyStyles(mButtonStyles, mCheckboxStyles);
    if (isDarkMode) {
        mCheckboxStyles[DOWN].bgColor = mCheckboxStyles[OVER].bgColor.lighter(0.1f);
    } else {
        mCheckboxStyles[DOWN].bgColor = mCheckboxStyles[OVER].bgColor.darker(0.1f);
    }

    copyStyles(mButtonOnStyles, mCheckboxOnStyles);
    mCheckboxOnStyles[NORMAL].bgColor = params.accentColor;
    if (isDarkMode) {
        mCheckboxOnStyles[OVER].bgColor = params.accentColor.lighter(0.05f);
        mCheckboxOnStyles[DOWN].bgColor = params.accentColor.lighter(0.15f);
    } else {
        mCheckboxOnStyles[OVER].bgColor = params.accentColor.darker(0.05f);
        mCheckboxOnStyles[DOWN].bgColor = params.accentColor.darker(0.15f);
    }

    // SegmentedControl (background)
    copyStyles(mButtonStyles, mSegmentedControlStyles);  // only NORMAL, DISABLED matter

    // SegmentedControl active segment (button)
    // Note: OVER and DOWN are used for button segments
    copyStyles(mButtonStyles, mSegmentStyles);
    // On macOS the colors can have alpha. This is normally okay, but because we
    // draw the segments on top of the background the alpha gets applied twice,
    // which results in the value being brighter than the equivalent for buttons.
    // We need to adjust the alpha values to be the equivalent of if we could
    // draw the segment directly on top of the background
    auto adjustSegmentBG = [widgetBG = mSegmentStyles[NORMAL].bgColor](const Color& segmentBG) {
        if (segmentBG.alpha() < 1.0f) {
            // If segmentBG is color, I don't think you can simply adjust the alpha,
            // but it should look reasonably good.
            float greyWidget = widgetBG.toGrey().red();
            float greySegment = segmentBG.toGrey().red();
            float widget = greyWidget * widgetBG.alpha();
            float desired = greySegment * segmentBG.alpha();
            // Simplify: desired = (1 - a) * widget + a * greySegment
            float alpha = (desired - widget) / (greySegment - widget);
            return Color(segmentBG.red(), segmentBG.green(), segmentBG.blue(), alpha);
        }
        return segmentBG;
    };
    mSegmentStyles[NORMAL].borderRadius = PicaPt::kZero;
    mSegmentStyles[NORMAL].borderWidth = PicaPt::kZero;
    mSegmentStyles[DISABLED].borderRadius = PicaPt::kZero;
    mSegmentStyles[DISABLED].borderWidth = PicaPt::kZero;
    mSegmentStyles[OVER].bgColor = adjustSegmentBG(mSegmentStyles[OVER].bgColor);
    mSegmentStyles[OVER].borderRadius = PicaPt::kZero;
    mSegmentStyles[OVER].borderWidth = PicaPt::kZero;
    mSegmentStyles[DOWN].bgColor = adjustSegmentBG(mSegmentStyles[DOWN].bgColor);
    mSegmentStyles[DOWN].borderRadius = PicaPt::kZero;
    mSegmentStyles[DOWN].borderWidth = PicaPt::kZero;

    // SegmentedControl active segment (toggleable)
    copyStyles(mCheckboxStyles, mSegmentOffStyles);
    mSegmentOffStyles[NORMAL].bgColor = adjustSegmentBG(mSegmentOffStyles[NORMAL].bgColor);
    mSegmentOffStyles[NORMAL].borderRadius = PicaPt::kZero;
    mSegmentOffStyles[NORMAL].borderWidth = PicaPt::kZero;
    mSegmentOffStyles[DISABLED].bgColor = adjustSegmentBG(mSegmentOffStyles[DISABLED].bgColor);
    mSegmentOffStyles[DISABLED].borderRadius = PicaPt::kZero;
    mSegmentOffStyles[DISABLED].borderWidth = PicaPt::kZero;
    mSegmentOffStyles[OVER].bgColor = adjustSegmentBG(mSegmentOffStyles[OVER].bgColor);
    mSegmentOffStyles[OVER].borderRadius = PicaPt::kZero;
    mSegmentOffStyles[OVER].borderWidth = PicaPt::kZero;
    mSegmentOffStyles[DOWN].bgColor = adjustSegmentBG(mSegmentOffStyles[DOWN].bgColor);
    mSegmentOffStyles[DOWN].borderRadius = PicaPt::kZero;
    mSegmentOffStyles[DOWN].borderWidth = PicaPt::kZero;
    copyStyles(mCheckboxOnStyles, mSegmentOnStyles);
    mSegmentOnStyles[NORMAL].bgColor = adjustSegmentBG(mSegmentOnStyles[NORMAL].bgColor);
    mSegmentOnStyles[NORMAL].borderRadius = PicaPt::kZero;
    mSegmentOnStyles[NORMAL].borderWidth = PicaPt::kZero;
    mSegmentOnStyles[DISABLED].bgColor = adjustSegmentBG(mSegmentOnStyles[DISABLED].bgColor);
    mSegmentOnStyles[DISABLED].borderRadius = PicaPt::kZero;
    mSegmentOnStyles[DISABLED].borderWidth = PicaPt::kZero;
    mSegmentOnStyles[OVER].bgColor = adjustSegmentBG(mSegmentOnStyles[OVER].bgColor);
    mSegmentOnStyles[OVER].borderRadius = PicaPt::kZero;
    mSegmentOnStyles[OVER].borderWidth = PicaPt::kZero;
    mSegmentOnStyles[DOWN].bgColor = adjustSegmentBG(mSegmentOnStyles[DOWN].bgColor);
    mSegmentOnStyles[DOWN].borderRadius = PicaPt::kZero;
    mSegmentOnStyles[DOWN].borderWidth = PicaPt::kZero;

    // ComboBox
    copyStyles(mButtonStyles, mComboBoxStyles);
    mComboBoxStyles[DOWN] = mComboBoxStyles[OVER];
    mComboBoxIconAreaStyles[NORMAL].bgColor = params.accentColor;
    mComboBoxIconAreaStyles[NORMAL].fgColor = params.accentedBackgroundTextColor;
    mComboBoxIconAreaStyles[NORMAL].borderColor = Color::kTransparent;
    mComboBoxIconAreaStyles[NORMAL].borderWidth = PicaPt::kZero;
    mComboBoxIconAreaStyles[DISABLED] = mComboBoxIconAreaStyles[NORMAL];
    mComboBoxIconAreaStyles[DISABLED].bgColor = Color::kTransparent;
    mComboBoxIconAreaStyles[DISABLED].fgColor = mComboBoxStyles[DISABLED].fgColor;
    mComboBoxIconAreaStyles[OVER] = mComboBoxIconAreaStyles[NORMAL];
    mComboBoxIconAreaStyles[OVER].bgColor = mCheckboxOnStyles[OVER].bgColor;
    mComboBoxIconAreaStyles[DOWN] = mComboBoxIconAreaStyles[OVER];

    // Slider
    copyStyles(mButtonStyles, mSliderTrackStyles);
    mSliderTrackStyles[NORMAL].fgColor = params.accentColor;
    mSliderTrackStyles[DISABLED].fgColor = params.accentColor.toGrey();
    mSliderTrackStyles[OVER] = mSliderTrackStyles[NORMAL];
    mSliderTrackStyles[DOWN] = mSliderTrackStyles[NORMAL];
    // Note that text colors can be alpha on macOS, and we need the thumb's background
    // colors to be solid to hide everything underneath.
    copyStyles(mButtonStyles, mSliderThumbStyles);
    mSliderThumbStyles[DISABLED].bgColor = Color(0.5f, 0.5f, 0.5f);
    if (isDarkMode) {
        mSliderThumbStyles[NORMAL].bgColor = Color(0.85f, 0.85f, 0.85f);
        mSliderThumbStyles[OVER].bgColor = Color(0.9f, 0.9f, 0.9f);
        mSliderThumbStyles[DOWN].bgColor = Color(1.0f, 1.0f, 1.0f);
    } else {
        mSliderThumbStyles[NORMAL].bgColor = Color(1.0f, 1.0f, 1.0f);
        mSliderThumbStyles[OVER].bgColor = Color(0.975f, 0.975f, 0.975f);
        mSliderThumbStyles[DOWN].bgColor = Color(0.95f, 0.95f, 0.95f);
    }

    // Scrollbar
    mScrollbarTrackStyles[NORMAL].bgColor = Color::kTransparent;
    mScrollbarTrackStyles[NORMAL].fgColor = params.textColor;
    if (Application::instance().shouldHideScrollbars()) {
        mScrollbarTrackStyles[NORMAL].borderColor = Color::kTransparent;
        mScrollbarTrackStyles[NORMAL].borderWidth = PicaPt::kZero;
    } else {
        mScrollbarTrackStyles[NORMAL].borderColor = borderColor;
        mScrollbarTrackStyles[NORMAL].borderWidth = mBorderWidth;
    }
    mScrollbarTrackStyles[NORMAL].borderRadius = PicaPt::kZero;
    mScrollbarTrackStyles[DISABLED] = mScrollbarTrackStyles[NORMAL];
    mScrollbarTrackStyles[OVER] = mScrollbarTrackStyles[NORMAL];
    mScrollbarTrackStyles[DOWN] = mScrollbarTrackStyles[NORMAL];

    if (isDarkMode) {
        mScrollbarThumbStyles[NORMAL].bgColor = Color(1.0f, 1.0f, 1.0f, 0.5f);
    } else {
        mScrollbarThumbStyles[NORMAL].bgColor = Color(0.0f, 0.0f, 0.0f, 0.5f);
    }
    mScrollbarThumbStyles[NORMAL].fgColor = params.textColor;
    mScrollbarThumbStyles[NORMAL].borderColor = Color::kTransparent;
    mScrollbarThumbStyles[NORMAL].borderWidth = PicaPt::kZero;
    mScrollbarThumbStyles[NORMAL].borderRadius = mBorderRadius;
    mScrollbarThumbStyles[DISABLED] = mScrollbarThumbStyles[NORMAL];
    mScrollbarThumbStyles[OVER] = mScrollbarThumbStyles[NORMAL];
    mScrollbarThumbStyles[DOWN] = mScrollbarThumbStyles[NORMAL];
    if (isDarkMode) {
        mScrollbarThumbStyles[OVER].bgColor = mScrollbarThumbStyles[NORMAL].bgColor.lighter(0.1f);
        mScrollbarThumbStyles[DOWN].bgColor = mScrollbarThumbStyles[NORMAL].bgColor.lighter(0.3f);
    } else {
        mScrollbarThumbStyles[OVER].bgColor = mScrollbarThumbStyles[NORMAL].bgColor.darker(0.1f);
        mScrollbarThumbStyles[DOWN].bgColor = mScrollbarThumbStyles[NORMAL].bgColor.darker(0.3f);
    }

    // ProgressBar
    copyStyles(mSliderTrackStyles, mProgressBarStyles);

    // TextEdit
    mTextEditStyles[NORMAL].bgColor = Color::kTransparent;
    mTextEditStyles[NORMAL].fgColor = params.textColor;
    mTextEditStyles[NORMAL].borderColor = borderColor;
    mTextEditStyles[NORMAL].borderWidth = mBorderWidth;
    mTextEditStyles[NORMAL].borderRadius = PicaPt::kZero;
    mTextEditStyles[DISABLED] = mTextEditStyles[NORMAL];
    mTextEditStyles[DISABLED].fgColor = params.disabledTextColor;
    mTextEditStyles[OVER] = mTextEditStyles[NORMAL];
    mTextEditStyles[DOWN] = mTextEditStyles[NORMAL];
    mTextEditStyles[SELECTED] = mTextEditStyles[SELECTED];

    // ScrollView
    mScrollViewStyles[NORMAL].bgColor = Color::kTransparent;
    mScrollViewStyles[NORMAL].fgColor = params.textColor;
    mScrollViewStyles[NORMAL].borderColor = borderColor;
    mScrollViewStyles[NORMAL].borderWidth = mBorderWidth;
    mScrollViewStyles[NORMAL].borderRadius = PicaPt::kZero;
    mScrollViewStyles[DISABLED] = mScrollViewStyles[NORMAL];
    mScrollViewStyles[OVER] = mScrollViewStyles[NORMAL];
    mScrollViewStyles[DOWN] = mScrollViewStyles[NORMAL];

    // ListView
    copyStyles(mScrollViewStyles, mListViewStyles);
    mListViewStyles[NORMAL].fgColor = params.accentColor;
    mListViewStyles[DISABLED].fgColor = Color(0.5f, 0.5f, 0.5f);
    mListViewStyles[OVER].fgColor = mListViewStyles[OVER].bgColor;  // don't highlight individual row
    mListViewStyles[DOWN].fgColor = mListViewStyles[DOWN].bgColor;  // don't highlight individual row
    mListViewStyles[SELECTED].fgColor = mParams.accentColor;
    mListViewStyles[SELECTED].bgColor = mParams.accentedBackgroundTextColor;

    // Menu Items
    mMenuItemStyles[NORMAL].bgColor = Color::kTransparent;
    mMenuItemStyles[NORMAL].fgColor = params.textColor;
    mMenuItemStyles[NORMAL].borderColor = Color::kTransparent;
    mMenuItemStyles[NORMAL].borderWidth = PicaPt::kZero;
    mMenuItemStyles[NORMAL].borderRadius = PicaPt::kZero;
    mMenuItemStyles[DISABLED] = mMenuItemStyles[NORMAL];
    mMenuItemStyles[DISABLED].fgColor = params.disabledTextColor;
    mMenuItemStyles[OVER] = mMenuItemStyles[NORMAL];
    mMenuItemStyles[OVER].bgColor = params.accentColor;
    mMenuItemStyles[OVER].fgColor = params.accentedBackgroundTextColor;
    mMenuItemStyles[DOWN] = mMenuItemStyles[OVER];
    mMenuItemStyles[SELECTED].bgColor = params.accentColor;
    mMenuItemStyles[SELECTED].fgColor = params.accentedBackgroundTextColor;

    // Menubar items
    copyStyles(mMenuItemStyles, mMenubarItemStyles);
    mMenubarItemStyles[OVER].bgColor = mMenubarItemStyles[NORMAL].bgColor;
    mMenubarItemStyles[DOWN].bgColor = params.accentColor;
    mMenubarItemStyles[DOWN].fgColor = params.accentedBackgroundTextColor;
}

const Theme::Params& VectorBaseTheme::params() const { return mParams; }

void VectorBaseTheme::setParams(const Params& params)
{
    setVectorParams(params);
}

Size VectorBaseTheme::calcPreferredTextMargins(const DrawContext& dc, const Font& font) const
{
    auto fm = dc.fontMetrics(font);
    auto tm = dc.textMetrics("Ag", font, kPaintFill);
    auto em = tm.height;
    auto margin = dc.ceilToNearestPixel(1.5 * fm.descent);
    return Size(margin, margin);
}

// Ideally all the widgets are the same size so that the naively placing them
// next to each other aligns the text baselines nicely. Buttons are a good widget
// to choose as the size, so this is called from the other preference-calculating
// functions to get the height.
Size VectorBaseTheme::calcPreferredButtonSize(const DrawContext& dc, const Font& font,
                                              const std::string& text) const
{
    auto fm = dc.fontMetrics(font);
    auto tm = dc.textMetrics(text.c_str(), font, kPaintFill);
    auto em = tm.height;
    auto margin = dc.ceilToNearestPixel(1.5 * fm.descent);
    // Height works best if the descent is part of the bottom margin,
    // because it looks visually empty even if there are a few descenders.
    // Now the ascent can be anything the font designer want it to be,
    // which is not helpful for computing accurate margins. But cap-height
    // is well-defined, so use that instead.
    return Size(dc.ceilToNearestPixel(2.0f * em + tm.width) + 2.0f * margin,
                dc.ceilToNearestPixel(fm.capHeight) + 2.0f * margin);
}

Size VectorBaseTheme::calcPreferredCheckboxSize(const DrawContext& dc,
                                                const Font& font) const
{
    auto size = calcPreferredButtonSize(dc, font, "Ag");
    return Size(size.height, size.height);
}

Size VectorBaseTheme::calcPreferredSegmentSize(const DrawContext& dc,
                                               const Font& font,
                                               const std::string& text) const
{
    auto pref = calcPreferredButtonSize(dc, font, text);
    auto tm = dc.textMetrics(text.c_str(), font, kPaintFill);
    auto margin = tm.height;  // 0.5*em on either side
    return Size(dc.ceilToNearestPixel(tm.width + margin),
                pref.height);  // height is already ceil'd.
}

Size VectorBaseTheme::calcPreferredComboBoxSize(const DrawContext& dc,
                                                const PicaPt& preferredMenuWidth) const
{
    auto height = calcPreferredButtonSize(dc, mParams.labelFont, "Ag").height;
    return Size(dc.ceilToNearestPixel(preferredMenuWidth + 0.8f * height), height);
}

Size VectorBaseTheme::calcPreferredSliderThumbSize(const DrawContext& ui) const
{
    auto buttonHeight = calcPreferredButtonSize(ui, mParams.labelFont, "Ag").height;
    return Size(buttonHeight, buttonHeight);
}

Size VectorBaseTheme::calcPreferredProgressBarSize(const DrawContext& dc) const
{
    auto buttonHeight = calcPreferredButtonSize(dc, mParams.labelFont, "Ag").height;
    return Size(PicaPt(144.0f), buttonHeight);
}

Size VectorBaseTheme::calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const
{
    auto buttonHeight = calcPreferredButtonSize(dc, mParams.labelFont, "Ag").height;
    return Size(Widget::kDimGrow, buttonHeight);
}

Rect VectorBaseTheme::calcTextEditRectForFrame(const Rect& frame, const DrawContext& dc,
                                               const Font& font) const
{
    auto textMargins = calcPreferredTextMargins(dc, font);
    auto fm = dc.fontMetrics(mParams.labelFont);
    auto baseline = dc.ceilToNearestPixel(frame.y + 0.5f * (frame.height + fm.capHeight));
    return Rect(frame.x + textMargins.width,
                baseline - fm.ascent,
                frame.width - 2.0f * textMargins.width,
                fm.ascent + fm.descent);
}

Size VectorBaseTheme::calcPreferredIncDecSize(const DrawContext& dc) const
{
    auto buttonHeight = calcPreferredButtonSize(dc, mParams.labelFont, "Ag").height;
    return Size(0.5f * buttonHeight, buttonHeight);
}

PicaPt VectorBaseTheme::calcPreferredScrollbarThickness(const DrawContext& dc) const
{
    auto fm = dc.fontMetrics(mParams.labelFont);
    return dc.ceilToNearestPixel(0.5f * fm.capHeight + fm.descent);
}

Size VectorBaseTheme::calcPreferredMenuItemSize(const DrawContext& dc,
                                                const std::string& text, const std::string& shortcut,
                                                MenuItemAttribute itemAttr,
                                                PicaPt *shortcutWidth) const
{
    auto height = dc.ceilToNearestPixel(calcPreferredButtonSize(dc, mParams.labelFont, text).height);
    auto metrics = calcPreferredMenuItemMetrics(dc, height);
    auto textMetrics = dc.textMetrics(text.c_str(), mParams.labelFont, kPaintFill);
    auto twidth = dc.ceilToNearestPixel(textMetrics.width);

    PicaPt swidth;
    if (itemAttr == MenuItemAttribute::kSubmenu) {
        swidth = metrics.submenuIconSize.width;
    } else {
        auto shortcutMetrics = dc.textMetrics(shortcut.c_str(), mParams.labelFont, kPaintFill);
        swidth = dc.ceilToNearestPixel(shortcutMetrics.width);
    }
    if (shortcutWidth) {
        *shortcutWidth = swidth;
    }

    return Size(metrics.horizMargin +
                metrics.checkboxWidth + metrics.afterCheckboxSeparator +
                twidth + metrics.afterTextSeparator + swidth +
                metrics.horizMargin,
                height);
}

Theme::MenubarMetrics VectorBaseTheme::calcPreferredMenuItemMetrics(const DrawContext& dc, const PicaPt& height) const
{
    auto fm = dc.fontMetrics(mParams.labelFont);

    return {
        dc.ceilToNearestPixel(0.5f * height),  // horizMargin
        dc.ceilToNearestPixel(height),         // checkboxWidth;
        dc.ceilToNearestPixel(0.5f * height),  // afterCheckboxSeparator;
        dc.ceilToNearestPixel(height),         // afterTextSeparator;
        Size(fm.capHeight, fm.capHeight)       // submenuIconSize;
    };
}

PicaPt VectorBaseTheme::calcPreferredMenuVerticalMargin() const
{
    auto margin = 0.25f * mParams.nonNativeMenubarFont.pointSize();
    return PicaPt::fromPixels(std::round(margin.toPixels(72.0f)), 72.0f);
}

PicaPt VectorBaseTheme::calcPreferredMenubarItemHorizMargin(const DrawContext& dc, const PicaPt& height) const
{
    return dc.ceilToNearestPixel(0.5f * calcPreferredButtonSize(dc, mParams.labelFont, "Ag").height);
}

void VectorBaseTheme::drawCheckmark(UIContext& ui, const Rect& r, const WidgetStyle& style) const
{
    const auto strokeWidth = PicaPt(2);
    // We need to inset to compensate for the stroke, since the points will
    // be at the center of the stroke. Don't adjust to nearest pixel, because
    // we actually want the partial pixels, otherwise it is a pixel too much,
    // visually.
    auto margin = 0.707f * strokeWidth;
    auto thirdW = (r.width - 2.0f * margin) / 3.0f;
    auto thirdH = (r.height - 2.0f * margin) / 3.0f;
    ui.dc.save();
    ui.dc.setStrokeColor(style.fgColor);
    ui.dc.setStrokeWidth(strokeWidth);
    ui.dc.setStrokeEndCap(kEndCapRound);
    ui.dc.setStrokeJoinStyle(kJoinRound);
    Point p1(r.x + margin, r.y + margin + 2.0f * thirdH);
    Point p2(r.x + margin + thirdW, r.y + margin + 3.0f * thirdH);
    Point p3(r.x + margin + 3.0f * thirdW, r.y + margin);
    ui.dc.drawLines({ p1, p2, p3 });
    ui.dc.restore();
}

void VectorBaseTheme::drawSubmenuIcon(UIContext& ui, const Rect& frame, const WidgetStyle& style) const
{
    const auto strokeWidth = PicaPt(2);
    const float tan45 = 1.0f;  // angle of arrow is 90 deg
    auto margin = 0.5f * strokeWidth; // inset a bit because point is in center of stroke
    auto h = 0.5f * frame.height - margin;
    auto w = h / tan45;
    Point p1(frame.midX() - 0.5f * w, frame.midY() - h);
    Point p2(frame.midX() + 0.5f * w, frame.midY());
    Point p3(frame.midX() - 0.5f * w, frame.midY() + h);
    ui.dc.save();
    ui.dc.setStrokeColor(style.fgColor);
    ui.dc.setStrokeWidth(strokeWidth);
    ui.dc.setStrokeEndCap(kEndCapRound);
    ui.dc.setStrokeJoinStyle(kJoinRound);
    ui.dc.drawLines({ p1, p2, p3 });
    ui.dc.restore();
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

void VectorBaseTheme::clipFrame(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style) const
{
    auto borderWidth = style.borderWidth;
    if (style.borderColor.alpha() < 0.0001f) {
        borderWidth = PicaPt::kZero;
    }
    Rect clipRect = frame.insetted(borderWidth, borderWidth);
    if (style.borderRadius <= PicaPt::kZero) {
        ui.dc.clipToRect(clipRect);
    } else {
        auto path = ui.dc.createBezierPath();
        path->addRoundedRect(clipRect, style.borderRadius - 1.414f * borderWidth);
        ui.dc.clipToPath(path);
    }
}

Theme::WidgetStyle VectorBaseTheme::labelStyle(const WidgetStyle& style, WidgetState state) const
{
    return mLabelStyles[int(state)].merge(style);
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
        auto margin = ui.dc.ceilToNearestPixel(0.15f * frame.width);
        drawCheckmark(ui, frame.insetted(margin, margin), *bs);
    }
}

void VectorBaseTheme::drawSegmentedControl(UIContext& ui,
                                           const Rect& frame,
                                           const WidgetStyle& style,
                                           WidgetState state) const
{
    if (state == WidgetState::kDisabled) {
        drawFrame(ui, frame, mSegmentedControlStyles[int(state)].merge(style));
    } else {
        drawFrame(ui, frame, mSegmentedControlStyles[int(WidgetState::kNormal)].merge(style));
    }
}

void VectorBaseTheme::drawSegment(UIContext& ui, const Rect& frame, WidgetState state,
                                  bool isButton, bool isOn, int segmentIndex, int nSegments) const
{
    auto &widgetStyle = mSegmentedControlStyles[int(WidgetState::kNormal)];
    Rect r(frame.x, frame.y + widgetStyle.borderWidth,
           frame.width, frame.height - 2.0f * widgetStyle.borderWidth);

    Color bg;
    if (isButton) {
        bg = mSegmentStyles[int(state)].bgColor;
    } else {
        if (isOn) {
            bg = mSegmentOnStyles[int(state)].bgColor;
        } else {
            bg = mSegmentOffStyles[int(state)].bgColor;
        }
    }
    ui.dc.setFillColor(bg);

    if (widgetStyle.borderRadius > PicaPt::kZero && (segmentIndex == 0 || segmentIndex == nSegments - 1)) {
        PicaPt radius = widgetStyle.borderRadius * 1.4142135f;  // br * sqrt(2)
        if (segmentIndex == 0) {
            r.x += widgetStyle.borderWidth;
        }
        r.width -= widgetStyle.borderWidth;

        auto path = ui.dc.createBezierPath();

        // (This is copied from libnativedraw)
        // This is the weight for control points for a 4-curve sphere.
        // Normally 4 cubic splines use 0.55228475, but a better number was
        // computed by http://www.tinaja.com/glib/ellipse4.pdf.
        // It has an error of .76 px/in at 1200 DPI (0.0633%).
        PicaPt tangentWeight(0.551784f);
        PicaPt zero(0);
        PicaPt dTangent = tangentWeight * radius;

        auto topLeft = r.upperLeft() + Point(radius, zero);
        auto topRight = r.upperRight() + Point(-radius, zero);
        auto rightTop = r.upperRight() + Point(zero, radius);
        auto rightBottom = r.lowerRight() + Point(zero, -radius);
        auto bottomLeft = r.lowerLeft() + Point(radius, zero);
        auto bottomRight = r.lowerRight() + Point(-radius, zero);
        auto leftTop = r.upperLeft() + Point(zero, radius);
        auto leftBottom = r.lowerLeft() + Point(zero, -radius);

        if (segmentIndex == 0) {
            path->moveTo(leftTop);
            path->cubicTo(leftTop + Point(zero, -dTangent),
                          topLeft + Point(-dTangent, zero),
                          topLeft);
            path->lineTo(r.upperRight());
            path->lineTo(r.lowerRight());
            path->lineTo(bottomLeft);
            path->cubicTo(bottomLeft + Point(-dTangent, zero),
                          leftBottom + Point(zero, dTangent),
                          leftBottom);
        } else {
            path->moveTo(r.upperLeft());
            path->lineTo(topRight);
            path->cubicTo(topRight + Point(dTangent, zero),
                          rightTop + Point(zero, -dTangent),
                          rightTop);
            path->lineTo(rightBottom);
            path->cubicTo(rightBottom + Point(zero, dTangent),
                          bottomRight + Point(dTangent, zero),
                          bottomRight);
            path->lineTo(r.lowerLeft());
        }

        ui.dc.drawPath(path, kPaintFill);
    } else {
        ui.dc.drawRect(r, kPaintFill);
    }
}

void VectorBaseTheme::drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                                         const WidgetStyle& ctrlStyle, WidgetState ctrlState) const
{
    auto style = mSegmentedControlStyles[int(ctrlState)].merge(ctrlStyle);
    auto p1 = top;
    p1.y += style.borderWidth;
    auto p2 = bottom;
    p2.y -= style.borderWidth;

    auto onePx = ui.dc.onePixel();
    int nPixels = int(ui.dc.roundToNearestPixel(style.borderWidth) / onePx);
    if (nPixels % 2 == 1) {
        p1.x += 0.5f * onePx;
        p2.x += 0.5f * onePx;
    }
    ui.dc.drawLines({p1, p2});
}

const Theme::WidgetStyle& VectorBaseTheme::segmentTextStyle(WidgetState state, bool isOn) const
{
    if (isOn) {
        return mSegmentOnStyles[int(state)];
    } else {
        return mSegmentOffStyles[int(state)];
    }
}

void VectorBaseTheme::drawComboBoxAndClip(UIContext& ui, const Rect& frame,
                                          const WidgetStyle& style, WidgetState state) const
{
    auto s = mComboBoxStyles[int(state)].merge(style);
    drawFrame(ui, frame, s);

    auto iconWidth = ui.dc.roundToNearestPixel(0.8f * frame.height - 2.0f * s.borderWidth);
    Rect iconRect(frame.maxX() - s.borderWidth - iconWidth, frame.y + s.borderWidth,
                  iconWidth, frame.height - 2.0f * s.borderWidth);
    auto path = ui.dc.createBezierPath();
    auto radius = s.borderRadius - 1.414f * s.borderWidth;
    if (radius > PicaPt::kZero) {
        // Modified from nativedraw.cpp, BezierPath::addRoundedRect().
        PicaPt tangentWeight(0.551784f);
        PicaPt zero(0);
        PicaPt dTangent = tangentWeight * radius;

        auto topRight = iconRect.upperRight() + Point(-radius, zero);
        auto rightTop = iconRect.upperRight() + Point(zero, radius);
        auto rightBottom = iconRect.lowerRight() + Point(zero, -radius);
        auto bottomLeft = iconRect.lowerLeft() + Point(radius, zero);
        auto bottomRight = iconRect.lowerRight() + Point(-radius, zero);

        path->moveTo(iconRect.upperLeft());
        path->lineTo(topRight);
        path->cubicTo(topRight + Point(dTangent, zero),
                      rightTop + Point(zero, -dTangent),
                      rightTop);
        path->lineTo(rightBottom);
        path->cubicTo(rightBottom + Point(zero, dTangent),
                      bottomRight + Point(dTangent, zero),
                      bottomRight);
        path->lineTo(iconRect.lowerLeft());
        path->close();
    } else {
        path->addRect(iconRect);
    }
    s = mComboBoxIconAreaStyles[int(state)];
    ui.dc.setFillColor(s.bgColor);
    ui.dc.drawPath(path, kPaintFill);

    ui.dc.save();  // so line style changes get cleaned up
    ui.dc.setStrokeColor(s.fgColor);
    ui.dc.setStrokeWidth(PicaPt(1.5));
    ui.dc.setStrokeEndCap(kEndCapRound);
    ui.dc.setStrokeJoinStyle(kJoinRound);
    auto h = 0.2f * iconRect.height;
    ui.dc.drawLines({ Point(iconRect.midX() - h, iconRect.midY() - 0.5f * h),
                      Point(iconRect.midX(), iconRect.midY() - 1.5f * h),
                      Point(iconRect.midX() + h, iconRect.midY() - 0.5f * h) });
    ui.dc.drawLines({ Point(iconRect.midX() - h, iconRect.midY() + 0.5f * h),
                      Point(iconRect.midX(), iconRect.midY() + 1.5f * h),
                      Point(iconRect.midX() + h, iconRect.midY() + 0.5f * h) });
    ui.dc.restore();

    auto x = frame.x + std::max(s.borderWidth, s.borderRadius);
    ui.dc.clipToRect(Rect(x,
                          frame.y + s.borderWidth,
                          iconRect.x - x,
                          frame.height - 2.0f * s.borderWidth));
}

void VectorBaseTheme::drawSliderTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                                      const WidgetStyle& style, WidgetState state) const
{
    auto frameStyle = mSliderTrackStyles[int(state)].merge(style);  // copy

    // Draw the track
    auto h = 0.3f * frame.height;
    if (frameStyle.borderRadius > PicaPt::kZero) {
        frameStyle.borderRadius = 0.5f * h;
    }
    Rect frameRect(frame.x, frame.midY() - 0.5f * h, frame.width, h);
    drawFrame(ui, frameRect, frameStyle);

    // Draw the highlight
    auto onePx = ui.dc.onePixel();
    frameRect.inset(onePx, onePx);
    switch (dir) {
        case SliderDir::kHoriz:
            frameRect.width = thumbMid.x - frameRect.x;
            break;
        case SliderDir::kVertZeroAtTop:
            frameRect.height = thumbMid.y - frameRect.y;
            break;
        case SliderDir::kVertZeroAtBottom:
            frameRect.height = frameRect.maxX() - thumbMid.y;
            frameRect.y = thumbMid.y;
            break;
    }
    frameStyle.bgColor = frameStyle.fgColor;
    frameStyle.borderRadius = frameStyle.borderRadius - frameStyle.borderWidth - onePx;
    frameStyle.borderWidth = PicaPt::kZero;
    drawFrame(ui, frameRect, frameStyle);
}

void VectorBaseTheme::drawSliderThumb(UIContext& ui, const Rect& frame,
                                      const WidgetStyle& style, WidgetState state) const
{
    // Q: Why don't we control the frame of the thumb?
    // A: The widget needs to handle mouse movements, so it needs to control the frame,
    //    otherwise it cannot guarantee accuracy. This limits the control of the theme has,
    //    but ultimately the theme and widget must work together. We are the view; all we
    //    can do is draw what the controller gives us.

    auto thumbStyle = mSliderThumbStyles[int(state)].merge(style);
    if (thumbStyle.borderRadius > PicaPt::kZero) {
        thumbStyle.borderRadius = 0.5f * frame.height;
    }
    drawFrame(ui, frame, thumbStyle);
}

void VectorBaseTheme::drawScrollbarTrack(UIContext& ui, SliderDir dir, const Rect& frame,
                                         const Point& thumbMid, const WidgetStyle& style,
                                         WidgetState state) const
{
    auto frameStyle = mScrollbarTrackStyles[int(state)].merge(style);  // copy

    // Draw the track
    auto h = 0.3f * frame.height;
    if (frameStyle.borderRadius > PicaPt::kZero) {
        frameStyle.borderRadius = 0.5f * h;
    }
    drawFrame(ui, frame, frameStyle);
}

void VectorBaseTheme::drawScrollbarThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                         WidgetState state) const
{
    auto thumbStyle = mScrollbarThumbStyles[int(state)].merge(style);
    if (thumbStyle.borderRadius > PicaPt::kZero) {
        thumbStyle.borderRadius = 0.5f * std::min(frame.width, frame.height);
    }
    drawFrame(ui, frame, thumbStyle);
}

void VectorBaseTheme::drawProgressBar(UIContext& ui, const Rect& frame, float value,
                                      const WidgetStyle& style, WidgetState state) const
{
    drawSliderTrack(ui, SliderDir::kHoriz, frame,
                    Point(frame.x + 0.01f * value * frame.width, PicaPt::kZero),
                    mProgressBarStyles[int(state)].merge(style), state);
}

void VectorBaseTheme::drawIncDec(UIContext& ui, const Rect& frame,
                                 WidgetState incState, WidgetState decState) const
{
    auto incRect = frame;
    incRect.height = 0.5f * frame.height;
    auto decRect = frame;
    decRect.height = 0.5f * frame.height;
    decRect.y = decRect.y + decRect.height;

    ui.dc.save();
    ui.dc.clipToRect(incRect);
    drawFrame(ui, frame, mButtonStyles[int(incState)]);
    ui.dc.restore();

    ui.dc.save();
    ui.dc.clipToRect(decRect);
    drawFrame(ui, frame, mButtonStyles[int(decState)]);
    ui.dc.restore();

    ui.dc.save();  // so line style changes get cleaned up
    ui.dc.setStrokeWidth(PicaPt(1.5));
    ui.dc.setStrokeEndCap(kEndCapRound);
    ui.dc.setStrokeJoinStyle(kJoinRound);
    auto w = 0.125f * frame.height;
    auto h = 0.2f * frame.height;
    ui.dc.setStrokeColor(mButtonStyles[int(incState)].fgColor);
    auto top = frame.midY() - 0.2f * frame.height;
    ui.dc.drawLines({ Point(frame.midX() - w, top),
                      Point(frame.midX(), top - w),
                      Point(frame.midX() + w, top) });
    ui.dc.setStrokeColor(mButtonStyles[int(decState)].fgColor);
    auto bottom = frame.midY() + 0.2f * frame.height + w;
    ui.dc.drawLines({ Point(frame.midX() - w, bottom - w),
                      Point(frame.midX(), bottom),
                      Point(frame.midX() + w, bottom - w) });
    ui.dc.restore();
}

Theme::WidgetStyle VectorBaseTheme::textEditStyle(const WidgetStyle& style, WidgetState state) const
{
    return mTextEditStyles[int(state)].merge(style);
}

void VectorBaseTheme::drawTextEdit(UIContext& ui, const Rect& frame, const PicaPt& scrollOffset,
                                   const std::string& placeholder, TextEditorLogic& editor,
                                   int horizAlign, const WidgetStyle& style, WidgetState state,
                                   bool hasFocus) const
{
    horizAlign = horizAlign & Alignment::kHorizMask;

    auto s = textEditStyle(style, state);
    drawFrame(ui, frame, s);

    auto font = mParams.labelFont;
    auto textMargins = calcPreferredTextMargins(ui.dc, font);
    auto selectionStart = PicaPt::kZero;
    auto selectionEnd = selectionStart;
    if (hasFocus) {
        auto sel = editor.selection();
        selectionStart = editor.pointAtIndex(sel.start).x + textMargins.width + scrollOffset;
        selectionEnd = editor.pointAtIndex(sel.end).x + textMargins.width + scrollOffset;
    }

    auto textRect = frame;
    textRect.x += textMargins.width;
    textRect.width -= 2.0f * textMargins.width;

    PicaPt caretWidth = ui.dc.ceilToNearestPixel(0.05f * textRect.height);

    ui.dc.save();
    // Outset textRect by the caret width for the clip rect so that cursor is visible at edges
    ui.dc.clipToRect(textRect.insetted(-caretWidth, PicaPt::kZero));

    if (hasFocus && selectionStart != selectionEnd) {
        auto selectionRect = Rect(ui.dc.roundToNearestPixel(selectionStart),
                                  textRect.y,
                                  PicaPt::kZero,
                                  textRect.height);
        selectionRect.width = ui.dc.roundToNearestPixel(selectionEnd) - selectionRect.x;
        ui.dc.setFillColor(mParams.selectionColor);
        ui.dc.drawRect(selectionRect, kPaintFill);
    }

    if (editor.isEmpty()) {
        if (!placeholder.empty()) {
            ui.dc.setFillColor(mTextEditStyles[int(WidgetState::kDisabled)].fgColor);
            ui.dc.drawText(placeholder.c_str(), textRect, horizAlign | Alignment::kVCenter,
                           TextWrapMode::kNoWrap, mParams.labelFont, kPaintFill);
        }
    } else {
        // The layout incorporates the color, so we cannot set it.
        auto textRect = calcTextEditRectForFrame(frame, ui.dc, font);
        if (editor.layout()) {
            ui.dc.drawText(*editor.layout(), textRect.upperLeft() + Point(scrollOffset, PicaPt::kZero));
        }
    }

    ui.dc.restore();

    if (hasFocus && selectionStart == selectionEnd) {
        PicaPt x = ui.dc.roundToNearestPixel(selectionStart) - std::floor(0.5f * caretWidth / ui.dc.onePixel());

        // On macOS, text caret is same color as text. Usually there is no need to change
        // the fill color, but in the case we have drew placeholder text the color will
        // be wrong.
        ui.dc.setFillColor(s.fgColor);
        ui.dc.drawRect(Rect(x, textRect.y, caretWidth, textRect.height), kPaintFill);
    }
}

void VectorBaseTheme::clipScrollView(UIContext& ui, const Rect& frame,
                                     const WidgetStyle& style, WidgetState state) const
{
    clipFrame(ui, frame, mScrollViewStyles[int(state)].merge(style));
}

void VectorBaseTheme::drawScrollView(UIContext& ui, const Rect& frame,
                                     const WidgetStyle& style, WidgetState state) const
{
    drawFrame(ui, frame, mScrollViewStyles[int(state)].merge(style));
}

void VectorBaseTheme::drawListView(UIContext& ui, const Rect& frame,
                                   const WidgetStyle& style, WidgetState state) const
{
    drawFrame(ui, frame, mListViewStyles[int(state)].merge(style));
}

void VectorBaseTheme::clipListView(UIContext& ui, const Rect& frame,
                                   const WidgetStyle& style, WidgetState state) const
{
    clipFrame(ui, frame, mListViewStyles[int(state)].merge(style));
}

void VectorBaseTheme::drawListViewSpecialRow(UIContext& ui, const Rect& frame,
                                             const WidgetStyle& style, WidgetState state) const
{
    WidgetStyle s = mListViewStyles[int(state)].merge(style);
    ui.dc.setFillColor(s.fgColor);
    ui.dc.drawRect(frame, kPaintFill);
}

void VectorBaseTheme::drawMenuBackground(UIContext& ui, const Size& size)
{
    if (mParams.nonNativeMenuBackgroundColor.alpha() > 0.0f) {
        ui.dc.fill(mParams.nonNativeMenuBackgroundColor);
    }
}

void VectorBaseTheme::calcMenuItemFrames(const DrawContext& dc, const Rect& frame,
                                         const PicaPt& shortcutWidth,
                                         Rect *checkRect, Rect *textRect, Rect *shortcutRect) const
{
    auto metrics = calcPreferredMenuItemMetrics(dc, frame.height);
    auto fm = mParams.labelFont.metrics(dc);
    // Checkmark should be in the cap-height of the text
    Rect checkmarkRect(frame.x + metrics.horizMargin, frame.midY() - 0.5f * fm.capHeight,
                       fm.capHeight, fm.capHeight);
    auto x = dc.ceilToNearestPixel(checkmarkRect.maxX() + metrics.afterCheckboxSeparator);
    if (checkRect) {
        *checkRect = checkmarkRect;
    }

    auto shortcutX = frame.width - metrics.horizMargin - shortcutWidth;
    if (shortcutRect) {
        *shortcutRect = Rect(shortcutX, frame.y, shortcutWidth, frame.height);
    }

    if (textRect) {
        *textRect = Rect(x, frame.y, shortcutX - x, frame.height);
    }
}

void VectorBaseTheme::drawMenuItem(UIContext& ui, const Rect& frame, const PicaPt& shortcutWidth,
                                   const std::string& text, const std::string& shortcutKey,
                                   MenuItemAttribute itemAttr,
                                   const WidgetStyle& style, WidgetState state) const
{
    Rect checkmarkRect, textRect, shortcutRect;
    calcMenuItemFrames(ui.dc, frame, shortcutWidth, &checkmarkRect, &textRect, &shortcutRect);

    auto s = mMenuItemStyles[int(state)].merge(style);
    drawFrame(ui, frame, s);
    if (itemAttr == MenuItemAttribute::kChecked) {
        drawCheckmark(ui, checkmarkRect, s);
    }
    ui.dc.setFillColor(s.fgColor);
    ui.dc.drawText(text.c_str(), textRect, Alignment::kLeft | Alignment::kVCenter,
                   TextWrapMode::kNoWrap, mParams.labelFont, kPaintFill);
    if (itemAttr == MenuItemAttribute::kSubmenu) {
        auto itemMetrics = calcPreferredMenuItemMetrics(ui.dc, frame.height);
        Rect r(shortcutRect.maxX() - itemMetrics.submenuIconSize.width,
               shortcutRect.midY() - 0.5 * itemMetrics.submenuIconSize.height,
               itemMetrics.submenuIconSize.width,
               itemMetrics.submenuIconSize.height);
        drawSubmenuIcon(ui, r, s);
    } else {
        ui.dc.drawText(shortcutKey.c_str(), shortcutRect, Alignment::kRight | Alignment::kVCenter,
                       TextWrapMode::kNoWrap, mParams.labelFont, kPaintFill);
    }
}

void VectorBaseTheme::drawMenuSeparatorItem(UIContext& ui, const Rect& frame) const
{
    int thicknessPx = int(PicaPt(2) / ui.dc.onePixel());
    if (thicknessPx % 2 == 1) {
        thicknessPx += 1;
    }
    ui.dc.setStrokeColor(mParams.nonNativeMenuSeparatorColor);
    ui.dc.setStrokeWidth(float(thicknessPx) * ui.dc.onePixel());
    ui.dc.setStrokeEndCap(kEndCapButt);
    ui.dc.drawLines({Point(frame.x, frame.midY()), Point(frame.maxX(), frame.midY()) });
}

void VectorBaseTheme::drawMenubarBackground(UIContext& ui, const Rect& frame) const
{
    Color bgColor(mParams.nonNativeMenubarBackgroundColor);
    Color borderColor(mParams.textColor, 0.075f);
    // Make the bottom border 0.5 pt. This will be 1px at 144 dpi, and since it is ceil()
    // it is also 1px at 72 dpi. But at 216 dpi it will just be 2px. We cannot use
    // ui.dc.onePixel() because at greater than 2X resolutions it will start getting
    // practically invisible.
    auto borderWidth = ui.dc.ceilToNearestPixel(PicaPt(0.5));
    auto onePx = ui.dc.onePixel();

    ui.dc.save();
    ui.dc.setFillColor(bgColor);
    ui.dc.drawRect(frame, kPaintFill);
    ui.dc.setStrokeColor(borderColor);
    ui.dc.setStrokeWidth(borderWidth);
    ui.dc.setStrokeEndCap(kEndCapButt);  // in case this got set previously
    ui.dc.setStrokeDashes({}, PicaPt::kZero);  // in case this got set previously
    ui.dc.drawLines({ Point(frame.x, frame.maxY() - 0.5f * onePx),
                      Point(frame.maxX(), frame.maxY() - 0.5f * onePx) });
    ui.dc.restore();
}

void VectorBaseTheme::drawMenubarItem(UIContext& ui, const Rect& frame, const std::string& text,
                                      WidgetState state) const
{
    auto s = mMenubarItemStyles[int(state)];
    drawFrame(ui, frame, s);
    ui.dc.setFillColor(s.fgColor);
    ui.dc.drawText(text.c_str(), frame, Alignment::kCenter, TextWrapMode::kNoWrap,
                   mParams.nonNativeMenubarFont, kPaintFill);
}

}  // namespace uitk
