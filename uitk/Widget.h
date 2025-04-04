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

#ifndef UITK_WIDGET_H
#define UITK_WIDGET_H

#include "Accessibility.h"
#include "CutPasteable.h"
#include "themes/Theme.h"

#include <memory>
#include <vector>

namespace uitk {

class EditableText;
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

    /// Ensures that the widget is re-laidout.
    void setNeedsLayout();

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

    const std::string& tooltip() const;
    /// Sets the tooltip text. This intentionally does not accept a RichText,
    /// since tooltips should be simple and understated. If you need a custom
    /// tooltip, override onTooltip() and hasTooltip() if necessary.
    Widget* setTooltip(const std::string& tooltip);

    const std::string& accessibilityText() const;
    /// Sets the text for the widget. This is generally not needed for
    /// individual widgets, which generally use their text. However,
    /// lists and other grouped elements benefit from accessibility
    /// otherwise they group may be ignored and the elements placed
    /// in the parent's space. A special case are widgets like
    /// SegmentedControl, which have (or emulate) child widgets, but
    /// for which it is sometimes better to have the items in a group
    /// and sometimes better to have them as siblings of the. However,
    /// it is still generally better easier to navigate if the group
    /// is labeled: "font settings" is a lot easier to navigate (and skip
    /// if you are not interested) than { "bold", "italic", "underline" }.
    Widget* setAccessibilityText(const std::string& text);

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
    // Design note: this cannot be virtual so that it can be used in
    // constructors.
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

    Point convertToLocalFromParent(const Point& parentPt) const;
    Point convertToParentFromLocal(const Point& localPt) const;

    bool focused() const;
    void resignKeyFocus() const;

    /// When using widgets like editable text as a child, it may be
    /// desireable to have the focus ring display around the parent.
    /// Defaults to false.
    void setShowFocusRingOnParent(bool show);
    bool showFocusRingOnParent() const;

    /// Returns true if the widget can accept key focus, false otherwise.
    /// The default implementation returns false. Implementations
    /// do not need to consider if the widget is disabled or hidden.
    virtual bool acceptsKeyFocus() const;

    /// Returns the accesibility information for the widget.
    /// Does not modify the object, but is non-const because further actions
    /// using the AccessibilityInfo object may modify the object.
    virtual AccessibilityInfo accessibilityInfo();
    
    /// Objects that support cut and paste should override this and
    /// return this interface. This is used by the copy/cut/paste menu items.
    /// The base class action returns nullptr, which disables the items.
    virtual CutPasteable* asCutPasteable();

    // If the widget has text that is editable, it should return the
    // interface here. Although key events are sufficient for languages
    // like English, many languages have text systems that require conversion
    // of intermediate description into the final glyphs. When the widget has
    // focus, the window system will use this to edit the text.
    // The base class returns nullptr.
    virtual TextEditorLogic* asTextEditorLogic();

    /// Returned by size calculations to indicate that the widget wants
    /// as much space as possible in that dimension.
    static const PicaPt kDimGrow;

    struct Constraints
    {
        PicaPt width;
        PicaPt height;
        Constraints() : width(kDimGrow), height(kDimGrow) {}
        Constraints(const PicaPt& w, const PicaPt& h) : width(w), height(h) {}
    };

    // Q: Why not have a LayoutParams struct or something like that?
    // A: That is an option, but it needs further thought. Would
    //    such a struct use ems or PicaPts? If the latter, would
    //    it need to recalculate based on theme changes? It might
    //    worth considering refactoring if/when we add a minimum
    //    and maximum size, and/or a weighting factor.

    static constexpr float kNotFixed = -1.0f;

    /// Sets width in layouts to be the specified number of ems
    /// (subject to constraints such the maximum size of the layout).
    /// Set to kNotFixed to unset.
    Widget* setFixedWidthEm(float ems);
    float fixedWidthEm() const;

    /// Sets width in layouts to be the specified number of ems
    /// (subject to constraints such the maximum size of the layout).
    /// Set to kNotFixed to unset.
    Widget* setFixedHeightEm(float ems);
    float fixedHeightEm() const;

    /// Returns the preferred size of the widget. If a dimension is
    /// >= kDimGrow, the widget wants as much space as it can get in that
    /// dimension. The prefered size of most widgets is always the same,
    /// but some widgets may adjust their height if the width is fixed,
    /// or vice-versa (for instance, large amounts of text, or a large
    /// image). Passing a constraint with the fixed size will signal the
    /// widget to return a more customized size. If you override this
    /// function, use context.dc.ceilToNearestPixel() to make sure that
    /// the size is a whole pixel so that downstream widgets are aligned
    /// nicely to pixel boundaries.
    virtual Size preferredSize(const LayoutContext& context) const;

    /// Lays out children according to the frame. Not intended to be called
    /// directly. If this is overridden, call the super-class layout().
    /// When Widget::layout() is called as a super call, it will only layout
    /// the children; it will not alter their frames. However, if this object
    /// is actually a base Widget object (not a derived instance), then layout()
    /// will set the frame of each child to bounds(). (This allows putting a
    /// Layout as a child of a Widget to work like you would expect, although in
    /// most cases is it most direct to use a Layout directly. In the rare
    /// case that you want a Widget that is just a container, and the children's
    /// bounds are managed elsewhere, such as ListView does, then create a
    /// derived class and instantiate that.
    virtual void layout(const LayoutContext& context);

    enum class EventResult { kIgnored, kConsumed };

    virtual EventResult mouse(const MouseEvent& e);
    virtual void mouseEntered();
    virtual void mouseExited();

    virtual EventResult key(const KeyEvent& e);
    virtual void keyFocusStarted();
    virtual void keyFocusEnded();

    virtual void text(const TextEvent& e);

    /// Called when the theme changes. Generally there is no need to override
    /// this, but if anything like text, text font, or text color is cached, it
    /// should be cleared here (as well as anywhere else relevant).
    virtual void themeChanged(const Theme& theme);

    /// Draws the widget. Classes inheriting from Widget must draw the frame
    /// themselves with Theme::drawFrame() or the desired equivalent. (This
    /// permits widgets to draw the frame differently if they desire.
    /// For example, the slider uses the frame styling to draw the slider
    /// track, but does not want the track to be the full frame.)
    virtual void draw(UIContext& context);

    std::string debugDescription(); // for use in the debugger
    std::string debugDescription(const Point& offset = Point(PicaPt::kZero, PicaPt::kZero),
                                 int indent = 0) const;

public:
    enum MouseState { kNormal = 0, kDisabled, kMouseOver, kMouseDown };
    MouseState state() const;
    /// Merges MouseState with any other state that the widget may have (notably,
    /// selection) to produce the state the widget should use to draw.
    /// If setThemeState() has been called, that state will be used.
    virtual Theme::WidgetState themeState() const;
    Theme::WidgetStyle& style(Theme::WidgetState state);

    /// Forces a widget state for drawing. This can be useful when using a widget
    /// as a child to reuse drawing, instead of reusing functionality.
    /// For example, Button uses Label to draw, but the label should take the
    /// button's theme state. Contrast with NumericEdit, which uses IncDecWidget
    /// as a child, but in that case the child keeps its own drawing state
    /// because it functions as widget.
    void setThemeState(Theme::WidgetState state);

    /// Sets theme state back to unset, to undo a call to setThemeState().
    void resetThemeState();

protected:
    /// Return true if parent's mouse() should grab the widget if mouse down is consumed.
    /// Default implementation returns true.
    virtual bool shouldAutoGrab() const;

    /// Return true if widget has a tooltip. Override this if you need a custom
    /// tooltip for which a string is insufficient.
    virtual bool hasTooltip() const;

    /// Called when the tooltip should be shown. This should create a Widget
    /// and pass to Widget::setTooltip().
    virtual void onTooltip() const;

    /// Since the window does not know about visibility changes, if our widget
    /// changes visibility, we need to ensure that ourself or a child widget
    /// does not have key focus if we became invisible and/or disabled. This is
    /// called by setVisible() and setEnabled(), so you would only need to call
    /// it in the case you are something like StackedWidget, where switching
    /// panels could make an ancestor of a focused widget invisible.
    void updateKeyFocusOnVisibilityOrEnabledChange();

    void setWindow(Window* window);  // for Window

    void setState(MouseState state, bool fromExited = false);

    EventResult mouseChild(const MouseEvent& e, Widget *child, EventResult result); // e is child's parent's event
    void drawChild(UIContext& context, Widget *child);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace uitk

#endif // UITK_WIDGET_H
