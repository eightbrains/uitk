//-----------------------------------------------------------------------------
// Copyright 2021 - 2025 Eight Brains Studios, LLC
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

/// Scrollable area. Add widgets with content()->addChild(), and make sure
/// to call setContentSize() in ScrollView parent's layout().
// Design note:
//   It seems like it would be elegant to be able to add widgets with addChild().
//   However, there are several problems with this:
//     1) it is counter-intuitive once you understand how a scroll view must
//        work (that is, putting everything in a container widget and moving
//        that around when you scroll, or at least drawing everything with an
//        offset, which an earlier version of this class tried).
//     2) it is rarely useful, because you need to be able to layout the
//        widgets, so you need to have a container widget of your own.
//        (Arguably this a consequence of not separating Widgets and Layouts
//        like Qt does, but I have always found that awkward.)
//     3) converting a coordinate from parent to child (e.g. mouse movement)
//        is troublesome, since scroll->convertFromParentToLocal() would need
//        to know which child of the ScrollView the next call will do, since
//        both the scrollbars and the scroll-content widgets are equally
//        ScrollView's children.
//   Exporting the ScrollView's implementation detail of a content widget
//   solves all these problems.
class ScrollView : public Widget {
    using Super = Widget;
public:
    ScrollView();
    ~ScrollView();

    /// Returns the content widget. Use content()->addChild() to add widgets
    /// to the ScrollView's scrollable area, not addChild().
    Widget* content() const;

    /// Returns the content view area. This includes the area under the scrollbars
    /// if this platform autohides them, otherwise it does not. (This is not the
    /// same thing as content().frame()!) This value is not usable until both
    /// the ScrollView's frame and the content size have been set.
    Rect contentRect() const;

    ScrollView* setFrame(const Rect& frame) override;

    /// bounds().size() is the content size and bounds().upperLeft() is contentOffset
    const Rect& bounds() const override;
    ScrollView* setBounds(const Rect& bounds);

    /// Sets the content size. This is required.
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

    AccessibilityInfo accessibilityInfo() override;

    Size preferredSize(const LayoutContext& context) const override;
    void layout(const LayoutContext& context) override;

    EventResult mouse(const MouseEvent& e) override;
    void mouseExited() override;

    void draw(UIContext& context) override;

protected:
    bool isMouseInScrollbar() const;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk
#endif // UITK_SCROLLVIEW_H
