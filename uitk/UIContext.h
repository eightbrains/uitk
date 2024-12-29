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

#ifndef UITK_UICONTEXT_H
#define UITK_UICONTEXT_H

#define ND_NAMESPACE uitk
#include <nativedraw.h>

namespace uitk {

class Theme;
class DrawContext;

struct LayoutContext
{
    const Theme& theme;
    const DrawContext& dc;

    /// Used by Widget::preferredSized() to return the preferred size in
    /// a constrained condition, for example a large image displayed with
    /// a fixed width, or a long piece of text, may return a different height
    /// if the width is constrained. Note that these are not necessarily set
    /// in Widget::layout(), which should use Widget::bounds() to retrieve
    /// sizes.
    struct {
        PicaPt width = PicaPt::fromPixels(32000.0f, 72.0f);
        PicaPt height = PicaPt::fromPixels(32000.0f, 72.0f);
    } constraints;

    LayoutContext withWidth(const PicaPt& w) const {
        auto copy = *this;
        if (copy.constraints.width > w) {  // don't use std::min() in headers
            copy.constraints.width = w;
        }
        return copy;
    }

    LayoutContext withHeight(const PicaPt& h) const {
        auto copy = *this;
        if (copy.constraints.height > h) {  // don't use std::min() in headers
            copy.constraints.height = h;
        }
        return copy;
    }
};

struct UIContext
{
    const Theme& theme;
    DrawContext& dc;
    /// This is the rectangle that is expected to draw in, in widget-local
    /// coordinates (that is, (0, 0) is (0, 0) of this widget). Generally
    /// this is the same as the bounds of the widget, however it might be
    /// smaller if the widget is in a ScrollView (or something that is
    /// a scrollview, like a ListView). Widget will not draw its children
    /// if they do not intersect the drawRect, so you can normally ignore
    /// this unless you are drawing lots of things manually.
    Rect drawRect;
    bool isWindowActive;
};

struct PrintContext : public UIContext
{
    Size paperSize;
    /// The imageableRect is advisory; the OS has been known to provide an
    /// incorrect value. Generally applications will print using the
    /// UIContext.drawRect, which is the full page, and use whatever
    /// margins the user specifies. But this can be used to warn if the
    /// margins may exceed the printable area.
    Rect imageableRect;
    int pageIndex;  /// 0-based;  the page number is pageIndex + 1
};

}  // namespace uitk
#endif // UITK_UICONTEXT_H
