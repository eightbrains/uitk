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

#ifndef UITK_WIDGET_H
#define UITK_WIDGET_H

#include "themes/Theme.h"

#include <memory>
#include <vector>

namespace uitk {

class MouseEvent;
struct LayoutContext;
struct UIContext;

class Widget {
    friend class Window;
public:
    Widget();
    virtual ~Widget();

    virtual const Rect& frame() const;
    /// The frame's coordinates are relative to its parent.
    virtual Widget* setFrame(const Rect& frame);

    /// Convenience function that calls setFrame underneath.
    Widget* setPosition(const Point& p);
    /// Convenience function that calls setFrame underneath.
    Widget* setSize(const Size& size);

    virtual bool visible() const;
    virtual Widget* setVisible(bool vis);

    virtual bool enabled() const;
    virtual Widget* setEnabled(bool enabled);

    const Color& backgroundColor() const;
    Widget* setBackgroundColor(const Color& bg);

    const Color& borderColor() const;
    Widget* setBorderColor(const Color& color);

    const PicaPt borderWidth() const;
    Widget* setBorderWidth(const PicaPt& width);

    const PicaPt borderRadius() const;
    Widget* setBorderRadius(const PicaPt& radius);

    /// Adds widget as a child. The order that widgets are added is the order
    /// that they will be drawn. Takes ownership of the pointer. Returns a
    /// pointer to `this`.
    Widget* addChild(Widget *w);

    const std::vector<Widget*> children() const;

    /// returned by size calculations to indicate that the widget wants
    /// as much space as possible in that dimension.
    static const PicaPt kDimGrow;

    /// Returns the preferred size of the widget. If a dimension is
    /// >= kDimGrow, the widget wants as much space as it can get in that
    /// dimension. If you override this function, use
    /// context.dc.ceilToNearestPixel() to make sure that the size is
    /// a whole pixel.
    virtual Size preferredSize(const LayoutContext& context) const;

    /// Lays out children according to the frame. Not intended to be called
    /// directly.
    virtual void layout(const LayoutContext& context);

    enum class EventResult { kIgnored, kConsumed };

    virtual EventResult mouse(const MouseEvent& e);
    virtual void mouseEntered();
    virtual void mouseExited();

    /// Draws the widget. Classes inheriting from Widget must draw the frame
    /// themselves with Theme::drawFrame() or the desired equivalent. (This
    /// permits widgets to draw the frame differently if they desire.
    /// For example, the slider uses the frame styling to draw the slider
    /// track, but does not want the track to be the full frame.)
    virtual void draw(UIContext& context);

protected:
    Theme::WidgetState state() const;
    void setState(Theme::WidgetState state);

    Theme::WidgetStyle& style(Theme::WidgetState state);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_WIDGET_H
