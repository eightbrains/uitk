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

struct KeyEvent;
struct MouseEvent;
struct TextEvent;
struct LayoutContext;
struct UIContext;
class Window;

class Widget {
    friend class Window;
public:
    Widget();
    virtual ~Widget();

    /// Ensures that the widget is redrawn
    void setNeedsDraw();

    virtual const Rect& frame() const;
    /// The frame's coordinates are relative to its parent
    virtual Widget* setFrame(const Rect& frame);

    /// Convenience function that calls setFrame underneath
    Widget* setPosition(const Point& p);
    /// Convenience function that calls setFrame underneath
    Widget* setSize(const Size& size);

    /// The bounds' coordinates are relative to the upper left of the widget
    virtual const Rect& bounds() const;

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

    /// Removes the widget and returns ownership to the caller.
    /// Returns a pointer to `w` (which the caller could use to `delete`,
    /// for instance). This function is O(n).
    Widget* removeChild(Widget *w);

    /// Removes all child widgets and returns ownership to the caller.
    /// This more efficient than calling removeChild(), but does require
    /// the call to have stored pointers to all the children.
    void removeAllChildren();

    /// Removes all the children and deletes them.
    void clearAllChildren();

    const std::vector<Widget*>& children() const;

    /// Returns the parent of this widget, or nullptr
    Widget* parent() const;

    /// Returns the Window that owns this widget, or nullptr
    Window* window() const;

    Point convertToLocalFromWindow(const Point& windowPt) const;
    Point convertToWindowFromLocal(const Point& localPt) const;

    bool focused() const;
    void resignKeyFocus() const;

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

    virtual void key(const KeyEvent& e);
    virtual void keyFocusStarted();
    virtual void keyFocusEnded();

    virtual void text(const TextEvent& e);

    /// Draws the widget. Classes inheriting from Widget must draw the frame
    /// themselves with Theme::drawFrame() or the desired equivalent. (This
    /// permits widgets to draw the frame differently if they desire.
    /// For example, the slider uses the frame styling to draw the slider
    /// track, but does not want the track to be the full frame.)
    virtual void draw(UIContext& context);

    std::string debugDescription(const Point& offset = Point(PicaPt::kZero, PicaPt::kZero),
                                 int indent = 0) const;

public:
    enum MouseState { kNormal = 0, kDisabled, kMouseOver, kMouseDown };
    MouseState state() const;
    /// Merges MouseState with any other state that the widget may have (notably,
    /// selection) to produce the state the widget should use to draw. If setThemeState()
    /// has been called, that state will be used.
    virtual Theme::WidgetState themeState() const;
    Theme::WidgetStyle& style(Theme::WidgetState state);

    /// Forces a widget state for drawing. This can be useful when using a widget
    /// as a child to reuse drawing, instead of reusing functionality. For example,
    /// Button uses Label to draw, but the label should take the button's theme state.
    /// Contrast with NumericEdit, which uses IncDecWidget as a child, but in that case
    /// the child keeps its own drawing state because it functions as widget.
    void setThemeState(Theme::WidgetState state);

    /// Sets theme state back to unset, to undo a call to setThemeState().
    void resetThemeState();

protected:
    // Return true if parent's mouse() should grab the widget if mouse down is consumed.
    // Default implementation returns true.
    virtual bool shouldAutoGrab() const;

    void setWindow(Window* window);  // for Window

    void setState(MouseState state);

    EventResult mouseChild(const MouseEvent& e, Widget *child, EventResult result); // e is child's parent's event
    void drawChild(UIContext& context, Widget *child);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_WIDGET_H
