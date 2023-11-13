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

#ifndef UITK_VECTOR_BASE_THEME_H
#define UITK_VECTOR_BASE_THEME_H

#include "Theme.h"

namespace uitk {

class VectorBaseTheme : public Theme
{
public:
    explicit VectorBaseTheme(const Params& params);

    const Params& params() const override;
    void setParams(const Params& params) override;

    Size calcPreferredTextMargins(const DrawContext& dc, const Font& font) const override;
    PicaPt calcStandardHeight(const DrawContext& dc, const Font& font) const override;
    Size calcStandardIconSize(const DrawContext& dc, const Font& font) const override;
    Rect calcStandardIconRect(const DrawContext& dc, const Rect& frame, const Font& font) const override;
    PicaPt calcStandardIconSeparator(const DrawContext& dc, const Font& font) const override;
    Size calcPreferredButtonMargins(const DrawContext& dc, const Font& font) const override;
    // Returns the size for the checkbox, not the checkbox + text
    Size calcPreferredCheckboxSize(const DrawContext& dc,
                                   const Font& font) const override;
    Size calcPreferredSegmentMargins(const DrawContext& dc, const Font& font) const override;
    Size calcPreferredComboBoxSize(const DrawContext& dc,
                                   const PicaPt& preferredMenuWidth) const override;
    Size calcPreferredSliderThumbSize(const DrawContext& dc) const override;
    Size calcPreferredProgressBarSize(const DrawContext& dc) const override;
    Size calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const override;
    Rect calcTextEditRectForFrame(const Rect& frame, const DrawContext& dc, const Font& font) const override;
    Size calcPreferredIncDecSize(const DrawContext& dc) const override;
    PicaPt calcPreferredScrollbarThickness(const DrawContext& dc) const override;
    PicaPt calcPreferredSplitterThumbThickness(const DrawContext& dc) const override;
    Size calcPreferredMenuItemSize(const DrawContext& dc,
                                   const std::string& text, const std::string& shortcut,
                                   MenuItemAttribute itemAttr,
                                   PicaPt *shortcutWidth) const override;
    PicaPt calcMenuScrollAreaHeight(const DrawContext& dc) const override;
    MenubarMetrics calcPreferredMenuItemMetrics(const DrawContext& dc, const PicaPt& height) const override;
    PicaPt calcPreferredMenuVerticalMargin() const override;
    PicaPt calcPreferredMenubarItemHorizMargin(const DrawContext& dc, const PicaPt& height) const override;

    void drawCheckmark(UIContext& ui, const Rect& r, const WidgetStyle& style) const override;
    void drawSubmenuIcon(UIContext& ui, const Rect& frame, const WidgetStyle& style) const override;

    void drawWindowBackground(UIContext& ui, const Size& size) const override;
    void drawFrame(UIContext& ui, const Rect& frame,
                   const WidgetStyle& style) const override;
    void clipFrame(UIContext& ui, const Rect& frame,
                   const WidgetStyle& style) const override;
    void drawFocusFrame(UIContext& ui, const Rect& frame, const PicaPt& radius) const override;
    //void drawFocusFrame(UIContext& ui, const std::shared_ptr<BezierPath> path) const override;
    WidgetStyle labelStyle(UIContext& ui, const WidgetStyle& style, WidgetState state) const override;
    void drawButton(UIContext& ui, const Rect& frame, ButtonDrawStyle buttonStyle,
                    const WidgetStyle& style, WidgetState state,
                    bool isOn) const override;
    const WidgetStyle& buttonTextStyle(UIContext& ui, WidgetState state, ButtonDrawStyle buttonStyle,
                                       bool isOn) const override;
    void drawCheckbox(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state,
                      bool isOn) const override;
    void drawSegmentedControl(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                              const WidgetStyle& style, WidgetState state) const override;
    void drawSegment(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                     WidgetState state,
                     bool isButton, bool isOn, bool showKeyFocus,
                     int segmentIndex, int nSegments) const override;
    void drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                            SegmentDrawStyle drawStyle,
                            const WidgetStyle& ctrlStyle, WidgetState ctrlState) const override;
    const WidgetStyle& segmentTextStyle(UIContext& ui, WidgetState state, SegmentDrawStyle drawStyle,
                                        bool isOn) const override;
    void drawComboBoxAndClip(UIContext& ui, const Rect& frame,
                             const WidgetStyle& style, WidgetState state) const override;
    void drawColorEdit(UIContext& ui, const Rect& frame, const Color& color,
                       const WidgetStyle& style, WidgetState state) const override;
    void drawSliderTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                         const WidgetStyle& style, WidgetState state) const override;
    void drawSliderThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                         WidgetState state) const override;
    void drawScrollbarTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                            const WidgetStyle& style, WidgetState state) const override;
    void drawScrollbarThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                            WidgetState state) const override;
    void drawProgressBar(UIContext& ui, const Rect& frame, float value,
                         const WidgetStyle& style, WidgetState state) const override;
    void drawIncDec(UIContext& ui, const Rect& frame, WidgetState incState, WidgetState decState) const override;
    WidgetStyle textEditStyle(UIContext& ui, const WidgetStyle& style,
                              WidgetState state) const override;
    void drawTextEdit(UIContext& ui, const Rect& frame, const PicaPt& scrollOffset,
                      const std::string& placeholder, TextEditorLogic& editor, int horizAlign, 
                      const WidgetStyle& style, WidgetState state, bool hasFocus) const override;
    void drawSearchBar(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                               WidgetState state) const override;
    void drawSplitterThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                           WidgetState state) const override;
    void clipScrollView(UIContext& ui, const Rect& frame,
                        const WidgetStyle& style, WidgetState state, bool drawsFrame) const override;
    void drawScrollView(UIContext& ui, const Rect& frame,
                        const WidgetStyle& style, WidgetState state) const override;
    void drawListView(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state) const override;
    void clipListView(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state) const override;
    void drawListViewSpecialRow(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style, WidgetState state) const override;
    void drawMenuBackground(UIContext& ui, const Size& size) override;
    void calcMenuItemFrames(const DrawContext& dc, const Rect& frame, const PicaPt& shortcutWidth,
                            Rect *checkRect, Rect *textRect, Rect *shortcutRect) const override;
    void drawMenuItem(UIContext& ui, const Rect& frame, const PicaPt& shortcutWidth,
                      const std::string& text, const std::string& shortcutKey,
                      MenuItemAttribute itemAttr,
                      const WidgetStyle& style, WidgetState state) const override;
    void drawMenuSeparatorItem(UIContext& ui, const Rect& frame) const override;
    void drawMenuScrollArea(UIContext& ui, const Rect& frame, ScrollDir dir) const override;
    void drawMenubarBackground(UIContext& ui, const Rect& frame) const override;
    void drawMenubarItem(UIContext& ui, const Rect& frame, const std::string& text,
                         WidgetState state) const override;
    void drawTooltip(UIContext& ui, const Rect& frame) const override;

protected:
    void setVectorParams(const Params& params);

protected:
    Params mParams;

    WidgetStyle mLabelStyles[6];
    WidgetStyle mButtonStyles[6];
    WidgetStyle mButtonOnStyles[6];
    WidgetStyle mButtonUndecoratedStyles[6];
    WidgetStyle mButtonUndecoratedOnStyles[6];
    WidgetStyle mButtonAccessoryStyles[6];
    WidgetStyle mButtonDefaultDialogStyles[6];
    WidgetStyle mCheckboxStyles[6];
    WidgetStyle mCheckboxOnStyles[6];
    WidgetStyle mSegmentedControlStyles[6];  // style for the background
    WidgetStyle mSegmentStyles[6];  // style for individual segment (button-style)
    WidgetStyle mSegmentOffStyles[6];  // style for individual segment (off)
    WidgetStyle mSegmentOnStyles[6];  // style for individual segment (on)
    WidgetStyle mSegmentUndecoratedStyles[6];  // style for individual segment (button-style)
    WidgetStyle mSegmentUndecoratedOffStyles[6];  // style for individual segment (off)
    WidgetStyle mSegmentUndecoratedOnStyles[6];  // style for individual segment (on)
    WidgetStyle mComboBoxStyles[6];
    WidgetStyle mComboBoxIconAreaStyles[6];
    WidgetStyle mColorEditTrackStyles[6];
    WidgetStyle mSliderTrackStyles[6];
    WidgetStyle mSliderThumbStyles[6];
    WidgetStyle mScrollbarTrackStyles[6];
    WidgetStyle mScrollbarThumbStyles[6];
    WidgetStyle mProgressBarStyles[6];
    WidgetStyle mTextEditStyles[6];
    WidgetStyle mSearchBarStyles[6];
    WidgetStyle mSplitterStyles[6];
    WidgetStyle mScrollViewStyles[6];
    WidgetStyle mListViewStyles[6];
    WidgetStyle mMenuItemStyles[6];
    WidgetStyle mMenubarItemStyles[6];
    WidgetStyle mTooltipStyle;
};

}  // namespace uitk
#endif // UITK_VECTOR_BASE_THEME_H
