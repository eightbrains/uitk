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

#ifndef UITK_VECTOR_BASE_THEME_H
#define UITK_VECTOR_BASE_THEME_H

#include "Theme.h"

namespace uitk {

class VectorBaseTheme : public Theme
{
public:
    explicit VectorBaseTheme(const Params& params, const PicaPt& borderWidth,
                             const PicaPt& borderRadius);

    const Params& params() const override;
    void setParams(const Params& params) override;

    Size calcPreferredTextMargins(const DrawContext& dc, const Font& font) const override;
    Size calcPreferredButtonSize(const DrawContext& dc, const Font& font,
                                 const std::string& text) const override;
    // Returns the size for the checkbox, not the checkbox + text
    Size calcPreferredCheckboxSize(const DrawContext& dc,
                                   const Font& font) const override;
    Size calcPreferredSegmentSize(const DrawContext& dc, const Font& font,
                                  const std::string& text) const override;
    Size calcPreferredComboBoxSize(const DrawContext& dc,
                                   const PicaPt& preferredMenuWidth) const override;
    Size calcPreferredSliderThumbSize(const DrawContext& dc) const override;
    Size calcPreferredProgressBarSize(const DrawContext& dc) const override;
    Size calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const override;
    Rect calcTextEditRectForFrame(const Rect& frame, const DrawContext& dc, const Font& font) const override;
    Size calcPreferredIncDecSize(const DrawContext& dc) const override;
    PicaPt calcPreferredScrollbarThickness(const DrawContext& dc) const override;
    Size calcPreferredMenuItemSize(const DrawContext& dc,
                                   const std::string& text, const std::string& shortcut,
                                   MenuItemAttribute itemAttr,
                                   PicaPt *shortcutWidth) const override;
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
    WidgetStyle labelStyle(const WidgetStyle& style, WidgetState state) const override;
    void drawButton(UIContext& ui, const Rect& frame, ButtonDrawStyle buttonStyle,
                    const WidgetStyle& style, WidgetState state,
                    bool isOn) const override;
    const WidgetStyle& buttonTextStyle(WidgetState state, bool isOn) const override;
    void drawCheckbox(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state,
                      bool isOn) const override;
    void drawSegmentedControl(UIContext& ui,
                              const Rect& frame,
                              const WidgetStyle& style,
                              WidgetState state) const override;
    void drawSegment(UIContext& ui, const Rect& frame, WidgetState state,
                     bool isButton, bool isOn,
                     int segmentIndex, int nSegments) const override;
    void drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                            const WidgetStyle& ctrlStyle,
                            WidgetState ctrlState) const override;
    const WidgetStyle& segmentTextStyle(WidgetState state, bool isOn) const override;
    void drawComboBoxAndClip(UIContext& ui, const Rect& frame,
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
    WidgetStyle textEditStyle(const WidgetStyle& style, WidgetState state) const override;
    void drawTextEdit(UIContext& ui, const Rect& frame, const PicaPt& scrollOffset,
                      const std::string& placeholder, TextEditorLogic& editor, int horizAlign, 
                      const WidgetStyle& style, WidgetState state, bool hasFocus) const override;
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
    void drawMenubarBackground(UIContext& ui, const Rect& frame) const override;
    void drawMenubarItem(UIContext& ui, const Rect& frame, const std::string& text,
                         WidgetState state) const override;

protected:
    void setVectorParams(const Params& params);

protected:
    Params mParams;
    PicaPt mBorderWidth;
    PicaPt mBorderRadius;

    WidgetStyle mLabelStyles[5];
    WidgetStyle mButtonStyles[5];
    WidgetStyle mButtonOnStyles[5];
    WidgetStyle mButtonDefaultDialogStyles[5];
    WidgetStyle mCheckboxStyles[5];
    WidgetStyle mCheckboxOnStyles[5];
    WidgetStyle mSegmentedControlStyles[5];  // style for the background
    WidgetStyle mSegmentStyles[5];  // style for individual segment (button-style)
    WidgetStyle mSegmentOffStyles[5];  // style for individual segment (off)
    WidgetStyle mSegmentOnStyles[5];  // style for individual segment (on)
    WidgetStyle mComboBoxStyles[5];
    WidgetStyle mComboBoxIconAreaStyles[5];
    WidgetStyle mSliderTrackStyles[5];
    WidgetStyle mSliderThumbStyles[5];
    WidgetStyle mScrollbarTrackStyles[5];
    WidgetStyle mScrollbarThumbStyles[5];
    WidgetStyle mProgressBarStyles[5];
    WidgetStyle mTextEditStyles[5];
    WidgetStyle mScrollViewStyles[5];
    WidgetStyle mListViewStyles[5];
    WidgetStyle mMenuItemStyles[5];
    WidgetStyle mMenubarItemStyles[5];
};

}  // namespace uitk
#endif // UITK_VECTOR_BASE_THEME_H
