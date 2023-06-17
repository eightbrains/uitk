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

#ifndef UITK_GET_BORDER_THEME_H
#define UITK_GET_BORDER_THEME_H

#include "Theme.h"

#include "../StringEditorLogic.h"

namespace uitk {

/// This is an internal class used to determine the border path for a given
/// item. It essentially forwards the calls to the real theme, but on the
/// assumption that the context is the fake DrawContext provided by
/// drawContext().
// Design note: this feels a little like a hack, instead of, say, having the
//     widget provide a focus shape. But since the widget is drawn by the
//     theme, this is not really feasible without requiring widget authors
//     to support keyboard navigation (and if they do not, people will blame
//     the library). One possibility is that we could have a WidgetType enum
//     which would allow for a calcBorderPath(WidgetType) function, but this
//     is clunky, and would require widgets to choose their type. At least
//     this way requires no code on the part of a Widget.
class GetBorderTheme : public Theme
{
public:
    enum class Type { kRect, kEllipse, kPath };
    struct FramePath {
        Type type = Type::kRect;
        Rect rect;
        PicaPt rectRadius = PicaPt(0.0f);
        std::shared_ptr<BezierPath> path;
    };

    ~GetBorderTheme() {}

    std::shared_ptr<DrawContext> drawContext(DrawContext& realDC);

    void setTheme(const Theme *theme)
    {
        mTheme = theme;
        mFrame = FramePath();
    }
    const FramePath& path() const { return mFrame; }

    const Params& params() const override { return mTheme->params(); }
    void setParams(const Params& params) override {}

    Size calcPreferredTextMargins(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcPreferredTextMargins(dc, font); }
    PicaPt calcStandardHeight(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcStandardHeight(dc, font); }
    Size calcStandardIconSize(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcStandardIconSize(dc, font); }
    Rect calcStandardIconRect(const DrawContext& dc, const Rect& frame, const Font& font) const override
        { return mTheme->calcStandardIconRect(dc, frame, font); }
    PicaPt calcStandardIconSeparator(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcStandardIconSeparator(dc, font); }
    Size calcPreferredButtonMargins(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcPreferredButtonMargins(dc, font); }
    Size calcPreferredCheckboxSize(const DrawContext& dc,
                                   const Font& font) const override
        { return mTheme->calcPreferredCheckboxSize(dc, font); }
    Size calcPreferredSegmentMargins(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcPreferredSegmentMargins(dc, font); }
    Size calcPreferredComboBoxSize(const DrawContext& dc,
                                   const PicaPt& preferredMenuWidth) const override
        { return mTheme->calcPreferredComboBoxSize(dc, preferredMenuWidth); }
    Size calcPreferredSliderThumbSize(const DrawContext& dc) const override
        { return mTheme->calcPreferredSliderThumbSize(dc); }
    Size calcPreferredProgressBarSize(const DrawContext& dc) const override
        { return mTheme->calcPreferredProgressBarSize(dc); }
    Size calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const override
        { return mTheme->calcPreferredTextEditSize(dc, font); }
    Rect calcTextEditRectForFrame(const Rect& frame, const DrawContext& dc, const Font& font) const override
        { return mTheme->calcTextEditRectForFrame(frame, dc, font); }
    Size calcPreferredIncDecSize(const DrawContext& dc) const override
        { return mTheme->calcPreferredIncDecSize(dc); }
    PicaPt calcPreferredScrollbarThickness(const DrawContext& dc) const override
        { return mTheme->calcPreferredScrollbarThickness(dc); }
    PicaPt calcPreferredSplitterThumbThickness(const DrawContext& dc) const override
        { return mTheme->calcPreferredSplitterThumbThickness(dc); }
    Size calcPreferredMenuItemSize(const DrawContext& dc,
                                   const std::string& text, const std::string& shortcut,
                                   MenuItemAttribute itemAttr,
                                   PicaPt *shortcutWidth) const override
        { return mTheme->calcPreferredMenuItemSize(dc, text, shortcut, itemAttr, shortcutWidth); }
    PicaPt calcMenuScrollAreaHeight(const DrawContext& dc) const override
        { return mTheme->calcMenuScrollAreaHeight(dc); }
    MenubarMetrics calcPreferredMenuItemMetrics(const DrawContext& dc, const PicaPt& height) const override
        { return mTheme->calcPreferredMenuItemMetrics(dc, height); }
    PicaPt calcPreferredMenuVerticalMargin() const override
        { return mTheme->calcPreferredMenuVerticalMargin(); }
    PicaPt calcPreferredMenubarItemHorizMargin(const DrawContext& dc, const PicaPt& height) const override
        { return mTheme->calcPreferredMenubarItemHorizMargin(dc, height); }

    void drawCheckmark(UIContext& ui, const Rect& r, const WidgetStyle& style) const override {}
    void drawSubmenuIcon(UIContext& ui, const Rect& frame, const WidgetStyle& style) const override {}

    void drawWindowBackground(UIContext& ui, const Size& size) const override {}
    void drawFrame(UIContext& ui, const Rect& frame,
                   const WidgetStyle& style) const override
        { mTheme->drawFrame(ui, frame, style); }
    void clipFrame(UIContext& ui, const Rect& frame,
                   const WidgetStyle& style) const override {}
    void drawFocusFrame(UIContext& ui, const Rect& frame, const PicaPt& radius) const override {}
    WidgetStyle labelStyle(const WidgetStyle& style, WidgetState state) const override
        { return mTheme->labelStyle(style, state); }
    void drawButton(UIContext& ui, const Rect& frame, ButtonDrawStyle buttonStyle,
                    const WidgetStyle& style, WidgetState state,
                    bool isOn) const override
        { mTheme->drawButton(ui, frame, buttonStyle, style, state, isOn);}
    const WidgetStyle& buttonTextStyle(WidgetState state, ButtonDrawStyle buttonStyle,
                                       bool isOn) const override
        { return mTheme->buttonTextStyle(state, buttonStyle, isOn); }
    void drawCheckbox(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state,
                      bool isOn) const override
        { mTheme->drawCheckbox(ui, frame, style, state, isOn); }
    void drawSegmentedControl(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                              const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawSegmentedControl(ui, frame, drawStyle, style, state); }
    void drawSegment(UIContext& ui, const Rect& frame, SegmentDrawStyle drawStyle,
                     WidgetState state,
                     bool isButton, bool isOn, bool showKeyFocus,
                     int segmentIndex, int nSegments) const override
        {}
    void drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                            SegmentDrawStyle drawStyle,
                            const WidgetStyle& ctrlStyle, WidgetState ctrlState) const override
        {}
    const WidgetStyle& segmentTextStyle(WidgetState state, SegmentDrawStyle drawStyle,
                                        bool isOn) const override
        { return mTheme->segmentTextStyle(state, drawStyle, isOn); }
    void drawColorEdit(UIContext& ui, const Rect& frame, const Color& color,
                       const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawColorEdit(ui, frame, color, style, state); }
    void drawComboBoxAndClip(UIContext& ui, const Rect& frame,
                             const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawComboBoxAndClip(ui, frame, style, state); }
    void drawSliderTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                         const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawSliderTrack(ui, dir, frame, thumbMid, style, state); }
    void drawSliderThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                         WidgetState state) const override
    {
        mFrame = FramePath();
        mTheme->drawSliderThumb(ui, frame, style, state);
    }
    void drawScrollbarTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                            const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawScrollbarTrack(ui, dir, frame, thumbMid, style, state); }
    void drawScrollbarThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                            WidgetState state) const override
        { mTheme->drawScrollbarThumb(ui, frame, style, state); }
    void drawProgressBar(UIContext& ui, const Rect& frame, float value,
                         const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawProgressBar(ui, frame, value, style, state); }
    void drawIncDec(UIContext& ui, const Rect& frame, WidgetState incState, WidgetState decState) const override
        { mTheme->drawIncDec(ui, frame, incState, decState); }
    WidgetStyle textEditStyle(const WidgetStyle& style, WidgetState state) const override
        { return mTheme->textEditStyle(style, state); }
    void drawTextEdit(UIContext& ui, const Rect& frame, const PicaPt& scrollOffset,
                      const std::string& placeholder, TextEditorLogic& editor, int horizAlign,
                      const WidgetStyle& style, WidgetState state, bool hasFocus) const override
    {
        StringEditorLogic fakeEdit;  // selection may draw a rectangle, don't want that
        mTheme->drawTextEdit(ui, frame, scrollOffset, placeholder, fakeEdit, horizAlign, style, state, hasFocus);
    }
    void drawSearchBar(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                               WidgetState state) const override
        { mTheme->drawSearchBar(ui, frame, style, state); }
    void drawSplitterThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                           WidgetState state) const override
        { mTheme->drawSplitterThumb(ui, frame, style, state); }
    void clipScrollView(UIContext& ui, const Rect& frame,
                        const WidgetStyle& style, WidgetState state, bool drawsFrame) const override
        { mTheme->clipScrollView(ui, frame, style, state, true); }
    void drawScrollView(UIContext& ui, const Rect& frame,
                        const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawScrollView(ui, frame, style, state); }
    void drawListView(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state) const override
        { mTheme->drawListView(ui, frame, style, state); }
    void clipListView(UIContext& ui, const Rect& frame,
                      const WidgetStyle& style, WidgetState state) const override
        { mTheme->clipListView(ui, frame, style, state); }
    void drawListViewSpecialRow(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style, WidgetState state) const override
        {}
    void drawMenuBackground(UIContext& ui, const Size& size) override {}
    void calcMenuItemFrames(const DrawContext& dc, const Rect& frame, const PicaPt& shortcutWidth,
                            Rect *checkRect, Rect *textRect, Rect *shortcutRect) const override
        { return mTheme->calcMenuItemFrames(dc, frame, shortcutWidth, checkRect, textRect, shortcutRect); }
    void drawMenuItem(UIContext& ui, const Rect& frame, const PicaPt& shortcutWidth,
                      const std::string& text, const std::string& shortcutKey,
                      MenuItemAttribute itemAttr,
                      const WidgetStyle& style, WidgetState state) const override
        {}
    void drawMenuSeparatorItem(UIContext& ui, const Rect& frame) const override
        {}
    void drawMenuScrollArea(UIContext& ui, const Rect& frame, ScrollDir dir) const override
        {}
    void drawMenubarBackground(UIContext& ui, const Rect& frame) const override
        {}
    void drawMenubarItem(UIContext& ui, const Rect& frame, const std::string& text,
                         WidgetState state) const override
        {}
    void drawTooltip(UIContext& ui, const Rect& frame) const override {}

protected:
    const Theme *mTheme = nullptr;
    mutable FramePath mFrame;
};
}  // namespace uitk
#endif // UITK_GET_BORDER_THEME_H

