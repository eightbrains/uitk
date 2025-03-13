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

#ifndef UITK_SCROLLVIEW_H
#define UITK_SCROLLVIEW_H

#include "Widget.h"

namespace uitk {

class ScrollView : public Widget {
    using Super = Widget;
public:
    ScrollView();
    ~ScrollView();

    ScrollView* setFrame(const Rect& frame) override;

    /// bounds().size() is the content size and bounds().upperLeft() is contentOffset
    const Rect& bounds() const override;
    ScrollView* setBounds(const Rect& bounds);

    ScrollView* setContentSize(const Size& size);
    /// Scrolls the content to offset. Note that (0, 0) is scrolled to the top
    /// and (0, -(bounds().height - frame().height)) is scrolled to the bottom
    ScrollView* setContentOffset(const Point& offset);

    /// Convenience function for calling setContentOffset(). The positive direction is as
    /// if you are pulling the scrollbar forward:  +dx is to the right (moves the
    /// bounds left), +dy is down (moves bounds up).
    void scroll(const PicaPt& dx, const PicaPt& dy);
    /// Convenience function for calling setContentOffset() The positive direction is as
    /// if you are pulling the scrollbar forward:  +dx is to the right (moves the
    /// bounds left), +dy is down (moves bounds up).
    void scrollTo(const PicaPt& x, const PicaPt& y);
    /// Convenience function for calling bounds(), but returns the result in the
    /// same coordinate system that scrollTo() uses.
    Point scrollPosition() const;

    Point convertToLocalFromParent(const Point& parentPt) const override;
    Point convertToParentFromLocal(const Point& localPt) const override;

    AccessibilityInfo accessibilityInfo() override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    EventResult mouse(const MouseEvent& e) override;

    void draw(UIContext& context) override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_SCROLLVIEW_H
