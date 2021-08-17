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

#ifndef UITK_THEME_H
#define UITK_THEME_H

#include "../Global.h"

#include <nativedraw.h>

namespace uitk {

struct LayoutContext;
class TextEditorLogic;
struct UIContext;

class Theme {
public:
    enum class WidgetState { kNormal = 0, kDisabled, kMouseOver, kMouseDown };

    struct WidgetStyle {
        enum Flags { kNoneSet = 0,
                     kBGColorSet = (1 << 0),
                     kFGColorSet = (1 << 1),
                     kBorderColorSet = (1 << 2),
                     kBorderWidthSet = (1 << 3),
                     kBorderRadiusSet = (1 << 4)
        };

        int flags = kNoneSet;
        Color bgColor;
        Color fgColor;
        Color borderColor;
        PicaPt borderWidth = PicaPt(0);
        PicaPt borderRadius = PicaPt(0);

        WidgetStyle merge(const WidgetStyle& s) const
        {
            WidgetStyle newStyle;

            if (s.flags & kBGColorSet) {
                newStyle.bgColor = s.bgColor;
            } else {
                newStyle.bgColor = this->bgColor;
            }

            if (s.flags & kFGColorSet) {
                newStyle.fgColor = s.fgColor;
            } else {
                newStyle.fgColor = this->fgColor;
            }

            if (s.flags & kBorderColorSet) {
                newStyle.borderColor = s.borderColor;
            } else {
                newStyle.borderColor = this->borderColor;
            }

            if (s.flags & kBorderWidthSet) {
                newStyle.borderWidth = s.borderWidth;
            } else {
                newStyle.borderWidth = this->borderWidth;
            }
            
            if (s.flags & kBorderRadiusSet) {
                newStyle.borderRadius = s.borderRadius;
            } else {
                newStyle.borderRadius = this->borderRadius;
            }

            return newStyle;
        }
    };

    struct Params
    {
        Color windowBackgroundColor;
        Color nonEditableBackgroundColor;
        Color editableBackgroundColor;
        Color disabledBackgroundColor;
        Color textColor;
        Color disabledTextColor;
        Color accentedBackgroundTextColor;  // for when accentColor is bg of text
        Color accentColor;
        Color selectionColor;
        Font labelFont;
    };

public:
    Theme() {}
    virtual ~Theme() {}

    virtual const Params& params() const = 0;
    virtual void setParams(const Params& params) = 0;

    /// The text margin vertically is around capHeight, NOT ascent + descent.
    /// So in more traditional terms, the margin is (margin - descent) below the
    /// descender, and (margin) above (baseline + capHeight). Note that baseline + ascent
    /// is can be substantially above the top of the text, and seems to act like
    /// leading (which in these fonts is usually zero).
    virtual Size calcPreferredTextMargins(const DrawContext& dc, const Font& font) const = 0;
    virtual Size calcPreferredButtonSize(const DrawContext& dc, const Font& font,
                                         const std::string& text) const = 0;
    virtual Size calcPreferredCheckboxSize(const DrawContext& dc,
                                           const Font& font) const = 0;
    virtual Size calcPreferredSegmentSize(const DrawContext& dc, const Font& font,
                                          const std::string& text) const = 0;
    virtual Size calcPreferredComboBoxSize(const DrawContext& dc, const PicaPt& preferredMenuWidth) const = 0;
    virtual Size calcPreferredSliderThumbSize(const DrawContext& dc) const = 0;
    virtual Size calcPreferredProgressBarSize(const DrawContext& dc) const = 0;
    virtual Size calcPreferredTextEditSize(const DrawContext& dc, const Font& font) const = 0;
    virtual Rect calcTextEditRectForFrame(const Rect& frame, const DrawContext& dc,
                                          const Font& font) const = 0;
    virtual PicaPt calcPreferredScrollbarThickness(const DrawContext& dc) const = 0;
    virtual Size calcPreferredMenuItemSize(const DrawContext& dc, const std::string& text) const = 0;

    virtual void drawCheckmark(UIContext& ui, const Rect& r, const WidgetStyle& style) const = 0;

    virtual void drawWindowBackground(UIContext& ui, const Size& size) const = 0;
    virtual void drawFrame(UIContext& ui, const Rect& frame,
                           const WidgetStyle& style) const = 0;
    virtual void clipFrame(UIContext& ui, const Rect& frame,
                           const WidgetStyle& style) const = 0;
    virtual void drawButton(UIContext& ui, const Rect& frame,
                            const WidgetStyle& style, WidgetState state,
                            bool isOn) const = 0;
    virtual const WidgetStyle& buttonTextStyle(WidgetState state, bool isOn) const = 0;
    virtual void drawCheckbox(UIContext& ui, const Rect& frame,
                              const WidgetStyle& style, WidgetState state,
                              bool isOn) const = 0;
    virtual void drawSegmentedControl(UIContext& ui, const Rect& frame,
                                      const WidgetStyle& style,
                                      WidgetState state) const = 0;
    virtual void drawSegment(UIContext& ui, const Rect& frame, WidgetState state,
                             bool isButton, bool isOn,
                             int segmentIndex, int nSegments) const = 0;
    virtual void drawSegmentDivider(UIContext& ui, const Point& top, const Point& bottom,
                                    const WidgetStyle& ctrlStyle,
                                    WidgetState ctrlState) const = 0;
    virtual const WidgetStyle& segmentTextStyle(WidgetState state, bool isOn) const = 0;
    virtual void drawComboBoxAndClip(UIContext& ui, const Rect& frame,
                                     const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawSliderTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                                 const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawSliderThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                 WidgetState state) const = 0;
    virtual void drawScrollbarTrack(UIContext& ui, SliderDir dir, const Rect& frame, const Point& thumbMid,
                                    const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawScrollbarThumb(UIContext& ui, const Rect& frame, const WidgetStyle& style,
                                    WidgetState state) const = 0;
    virtual void drawProgressBar(UIContext& ui, const Rect& frame, float value,
                                 const WidgetStyle& style, WidgetState state) const = 0;
    virtual WidgetStyle textEditStyle(const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawTextEdit(UIContext& ui, const Rect& frame, const PicaPt& scrollOffset,
                              const std::string& placeholder, TextEditorLogic& editor,
                              const WidgetStyle& style, WidgetState state, bool hasFocus) const = 0;
    virtual void clipScrollView(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawScrollView(UIContext& ui, const Rect& frame,
                                const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawListView(UIContext& ui, const Rect& frame,
                              const WidgetStyle& style, WidgetState state) const = 0;
    virtual void clipListView(UIContext& ui, const Rect& frame,
                              const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawListViewSelectedRow(UIContext& ui, const Rect& frame,
                                         const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawMenuBackground(UIContext& ui, const Rect& frame) = 0;
    virtual void calcMenuItemFrames(const DrawContext& dc, const Rect& frame,
                                    Rect *checkRect, Rect *textRect) const = 0;
    virtual void drawMenuItem(UIContext& ui, const Rect& frame, const std::string& text,
                              const bool isChecked, const WidgetStyle& style, WidgetState state) const = 0;
    virtual void drawMenuSeparatorItem(UIContext& ui, const Rect& frame) const = 0;
};

}  // namespace uitk
#endif // UITK_THEME_H
