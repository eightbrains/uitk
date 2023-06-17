//-----------------------------------------------------------------------------
// Copyright 2021 - 2022 Eight Brains Studios, LLC
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
    if (params.useHighContrast) {
        mButtonStyles[NORMAL].borderColor = params.borderColor;
    } else {
        mButtonStyles[NORMAL].borderColor = params.nonEditableBackgroundColor.darker(0.2f);
    }
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

    // Undecorated button (normal)
    mButtonUndecoratedStyles[NORMAL].bgColor = Color::kTransparent;
    mButtonUndecoratedStyles[NORMAL].fgColor = params.textColor;
    mButtonUndecoratedStyles[NORMAL].borderColor = Color::kTransparent;
    mButtonUndecoratedStyles[NORMAL].borderWidth = PicaPt::kZero;
    mButtonUndecoratedStyles[NORMAL].borderRadius = mBorderRadius;
    mButtonUndecoratedStyles[DISABLED] = mButtonUndecoratedStyles[NORMAL];
    mButtonUndecoratedStyles[DISABLED].fgColor = params.disabledTextColor;
    mButtonUndecoratedStyles[OVER] = mButtonUndecoratedStyles[NORMAL];
    if (isDarkMode) {
        mButtonUndecoratedStyles[OVER].fgColor = mButtonStyles[NORMAL].fgColor.lighter();
    } else {
        mButtonUndecoratedStyles[OVER].fgColor = mButtonStyles[NORMAL].fgColor.darker(0.025f);
    }
    mButtonUndecoratedStyles[DOWN] = mButtonUndecoratedStyles[NORMAL];
    mButtonUndecoratedStyles[DOWN].fgColor = params.accentColor;
    mButtonUndecoratedStyles[SELECTED] = mButtonUndecoratedStyles[DOWN];

    // Undecorated button (ON)
    copyStyles(mButtonUndecoratedStyles, mButtonUndecoratedOnStyles);
    mButtonUndecoratedOnStyles[NORMAL].fgColor = params.accentColor;
    mButtonUndecoratedOnStyles[DISABLED].fgColor = mButtonOnStyles[DISABLED].bgColor;
    mButtonUndecoratedOnStyles[OVER].fgColor = params.accentColor.lighter();
    mButtonUndecoratedOnStyles[DOWN].fgColor = params.textColor;
    mButtonUndecoratedOnStyles[SELECTED] = mButtonUndecoratedOnStyles[DOWN];

    // Accessory button, like the X that clear a text widget
    copyStyles(mButtonStyles, mButtonAccessoryStyles);
    mButtonAccessoryStyles[NORMAL].bgColor = Color::kTransparent;
    mButtonAccessoryStyles[NORMAL].fgColor = params.textColor;
    mButtonAccessoryStyles[NORMAL].borderColor = Color::kTransparent;
    mButtonAccessoryStyles[NORMAL].borderWidth = PicaPt::kZero;
    mButtonAccessoryStyles[DISABLED] = mButtonAccessoryStyles[NORMAL];
    mButtonAccessoryStyles[DISABLED].fgColor = Color(mButtonAccessoryStyles[NORMAL].fgColor,
                                                     0.75f * mButtonAccessoryStyles[NORMAL].fgColor.alpha());
    mButtonAccessoryStyles[OVER] = mButtonAccessoryStyles[NORMAL];
    mButtonAccessoryStyles[OVER].fgColor = mButtonAccessoryStyles[NORMAL].fgColor.lighter();
    mButtonAccessoryStyles[DOWN] = mButtonAccessoryStyles[NORMAL];
    mButtonAccessoryStyles[DOWN].fgColor = params.textColor;
    mButtonAccessoryStyles[SELECTED] = mButtonAccessoryStyles[NORMAL];

    // Button that is default for a dialog
    copyStyles(mButtonStyles, mButtonDefaultDialogStyles);
    mButtonDefaultDialogStyles[NORMAL].bgColor = params.accentColor;
    mButtonDefaultDialogStyles[NORMAL].fgColor = params.accentedBackgroundTextColor;
    mButtonDefaultDialogStyles[OVER].bgColor = params.accentColor.lighter();
    mButtonDefaultDialogStyles[DOWN].bgColor = mButtonDefaultDialogStyles[OVER].bgColor;

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

    // Segmented control, button action (undecorated)
    copyStyles(mButtonUndecoratedStyles, mSegmentUndecoratedStyles);

    // Segmented control, toggleable (undecorated)
    copyStyles(mButtonUndecoratedStyles, mSegmentUndecoratedOffStyles);
    copyStyles(mButtonUndecoratedOnStyles, mSegmentUndecoratedOnStyles);

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

    // ColorEdit
    copyStyles(mComboBoxStyles, mColorEditTrackStyles);

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
        mScrollbarTrackStyles[NORMAL].borderColor = params.borderColor;
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
    if (params.useHighContrast) {
        // Windows 10's high contrast can use yellow or green for text
        // It looks rather strange to have the track green and the thumb white.
        // (However, Settings, MSVC, and Firefox all do the scrollbar differently,
        // so there's no right way). On macOS the text color will be the same
        // as the color above, so no change.
        mScrollbarThumbStyles[NORMAL].bgColor = params.textColor;
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
    if (params.useHighContrast) {
        // This is especially important for Windows 10, where the scrollbars
        mScrollbarThumbStyles[NORMAL].bgColor = params.textColor;
        mScrollbarThumbStyles[OVER].bgColor = params.textColor;
        mScrollbarThumbStyles[DOWN].bgColor = params.textColor;
    }

    // ProgressBar
    copyStyles(mSliderTrackStyles, mProgressBarStyles);

    // TextEdit
    mTextEditStyles[NORMAL].bgColor = params.editableBackgroundColor;
    mTextEditStyles[NORMAL].fgColor = params.textColor;
    mTextEditStyles[NORMAL].borderColor = params.borderColor;
    mTextEditStyles[NORMAL].borderWidth = mBorderWidth;
    mTextEditStyles[NORMAL].borderRadius = PicaPt::kZero;
    mTextEditStyles[DISABLED] = mTextEditStyles[NORMAL];
    mTextEditStyles[DISABLED].fgColor = params.disabledTextColor;
    mTextEditStyles[OVER] = mTextEditStyles[NORMAL];
    mTextEditStyles[DOWN] = mTextEditStyles[NORMAL];
    mTextEditStyles[SELECTED] = mTextEditStyles[SELECTED];

    // SearchBar
    copyStyles(mTextEditStyles, mSearchBarStyles);
    mSearchBarStyles[NORMAL].borderRadius = mBorderRadius;
    mSearchBarStyles[DISABLED].borderRadius = mBorderRadius;
    mSearchBarStyles[OVER].borderRadius = mBorderRadius;
    mSearchBarStyles[DOWN].borderRadius = mBorderRadius;

    // Splitter
    mSplitterStyles[NORMAL].bgColor = params.splitterColor;
    mSplitterStyles[DISABLED] = mSplitterStyles[NORMAL];
    mSplitterStyles[OVER] = mSplitterStyles[NORMAL];
    mSplitterStyles[DOWN] = mSplitterStyles[NORMAL];
    mSplitterStyles[SELECTED] = mSplitterStyles[NORMAL];

    // ScrollView
    mScrollViewStyles[NORMAL].bgColor = Color::kTransparent;
    mScrollViewStyles[NORMAL].fgColor = params.textColor;
    mScrollViewStyles[NORMAL].borderColor = params.borderColor;
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

    // Tooltips
    // Neither macOS nor Win32 offer any good way of getting tooltip colors.
    // Tooltips are not consistent within native apps, although typically
    // dark mode has the background a little lighter, but light mode uses the
    // same background as the window background. The border color is also not
    // consistent: on macOS the border is usually darker/lighter than the background
    // color, but the macOS 10.14 Settings app has a black border. Windows 10 still
    // has the old yellow tooltips in some places, like the close/minimize/maximize
    // buttons on the window.
    if (isDarkMode) {
        mTooltipStyle.bgColor = params.windowBackgroundColor.lighter();
        mTooltipStyle.borderColor = mTooltipStyle.bgColor.lighter();
    } else {
        mTooltipStyle.bgColor = params.windowBackgroundColor;  // gets too dark if we use darker()
        mTooltipStyle.borderColor = mTooltipStyle.bgColor.darker();
    }
    mTooltipStyle.borderWidth = mBorderWidth;
}

const Theme::Params& VectorBaseTheme::params() const { return mParams; }

void VectorBaseTheme::setParams(const Params& params)
{
    setVectorParams(params);
}

Size VectorBaseTheme::calcPreferredTextMargins(const DrawContext& dc, const Font& font) const
{
    auto fm = dc.fontMetrics(font);
    auto em = fm.ascent + fm.descent;
    auto margin = dc.ceilToNearestPixel(1.5 * fm.descent);
    return Size(margin, margin);
}

PicaPt VectorBaseTheme::calcStandardHeight(const DrawContext& dc, const Font& font) const
{
    auto fm = dc.fontMetrics(font);
    // Height works best if the descent is part of the bottom margin,
    // because it looks visually empty even if there are a few descenders.
    // Now the ascent can be anything the font designer want it to be,
    // which is not helpful for computing accurate margins. But cap-height
    // is well-defined, so use that instead.
    return dc.ceilToNearestPixel(fm.capHeight) + 2.0f * calcPreferredTextMargins(dc, font).height;
}

Size VectorBaseTheme::calcStandardIconSize(const DrawContext& dc, const Font& font) const
{
    auto fm = dc.fontMetrics(font);
    auto size = dc.ceilToNearestPixel(fm.capHeight + fm.descent);
    return Size(size, size);
}

Rect VectorBaseTheme::calcStandardIconRect(const DrawContext& dc, const Rect& frame, const Font& font) const
{
    auto size = calcStandardIconSize(dc, font);
    auto x = frame.x + dc.roundToNearestPixel(0.5f * (frame.width - size.width));
    auto y = frame.y + dc.roundToNearestPixel(0.5f * (frame.height - size.height));
    return Rect(x, y, size.width, size.height);
}

PicaPt VectorBaseTheme::calcStandardIconSeparator(const DrawContext& dc, const Font& font) const
{
    return dc.roundToNearestPixel(0.1f * font.pointSize());
}

Size VectorBaseTheme::calcPreferredButtonMargins(const DrawContext& dc, const Font& font) const
{
    auto fm = dc.fontMetrics(font);
    return Size(dc.ceilToNearestPixel(0.5f * (fm.capHeight + fm.descent)), PicaPt::kZero);
}

Size VectorBaseTheme::calcPreferredCheckboxSize(const DrawContext& dc,
                                                const Font& font) const
{
    auto size = calcStandardHeight(dc, font);
    return Size(size, size);
}

Size VectorBaseTheme::calcPreferredSegmentMargins(const DrawContext& dc, const Font& font) const
{
    // The button y-margin is also zero, but specify zero here, in case the button margin
    // becomes not zero some time (for instance, a custom layout).
    return Size(calcPreferredButtonMargins(dc, font).width, PicaPt::kZero);
}

Size VectorBaseTheme::calcPreferredComboBoxSize(const DrawContext& dc,
                                                const PicaPt& preferredMenuWidth) const
{
    auto height = calcStandardHeight(dc, mParams.labelFont);
    return Size(dc.ceilToNearestPixel(preferredMenuWidth + 0.8f * height), height);
}

Size VectorBaseTheme::calcPreferredSliderThumbSize(const DrawContext& dc) const
{
    auto height = calcStandardHeight(dc, mParams.labelFont);
    return Size(height, height);
}

Size VectorBaseTheme::calcPreferredProgressBarSize(const DrawContext& dc) const
{
    auto height = calcStandardHeight(dc, mParams.labelFont);
    return Size(PicaPt(144.0f), height);
}

Size VectorBaseTheme::calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const
{
    auto height = calcStandardHeight(dc, mParams.labelFont);
    return Size(Widget::kDimGrow, height);
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
    auto height = calcStandardHeight(dc, mParams.labelFont);
    return Size(0.5f * height, height);
}

PicaPt VectorBaseTheme::calcPreferredScrollbarThickness(const DrawContext& dc) const
{
    auto fm = dc.fontMetrics(mParams.labelFont);
    return dc.ceilToNearestPixel(0.5f * fm.capHeight + fm.descent);
}

PicaPt VectorBaseTheme::calcPreferredSplitterThumbThickness(const DrawContext& dc) const
{
    return dc.ceilToNearestPixel(PicaPt::fromStandardPixels(1.0f));
}

Size VectorBaseTheme::calcPreferredMenuItemSize(const DrawContext& dc,
                                                const std::string& text, const std::string& shortcut,
                                                MenuItemAttribute itemAttr,
                                                PicaPt *shortcutWidth) const
{
    auto height = calcStandardHeight(dc, mParams.labelFont);
    auto metrics = calcPreferredMenuItemMetrics(dc, height);
    auto textMetrics = dc.textMetrics(text.c_str(), mParams.labelFont, kPaintFill);
    auto twidth = dc.ceilToNearestPixel(textMetrics.width);

    PicaPt swidth = PicaPt::kZero;
    if (itemAttr == MenuItemAttribute::kSubmenu) {
        swidth = metrics.submenuIconSize.width;
    } else if (!shortcut.empty()) {
        auto shortcutMetrics = dc.textMetrics(shortcut.c_str(), mParams.labelFont, kPaintFill);
        swidth = dc.ceilToNearestPixel(shortcutMetrics.width);
    }
    if (shortcutWidth) {
        swidth = std::max(*shortcutWidth, swidth);
        *shortcutWidth = swidth;
    }

    return Size(metrics.horizMargin +
                metrics.checkboxWidth + metrics.afterCheckboxSeparator +
                twidth + metrics.afterTextSeparator + swidth +
                metrics.horizMargin,
                height);
}

PicaPt VectorBaseTheme::calcMenuScrollAreaHeight(const DrawContext& dc) const
{
    return calcPreferredMenuItemSize(dc, "Ag", "", MenuItemAttribute::kNormal, nullptr).height;
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
    return dc.ceilToNearestPixel(0.5f * calcStandardHeight(dc, mParams.labelFont));
}

void VectorBaseTheme::drawCheckmark(UIContext& ui, const Rect& r, const WidgetStyle& style) const
{
    const auto strokeWidth = PicaPt::fromPixels(2.0f, 96.0f);
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

void VectorBaseTheme::drawFocusFrame(UIContext& ui, const Rect& frame, const PicaPt& radius) const
{
    auto strokeWidth = PicaPt::fromStandardPixels(3.0f);
    ui.dc.save();
    if (radius > PicaPt::kZero) {
        auto expandedRadius = radius * (1.0f + (frame.width + 0.5f * strokeWidth) / frame.width);
        auto focusRect = frame.insetted(-strokeWidth, -strokeWidth);
        if (expandedRadius > 0.5f * focusRect.width) {
            expandedRadius = 0.5f * focusRect.width;
        }
        if (expandedRadius > 0.5f * focusRect.height) {
            expandedRadius = 0.5f * focusRect.height;
        }
        auto path = ui.dc.createBezierPath();
        path->addRoundedRect(focusRect, expandedRadius);
        path->moveTo(frame.upperLeft() + Point(radius, PicaPt::kZero));
        path->quarterEllipseTo(frame.upperLeft(), frame.upperLeft() + Point(PicaPt::kZero, radius));
        path->lineTo(frame.lowerLeft() - Point(PicaPt::kZero, radius));
        path->quarterEllipseTo(frame.lowerLeft(),
                               frame.lowerLeft() + Point(radius, PicaPt::kZero));
        path->lineTo(frame.lowerRight() - Point(radius, PicaPt::kZero));
        path->quarterEllipseTo(frame.lowerRight(),
                               frame.lowerRight() - Point(PicaPt::kZero, radius));
        path->lineTo(frame.upperRight() + Point(PicaPt::kZero, radius));
        path->quarterEllipseTo(frame.upperRight(),
                               frame.upperRight() - Point(radius, PicaPt::kZero));
        path->close();
        ui.dc.setFillColor(mParams.keyFocusColor);
        ui.dc.drawPath(path, kPaintFill);
    } else {
        auto focusRect = frame.insetted(-strokeWidth, -strokeWidth);
        auto path = ui.dc.createBezierPath();
        path->addRoundedRect(focusRect, strokeWidth);
        path->moveTo(frame.upperLeft());
        path->lineTo(frame.lowerLeft());
        path->lineTo(frame.lowerRight());
        path->lineTo(frame.upperRight());
        path->close();
        ui.dc.setFillColor(mParams.keyFocusColor);
        ui.dc.drawPath(path, kPaintFill);
    }
    ui.dc.restore();
}

//void VectorBaseTheme::drawFocusFrame(UIContext& ui, const std::shared_ptr<BezierPath> path) const
//{
//   TODO: need to implement path expansion
//}

Theme::WidgetStyle VectorBaseTheme::labelStyle(const WidgetStyle& style, WidgetState state) const
{
    return mLabelStyles[int(state)].merge(style);
}

void VectorBaseTheme::drawButton(UIContext& ui, const Rect& frame, ButtonDrawStyle buttonStyle,
                                 const WidgetStyle& style, WidgetState state,
                                 bool isOn) const
{
    const WidgetStyle *bs;
    switch (buttonStyle) {
        case Theme::ButtonDrawStyle::kNormal:
            if (isOn) {
                bs = &mButtonOnStyles[int(state)];
            } else {
                bs = &mButtonStyles[int(state)];
            }
            break;
        case Theme::ButtonDrawStyle::kNoDecoration:
        case Theme::ButtonDrawStyle::kAccessory:
            return;
        case Theme::ButtonDrawStyle::kDialogDefault:
            bs = &mButtonDefaultDialogStyles[int(state)];
            break;
    }
    drawFrame(ui, frame, bs->merge(style));
}

const Theme::WidgetStyle& VectorBaseTheme::buttonTextStyle(WidgetState state, ButtonDrawStyle buttonStyle,
                                                           bool isOn) const
{
    switch (buttonStyle) {
        case ButtonDrawStyle::kNormal:
            if (isOn) {
                return mButtonOnStyles[int(state)];
            } else {
                return mButtonStyles[int(state)];
            }
        case ButtonDrawStyle::kDialogDefault:
            if (isOn) {
                return mButtonOnStyles[int(state)]; // shouldn't happen
            } else {
                return mButtonDefaultDialogStyles[int(state)];
            }
        case ButtonDrawStyle::kNoDecoration:
            if (isOn) {
                return mButtonUndecoratedOnStyles[int(state)];
            } else {
                return mButtonUndecoratedStyles[int(state)];
            }
        case ButtonDrawStyle::kAccessory:
            return mButtonAccessoryStyles[int(state)];
    }
    return mButtonStyles[int(state)];  // for MSVC (don't use 'default' so we get warnings if enum changes)
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
                                           SegmentDrawStyle drawStyle,
                                           const WidgetStyle& style,
                                           WidgetState state) const
{
    if (drawStyle == SegmentDrawStyle::kNoDecoration) {
        return;
    }

    if (state == WidgetState::kDisabled) {
        drawFrame(ui, frame, mSegmentedControlStyles[int(state)].merge(style));
    } else {
        drawFrame(ui, frame, mSegmentedControlStyles[int(WidgetState::kNormal)].merge(style));
    }
}

void VectorBaseTheme::drawSegment(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                                  WidgetState state,
                                  bool isButton, bool isOn, bool showKeyFocus,
                                  int segmentIndex, int nSegments) const
{
    if (drawStyle == SegmentDrawStyle::kNoDecoration && !showKeyFocus) {
        return;
    }

    auto &widgetStyle = mSegmentedControlStyles[int(WidgetState::kNormal)];
    Rect r(frame.x, frame.y + widgetStyle.borderWidth,
           frame.width, frame.height - 2.0f * widgetStyle.borderWidth);
    if (segmentIndex > 0) {  // offset to not cover left divider (segment 0 has no left divider)
        r.x += widgetStyle.borderWidth;
    }

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
    if (drawStyle == SegmentDrawStyle::kNoDecoration && showKeyFocus) {
        bg = Color::kTransparent;
    }
    ui.dc.setFillColor(bg);

    auto borderWidth  = (showKeyFocus ? PicaPt::fromStandardPixels(1.0f) : widgetStyle.borderWidth);

    if (widgetStyle.borderRadius > PicaPt::kZero && (segmentIndex == 0 || segmentIndex == nSegments - 1)) {
        PicaPt radius = widgetStyle.borderRadius * 1.4142135f;  // br * sqrt(2)
        if (segmentIndex == 0) {
            r.x += borderWidth;
        }
        r.width -= borderWidth;

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

        if (showKeyFocus) {
            ui.dc.setStrokeWidth(borderWidth);
            if (isOn) {
                ui.dc.setStrokeColor(mParams.accentedBackgroundTextColor);
            } else {
                ui.dc.setStrokeColor(mParams.accentColor);
            }
            ui.dc.drawPath(path, kPaintStrokeAndFill);
        } else {
            ui.dc.drawPath(path, kPaintFill);
        }
    } else {
        if (showKeyFocus) {
            ui.dc.setStrokeWidth(borderWidth);
            if (isOn) {
                ui.dc.setStrokeColor(mParams.accentedBackgroundTextColor);
            } else {
                ui.dc.setStrokeColor(mParams.accentColor);
            }
            ui.dc.setStrokeJoinStyle(kJoinMiter);
            ui.dc.drawRect(r, kPaintStrokeAndFill);
        } else {
            ui.dc.drawRect(r, kPaintFill);
        }
    }
}

void VectorBaseTheme::drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                                         SegmentDrawStyle drawStyle, const WidgetStyle& ctrlStyle,
                                         WidgetState ctrlState) const
{
    if (drawStyle == SegmentDrawStyle::kNoDecoration) {
        return;
    }

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
    ui.dc.setStrokeColor(style.borderColor);
    ui.dc.drawLines({p1, p2});
}

const Theme::WidgetStyle& VectorBaseTheme::segmentTextStyle(WidgetState state, SegmentDrawStyle drawStyle,
                                                            bool isOn) const
{
    switch (drawStyle) {
        case SegmentDrawStyle::kNormal:
            if (isOn) {
                return mSegmentOnStyles[int(state)];
            } else {
                return mSegmentOffStyles[int(state)];
            }
        case SegmentDrawStyle::kNoDecoration:
            if (isOn) {
                return mSegmentUndecoratedOnStyles[int(state)];
            } else {
                return mSegmentUndecoratedOffStyles[int(state)];
            }
    }
    return mSegmentOffStyles[int(state)];  //for MSVC (don't use 'default' so we get warnings if enum changes)
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

void VectorBaseTheme::drawColorEdit(UIContext& ui, const Rect& frame, const Color& color,
                                    const WidgetStyle& style, WidgetState state) const
{
    auto frameStyle = mColorEditTrackStyles[int(state)].merge(style);  // copy
    drawFrame(ui, frame, frameStyle);

    auto marginVert = ui.dc.roundToNearestPixel(0.25f * mParams.labelFont.pointSize());
    ui.dc.setFillColor(color);
    ui.dc.drawRect(frame.insetted(2.0f * marginVert, marginVert), kPaintFill);
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
    const PicaPt kStrokeWidth(1.0f);
    const float kHalfWidthMultiplier = 0.35f;

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

    ui.dc.setStrokeWidth(kStrokeWidth);
    ui.dc.setStrokeEndCap(kEndCapRound);
    ui.dc.setStrokeJoinStyle(kJoinRound);

    const auto aspect = 0.625f;
    PicaPt hw;
    if (frame.height > frame.width) {
        hw = kHalfWidthMultiplier * (frame.width - 2.0f * kStrokeWidth);
    } else {
        auto h = frame.height;
        hw = kHalfWidthMultiplier * (h * aspect - 2.0f * kStrokeWidth);
    }
    ui.dc.setStrokeColor(mButtonStyles[int(incState)].fgColor);
    auto top = frame.midY() - 0.2f * frame.height;
    ui.dc.drawLines({ Point(frame.midX() - hw, top),
                      Point(frame.midX(), top - hw),
                      Point(frame.midX() + hw, top) });
    ui.dc.setStrokeColor(mButtonStyles[int(decState)].fgColor);
    auto bottom = frame.midY() + 0.2f * frame.height + hw;
    ui.dc.drawLines({ Point(frame.midX() - hw, bottom - hw),
                      Point(frame.midX(), bottom),
                      Point(frame.midX() + hw, bottom - hw) });
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
        auto imeConversion = editor.imeConversion();
        if (imeConversion.isEmpty()) {
            auto sel = editor.selection();
            selectionStart = editor.pointAtIndex(sel.start).x + textMargins.width + scrollOffset;
            selectionEnd = editor.pointAtIndex(sel.end).x + textMargins.width + scrollOffset;
        } else {
            selectionStart = editor.pointAtIndex(imeConversion.start + imeConversion.cursorOffset).x + textMargins.width + scrollOffset;
            selectionEnd = selectionStart;
        }
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

    if (editor.isEmpty() && editor.imeConversion().isEmpty()) {
        if (!placeholder.empty()) {
            ui.dc.setFillColor(mTextEditStyles[int(WidgetState::kDisabled)].fgColor);
            ui.dc.drawText(placeholder.c_str(), textRect, horizAlign | Alignment::kVCenter,
                           kWrapNone, mParams.labelFont, kPaintFill);
        }
    } else {
        // The layout incorporates the color, so we cannot set it.
        auto textRect = calcTextEditRectForFrame(frame, ui.dc, font);
        if (editor.layout()) {
            ui.dc.drawText(*editor.layout(), textRect.upperLeft() + Point(scrollOffset, textMargins.height));
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

void VectorBaseTheme::drawSearchBar(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                    WidgetState state) const
{
    drawFrame(ui, frame, mSearchBarStyles[int(state)].merge(style));
}

void VectorBaseTheme::drawSplitterThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                        WidgetState state) const
{
    ui.dc.setFillColor(mSplitterStyles[int(state)].merge(style).bgColor);
    ui.dc.drawRect(frame, kPaintFill);
}

void VectorBaseTheme::clipScrollView(UIContext& ui, const Rect& frame,
                                     const WidgetStyle& style, WidgetState state, bool drawsFrame) const
{
    auto s = mScrollViewStyles[int(state)].merge(style);
    if (!drawsFrame) {
        s.borderWidth = PicaPt::kZero;
        s.borderColor = Color::kTransparent;
    }
    clipFrame(ui, frame, s);
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
                   kWrapNone, mParams.labelFont, kPaintFill);
    if (itemAttr == MenuItemAttribute::kSubmenu) {
        auto itemMetrics = calcPreferredMenuItemMetrics(ui.dc, frame.height);
        Rect r(shortcutRect.maxX() - itemMetrics.submenuIconSize.width,
               shortcutRect.midY() - 0.5 * itemMetrics.submenuIconSize.height,
               itemMetrics.submenuIconSize.width,
               itemMetrics.submenuIconSize.height);
        drawSubmenuIcon(ui, r, s);
    } else {
        ui.dc.drawText(shortcutKey.c_str(), shortcutRect, Alignment::kRight | Alignment::kVCenter,
                       kWrapNone, mParams.labelFont, kPaintFill);
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

void VectorBaseTheme::drawMenuScrollArea(UIContext& ui, const Rect& frame, ScrollDir dir) const
{
#if __APPLE__
    auto icon = (dir == ScrollDir::kUp
                    ? Theme::StandardIcon::kTriangleUp
                    : Theme::StandardIcon::kTriangleDown);
    auto margin = std::max(PicaPt::fromStandardPixels(2),
                           ui.dc.roundToNearestPixel(0.15f * frame.height));
#else
    auto icon = (dir == ScrollDir::kUp
                    ? Theme::StandardIcon::kChevronUp
                    : Theme::StandardIcon::kChevronDown);
    auto margin = std::max(PicaPt::fromStandardPixels(2),
                           ui.dc.roundToNearestPixel(0.2f * frame.height));
#endif // __APPLE__
    drawIcon(ui, frame.insetted(PicaPt::kZero, margin), icon, mParams.textColor);
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
    // Draw background (if selected)
    drawFrame(ui, frame, s);
    
    // Draw text: inset by the margin
    auto horizMargin = calcPreferredMenubarItemHorizMargin(ui.dc, frame.height);
    Rect textFrame(frame.x + horizMargin, frame.y, frame.width - horizMargin, frame.height);
    ui.dc.setFillColor(s.fgColor);
    ui.dc.drawText(text.c_str(), textFrame, Alignment::kLeft | Alignment::kVCenter, kWrapNone,
                   mParams.nonNativeMenubarFont, kPaintFill);
}

void VectorBaseTheme::drawTooltip(UIContext& ui, const Rect& frame) const
{
    drawFrame(ui, frame, mTooltipStyle);
}

}  // namespace uitk
