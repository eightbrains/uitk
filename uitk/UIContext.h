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
        PicaPt width = PicaPt(10000);
        PicaPt height = PicaPt(10000);
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
    bool isWindowActive;
};

}  // namespace uitk
#endif // UITK_UICONTEXT_H
