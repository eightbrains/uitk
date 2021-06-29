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

#ifndef UITK_SCROLLBAR_H
#define UITK_SCROLLBAR_H

#include "SliderLogic.h"

namespace uitk {

class ScrollBar : public SliderLogic {
    using Super = SliderLogic;
public:
    explicit ScrollBar(Dir dir);
    ~ScrollBar();

    /// Sets the scrollbar's minimum and maximum values, as well as its
    /// viewing size. The viewing size is the size of the viewing area in the
    /// scrollbar's dimension, basical the size of the window onto the content.
    /// This determines the width of the scroll thumb (if the theme supports it).
    ScrollBar* setRange(double minValue, double maxValue,
                        double viewingSize, double contentSize);

    Widget* setFrame(const Rect& frame) override;

    Size preferredSize(const LayoutContext& context) const override;

protected:
    Size preferredThumbSize(const LayoutContext& context) const override;
    void drawTrack(UIContext& context, const Point& thumbMid) override;
    void drawThumb(UIContext& context, Widget *thumb) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_SCROLLBAR_H
