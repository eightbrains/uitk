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

#include "Widget.h"

#include "Application.h"
#include "Events.h"
#include "Label.h"
#include "UIContext.h"
#include "Window.h"
#include <nativedraw.h>

#include <limits>
#include <list>
#include <optional>
#include <typeinfo>

namespace uitk {

static const int kNStyles = 5;

static const std::optional<Theme::WidgetState> kUnsetThemeState = std::nullopt;

struct Widget::Impl {
    Window* window = nullptr;  // we do not own this
    Widget* parent = nullptr;  // we do not own this
    std::vector<Widget*> children;  // we own these
    Rect frame;
    Rect bounds;
    Theme::WidgetStyle styles[kNStyles];
    MouseState state = MouseState::kNormal;
    std::optional<Theme::WidgetState> forcedThemeState = kUnsetThemeState;
    std::string tooltip;
    std::string accessibilityText;
    struct {
        float fixedWidthEm = kNotFixed;
        float fixedHeightEm = kNotFixed;
    } layout;
    double lastTooltipPreventingActivityTime = std::numeric_limits<double>::max();
    Application::ScheduledId tooltipTimer = Application::kInvalidScheduledId;
    bool drawsFrame = false;
    bool visible = true;
    bool enabled = true;
    bool showFocusRingOnParent = false;

    void updateDrawsFrame(const Widget *w)
    {
        int userSetBorderMask = (int(Theme::WidgetStyle::kBorderWidthSet) |
                                 int(Theme::WidgetStyle::kBorderColorSet) |
                                 int(Theme::WidgetStyle::kBorderRadiusSet) |
                                 int(Theme::WidgetStyle::kBGColorSet));
        // typeid() == typeid() should be fast, see
        //     https://stackoverflow.com/a/13894738/218226
        // but it is (hopefully) faster to only do the check when a
        // frame value has been altered, since we almost never have a
        // true Widget with a frame (but it should still work).
        // Note that typeid(w) == typeid(Widget*) is always true, and is not
        // a usable check that we are an actual base class.
        if (typeid(*w) == typeid(Widget) || (this->styles[0].flags & userSetBorderMask)) {
            this->drawsFrame = true;
        }
    }

    void clearTooltip()
    {
        if (this->tooltipTimer != Application::kInvalidScheduledId) {
            Application::instance().cancelScheduled(this->tooltipTimer);
            this->tooltipTimer = Application::kInvalidScheduledId;
        }
    }
};

const PicaPt Widget::kDimGrow = PicaPt::fromPixels(8000000.0f /* approx max int-float */, 72.0f);

Widget::Widget()
    : mImpl(new Impl())
{
}

Widget::~Widget()
{
    if (mImpl->parent) {
        mImpl->parent->removeChild(this);
    }
    mImpl->clearTooltip();
    clearAllChildren();
}

std::string Widget::debugDescription()
{
    return debugDescription(Point::kZero, 0);
}

std::string Widget::debugDescription(const Point& offset /*= Point(PicaPt::kZero, PicaPt::kZero)*/,
                                     int indent /*= 0*/) const
{
    std::string s;
    for (int i = 0;  i < indent;  ++i) {
        s += "  ";
    }
    s += std::string(typeid(*this).name()) + " (";
    auto f = frame();
    s += std::to_string((f.x + offset.x).asFloat()) + ", ";
    s += std::to_string((f.y + offset.y).asFloat()) + ") ";
    s += std::to_string(f.width.asFloat()) + " x " + std::to_string(f.height.asFloat());
    s += "\n";

    for (auto child : mImpl->children) {
        s += child->debugDescription(Point(offset.x + f.x, offset.y + f.y), indent + 1);
    }

    return s;
}

const Rect& Widget::frame() const { return mImpl->frame; }

Widget* Widget::setFrame(const Rect& frame)
{
    mImpl->frame = frame;
    mImpl->bounds = Rect(PicaPt::kZero, PicaPt::kZero, frame.width, frame.height);
    return this;
}

Widget* Widget::setPosition(const Point& p)
{
    Rect r = frame();
    r.x = p.x;
    r.y = p.y;
    return setFrame(r);  // need to call setFrame in case it is overridden
}

Widget* Widget::setSize(const Size& size)
{
    Rect r = frame();
    r.width = size.width;
    r.height = size.height;
    return setFrame(r);  // need to call setFrame in case it is overridden
}

const Rect& Widget::bounds() const
{
    return mImpl->bounds;
}

bool Widget::visible() const { return mImpl->visible; }

Widget* Widget::setVisible(bool vis)
{
    if (mImpl->visible != vis) {
        mImpl->visible = vis;
        if (!vis) {
            updateKeyFocusOnVisibilityOrEnabledChange();
        }
        setNeedsDraw();
        if (auto *win = window()) {
            win->setNeedsAccessibilityUpdate();
        }
    }
    return this;
}

bool Widget::enabled() const { return mImpl->enabled; }

Widget* Widget::setEnabled(bool enabled)
{
    mImpl->enabled = enabled;
    setState(enabled ? MouseState::kNormal : MouseState::kDisabled);
    // Recursively disable, so that children's state is also kDisabled, so that
    // they draw correctly.
    for (auto *child : mImpl->children) {
        child->setEnabled(enabled);
    }

    if (!enabled) {
        updateKeyFocusOnVisibilityOrEnabledChange();
    }

    return this;
}

const std::string& Widget::tooltip() const { return mImpl->tooltip; }

Widget* Widget::setTooltip(const std::string& tooltip)
{
    mImpl->tooltip = tooltip;
    return this;
}

bool Widget::hasTooltip() const { return (!mImpl->tooltip.empty()); }

const std::string& Widget::accessibilityText() const
{
    return mImpl->accessibilityText;
}

Widget* Widget::setAccessibilityText(const std::string& text)
{
    mImpl->accessibilityText = text;
    return this;
}

const Color& Widget::backgroundColor() const
{
    return mImpl->styles[0].bgColor;
}

Widget* Widget::setBackgroundColor(const Color& bg)
{
    for (int i = 0;  i < kNStyles;  ++i) {
        mImpl->styles[i].bgColor = bg;
        mImpl->styles[i].flags |= int(Theme::WidgetStyle::kBGColorSet);
    }
    mImpl->updateDrawsFrame(this);
    setNeedsDraw();
    return this;
}

const Color& Widget::borderColor() const
{
    return mImpl->styles[0].borderColor;
}

Widget* Widget::setBorderColor(const Color& color)
{
    for (int i = 0;  i < kNStyles;  ++i) {
        mImpl->styles[i].borderColor = color;
        mImpl->styles[i].flags |= int(Theme::WidgetStyle::kBorderColorSet);
    }
    mImpl->updateDrawsFrame(this);
    setNeedsDraw();
    return this;
}

const PicaPt Widget::borderWidth() const
{
    return mImpl->styles[0].borderWidth;
}

Widget* Widget::setBorderWidth(const PicaPt& width)
{
    for (int i = 0;  i < kNStyles;  ++i) {
        mImpl->styles[i].borderWidth = width;
        mImpl->styles[i].flags |= int(Theme::WidgetStyle::kBorderWidthSet);
    }
    mImpl->updateDrawsFrame(this);
    setNeedsDraw();
    return this;
}

const PicaPt Widget::borderRadius() const
{
    return mImpl->styles[0].borderRadius;
}

Widget* Widget::setBorderRadius(const PicaPt& radius)
{
    for (int i = 0;  i < kNStyles;  ++i) {
        mImpl->styles[i].borderRadius = radius;
        mImpl->styles[i].flags |= int(Theme::WidgetStyle::kBorderRadiusSet);
    }
    mImpl->updateDrawsFrame(this);
    setNeedsDraw();
    return this;
}

Widget* Widget::addChild(Widget *w)
{
    mImpl->children.push_back(w);
    w->mImpl->parent = this;
    setNeedsLayout();
    return this;
}

Widget* Widget::removeChild(Widget *w)
{
    for (auto it = mImpl->children.begin();  it != mImpl->children.end();  ++it) {
        if (*it == w) {
            if (auto *win = window()) {
                if (win->mouseoverWidget() == w) {
                    win->setMouseoverWidget(nullptr);
                }
            }
            mImpl->children.erase(it);
            w->mImpl->parent = nullptr;
            setNeedsLayout();
            return w;
        }
    }
    return nullptr;
}

void Widget::removeAllChildren()
{
    // Since we are returning these widgets to ownership of the caller,
    // clear their state, in case the mouse was over one. (For instance,
    // selecting an item in a combobox.)
    auto *win = window();
    auto *mw = (win ? win->mouseoverWidget() : nullptr);
    for (auto *child : mImpl->children) {
        if (win && mw == child) {
            win->setMouseoverWidget(nullptr);
        }
        child->setState(MouseState::kNormal);
        child->resetThemeState();
        child->mImpl->parent = nullptr;
    }
    mImpl->children.clear();
    // Do not need a layout, technically: there's nothing left to layout
    if (win) {
        win->setNeedsAccessibilityUpdate();
    }
}

void Widget::clearAllChildren()
{
    auto children = mImpl->children;  // copy: removeAllChildren() will clear mImpl->children;
    removeAllChildren();
    for (auto *child : children) {
        delete child;
    }
    mImpl->children.clear();
    // Do not need a layout, technically: there's nothing left to layout
}

const std::vector<Widget*>& Widget::children() const
{
    return mImpl->children;
}

Widget* Widget::parent() const
{
    // TODO: can we detect if the parent is our internal toplevel root widget
    // and return null?
    return mImpl->parent;
}

Window* Widget::window() const
{
    const Widget *w = this;
    while (w->mImpl->parent) {
        w = w->mImpl->parent;
        if (!w) {
            return nullptr;
        }
    }
    return w->mImpl->window;
}

void Widget::setWindow(Window *window) { mImpl->window = window; }

Point Widget::convertToLocalFromWindow(const Point& windowPt) const
{
    Point localPt = windowPt;
    if (Window *win = window()) {
        localPt -= win->contentRect().upperLeft();
    }
    const Widget *w = this;
    while (w->mImpl->parent) {
        localPt -= w->frame().upperLeft();
        w = w->mImpl->parent;
    }
    return localPt;
}

Point Widget::convertToWindowFromLocal(const Point& localPt) const
{
    Point windowPt = localPt;
    if (Window *win = window()) {
        windowPt += win->contentRect().upperLeft();
    }
    const Widget *w = this;
    while (w && w->mImpl->parent) {
        windowPt += w->frame().upperLeft();
        w = w->mImpl->parent;
    }
    return windowPt;
}

void Widget::setNeedsDraw()
{
    if (Window *win = window()) {
        win->setNeedsDraw();
    }
}

void Widget::setNeedsLayout()
{
    if (Window *win = window()) {
        win->setNeedsLayout();
    }
}

bool Widget::focused() const
{
    if (auto *w = window()) {
        return (w->focusWidget() == this);
    }
    return false;
}

void Widget::resignKeyFocus() const
{
    if (auto *w = window()) {
        if (w->focusWidget() == this) {
            w->setFocusWidget(nullptr);
        }
    }
}

void Widget::setShowFocusRingOnParent(bool show)
{
    mImpl->showFocusRingOnParent = show;
}

bool Widget::showFocusRingOnParent() const { return mImpl->showFocusRingOnParent; }

bool Widget::acceptsKeyFocus() const { return false; }

AccessibilityInfo Widget::accessibilityInfo()
{
    auto r = frame();
    r.x = PicaPt::kZero;
    r.y = PicaPt::kZero;
    return {
        AccessibilityInfo::Type::kNone,
        this,
        // Q: why calculate the frame even though we are invisible to the accessibility?
        // A: so that derived classes can just Super and not have to copy the calculation.
        r.translated(convertToWindowFromLocal(Point::kZero))
        // The accessibilityText will be applied by the top-level caller, or
        // when the native element is created, since it needs to override
        // the default text, and anything we set here will be overwritten by
        // the subclass, since we probably in a subclass super-ing.
    };
}

CutPasteable* Widget::asCutPasteable() { return nullptr; }

TextEditorLogic* Widget::asTextEditorLogic() { return nullptr; }

Widget::MouseState Widget::state() const { return mImpl->state; }

void Widget::setState(MouseState state, bool fromExited /*= false*/)
{
    if (!mImpl->enabled) {
        // Need to specifically set in case setEnabled(false) calls setState()
        // after setting mImpl->enabled = false.
        mImpl->state = MouseState::kDisabled;
        return;
    }
    if (mImpl->state == state) {
        return;
    }

    auto oldState = mImpl->state;
    mImpl->state = state;

    if (oldState == MouseState::kNormal && state != MouseState::kNormal) {
        mouseEntered();
    } else if ((oldState != MouseState::kNormal && oldState != MouseState::kDisabled) && state == MouseState::kNormal && !fromExited) {
        mouseExited();
    }

    setNeedsDraw();
}

void Widget::setThemeState(Theme::WidgetState state)
{
    mImpl->forcedThemeState = state;
}

void Widget::resetThemeState()
{
    mImpl->forcedThemeState = kUnsetThemeState;
}

Theme::WidgetState Widget::themeState() const
{
    if (mImpl->enabled) {
        if (mImpl->forcedThemeState.has_value()) {
            return mImpl->forcedThemeState.value();
        } else {
            return (Theme::WidgetState)mImpl->state;
        }
    } else {
        return Theme::WidgetState::kDisabled;
    }
}

Theme::WidgetStyle& Widget::style(Theme::WidgetState state)
{
    return mImpl->styles[int(state)];
}

bool Widget::shouldAutoGrab() const { return true;  }

void Widget::updateKeyFocusOnVisibilityOrEnabledChange()
{
    if (auto w = window()) {
        if (auto focus = w->focusWidget()) {
            auto *p = focus->parent();
            while (p) {
                if (!p->visible() || !p->enabled()) {
                    focus->resignKeyFocus();
                    break;
                }
                p = p->parent();
            }
        }
    }
}

Widget* Widget::setFixedWidthEm(float ems)
{
    mImpl->layout.fixedWidthEm = ems;
    setNeedsLayout();
    return this;
}

float Widget::fixedWidthEm() const
{
    return mImpl->layout.fixedWidthEm;
}

Widget* Widget::setFixedHeightEm(float ems)
{
    mImpl->layout.fixedHeightEm = ems;
    setNeedsLayout();
    return this;
}

float Widget::fixedHeightEm() const
{
    return mImpl->layout.fixedHeightEm;
}

Size Widget::preferredSize(const LayoutContext& context) const
{
    bool isRealWidget = (typeid(*this) == typeid(Widget));  // typeid(this) is always Widget*
    if (isRealWidget) {
        // If this is an actual instance of Widget, make the preferred size
        // the maximum widget size (layout will be all on top of each other).
        // This behavior is useful so that putting a Layout in a widget works
        // like you would expect. (If you want a widget that is just a container
        // and does not size the children, create a derived class.)
        if (mImpl->children.size() == 0) {
            return Size(kDimGrow, kDimGrow);
        } else {
            Size size;
            for (auto *child : mImpl->children) {
                auto pref = child->preferredSize(context);
                size.width = std::max(size.width, pref.width);
                size.height = std::max(size.height, pref.height);
            }
            if (size.width <= PicaPt::kZero) {
                size.width = kDimGrow;
            }
            if (size.height <= PicaPt::kZero) {
                size.height = kDimGrow;
            }
            return size;
        }
    } else {  // this is a derived class of Widget
        return Size(kDimGrow, kDimGrow);
    }
}

void Widget::layout(const LayoutContext& context)
{
    static bool showBaseWarning = true;

    // If this is actually and instance of Widget, we need to size the children.
    // If this is a derived class, then we are supering and should NOT size
    // the children, because the derived class' layout() already did this.
    // This behavior for a real Widget allows users to put a Layout in a widget
    // and get the results that they expect. (If you want a widget that just
    // acts as a container and someone else manages the frames of the children,
    // then create a derived class.)
    // (Note: when changing, test with print dialog on Linux, and the ListView
    //        container widget)
    bool isRealWidget = (typeid(*this) == typeid(Widget));  // typeid(this) is always Widget*
    if (isRealWidget) {
        for (auto *child : mImpl->children) {
            child->setFrame(bounds());
        }
    }

    // Always layout the children
    for (auto *child : mImpl->children) {
        child->layout(context);
    }
}

Widget::EventResult Widget::mouse(const MouseEvent& e)
{
    if (!enabled()) {
        return EventResult::kIgnored;
    }

    auto *win = window();
    if (e.type == MouseEvent::Type::kButtonDown) {
        mImpl->clearTooltip();
        if (win) {
            win->clearTooltip();
        }
    }

    auto *oldMouseoverWidget = (win ? win->mouseoverWidget() : nullptr);

    auto result = EventResult::kIgnored;
    // Drawing is done in order (first is bottom, last is top), so hit-testing must be
    // done in reverse order.
    for (auto childIt = mImpl->children.rbegin();  childIt != mImpl->children.rend();  ++childIt) {
        result = mouseChild(e, *childIt, result);
        if (result == EventResult::kConsumed) {
            break;
        }
    }

    // Note that oldMouseoverWidget might be DELETED, if this was an action.
    // (For instance, a directory list when .. was clicked and it cleared out the listbox;
    // the old ".." label is now deleted.) Presumably moves will not produce actions.
    // (We could also check that `result == EventResult::kIgnored`, but then if someone
    // was confused and thought they needed to consume a move [or, more likely, a drag],
    // then this would fail.)
    if (win && oldMouseoverWidget) {
        bool isMove = (e.type == MouseEvent::Type::kMove || e.type == MouseEvent::Type::kDrag ||
                       e.type == MouseEvent::Type::kScroll);
        auto *mw = win->mouseoverWidget();
        if (isMove && oldMouseoverWidget != mw && oldMouseoverWidget->state() != MouseState::kNormal) {
            auto calcParents = [](Widget *w) -> std::list<Widget*> {
                std::list<Widget*> parents; // list, because will always insert
                auto *p = w;
                do {
                    p = p->parent();
                    parents.push_front(p);
                } while (p);
                return parents;
             };

            auto oldParents = calcParents(oldMouseoverWidget);
            auto newParents = calcParents(mw);
            auto oit = oldParents.begin();
            auto nit = newParents.begin();
            while (oit != oldParents.end() && nit != newParents.end() && *nit == *oit) {
                ++oit;
                ++nit;
            }
            for (; oit != oldParents.end();  ++oit) {
                (*oit)->mouseExited();
            }
        }
    }

    return result;
}

Widget::EventResult Widget::mouseChild(const MouseEvent& e, Widget *child, EventResult result)
{
    if (!child->enabled() || !child->visible()) {
        return result;
    }

    if (child->frame().contains(e.pos)) {
        // Mouse is in child, check if we just entered
        switch (e.type) {
            case MouseEvent::Type::kMove:
            case MouseEvent::Type::kScroll: {
                child->setState(MouseState::kMouseOver);
                if (auto *win = child->window()) {
                    win->setMouseoverWidget(child);
                }
                break;
            }
            case MouseEvent::Type::kButtonDown:
            case MouseEvent::Type::kDrag:
                child->setState(MouseState::kMouseDown);
                break;
            case MouseEvent::Type::kButtonUp:
                child->setState(MouseState::kMouseOver);
                break;
        }
        // Send the event to the child if an earlier widget has not already
        // consumed this event.
        if (result != EventResult::kConsumed) {
            auto childE = e;
            childE.pos -= child->frame().upperLeft();
            if (child->mouse(childE) == EventResult::kConsumed) {
                result = EventResult::kConsumed;
                // When to grab the mouse is a bit tricky. We want grabbing to be fairly
                // automatic, so that each control does not need to do it, but we also
                // do not want purely visual widgets like Label to grab it. Compromise and
                // require mouse down to be consumed. This still forces a number of controls
                // (like Button) to make changes, since button down doesn't really do anything,
                // but it seems a good compromise. (Note: if we refactor so that there is a
                // Control base class, maybe we can move this below the if and only grab
                // if it is a control.)
                if (e.type == MouseEvent::Type::kButtonDown && !window()->mouseGrabWidget()
                    && child->shouldAutoGrab()) {
                    window()->setMouseGrab(child);
                }
            }
        }
    } else {
        // Mouse not in child, check if it exited the widget
        child->setState(MouseState::kNormal);
    }

    return result;
}

void Widget::mouseEntered()
{
    auto *w = window();
    if (hasTooltip()) {
        assert(mImpl->tooltipTimer == Application::kInvalidScheduledId);
        mImpl->lastTooltipPreventingActivityTime = Application::instance().microTime();
        auto tooltipDelay = Application::instance().tooltipDelaySecs();
        mImpl->tooltipTimer = Application::instance().scheduleLater(w, 0.1f,
                                                                    Application::ScheduleMode::kRepeating,
                                                                    [this,tooltipDelay](Application::ScheduledId id) {
            assert(state() != MouseState::kNormal);
            auto now = Application::instance().microTime();
            if (now >= mImpl->lastTooltipPreventingActivityTime + tooltipDelay) {
                onTooltip();
                mImpl->clearTooltip();
            }
        });
    } else {
        if (w) {
            w->clearTooltip();
        }
    }
}

void Widget::mouseExited()
{
    auto *w = window();
    if (hasTooltip()) {
        mImpl->clearTooltip();
        if (w) {
            w->clearTooltip();
        }
    }

    // Normally it is not necessary to set the state back to normal, as the mouse
    // event handler will take care of this. However, if the application loses focus
    // by a key event (e.g. Alt-Tab, this event will be generated but obviously
    // the mouse will not have moved.
    if (state() != MouseState::kNormal) {
        setState(MouseState::kNormal, true);
    }

    for (auto *child : mImpl->children) {
        if (child->state() == MouseState::kMouseOver
            || child->state() == MouseState::kMouseDown) {
            child->mouseExited();
            setState(MouseState::kNormal, true);
        }
    }

    assert(state() == MouseState::kNormal);
}

void Widget::onTooltip() const
{
    if (auto *w = window()) {
        if (!mImpl->tooltip.empty()) {
            w->setTooltip(new Label(mImpl->tooltip));
        } else {
            w->clearTooltip();
        }
    }
}

Widget::EventResult Widget::key(const KeyEvent& e)
{
    return EventResult::kIgnored;
}

void Widget::keyFocusStarted()
{
}

void Widget::keyFocusEnded()
{
}

void Widget::text(const TextEvent& e)
{
}

void Widget::themeChanged(const Theme& theme)
{
    for (auto *child : mImpl->children) {
        child->themeChanged(theme);
    }
}

void Widget::draw(UIContext& context)
{
    // If we are truly just a Widget (instead of a derived class that is
    // supering), then draw the frame. It would be nice to do this only if
    // draw() has been overridden, but it is not possible to know that.
    if (mImpl->drawsFrame) {
        // The frame should always be the normal; Widget does not process
        // events and so should not have mouseover or activated appearances.
        context.theme.drawFrame(context, bounds(), style(Theme::WidgetState::kNormal));
    }

    // Draw the children.
    // We do not draw a child that is completely outside the draw rect.
    // For the most part, this is not necessary, and is here mostly for
    // the ScrollView. In particular, a ListView with thousands of items
    // draws really slowly if it needs to draw thousands of invisible texts
    // (especially since drawing text is fairly slow). So why not just
    // put this in ScrollView, instead of needing to adjust the draw rect
    // and create a new UIContext for each object? Because there is no way
    // to guarantee that the all the items are directly owned by the
    // ScrollView; if the caller puts all the items in a big widget and
    // then the ScrollView just owns that, it will see that the big child
    // intersects with the draw rect and draw then entire child.
    for (auto *child : mImpl->children) {
        if (context.drawRect.intersects(child->frame())) {
            drawChild(context, child);
        }
    }
}

void Widget::drawChild(UIContext& context, Widget *child)
{
    if (child->visible()) {
        auto ul = child->frame().upperLeft();
        context.dc.translate(ul.x, ul.y);

        // Need to create a new context to update the draw rect.
        // The context is not const, so we could just modify the context, but the
        // reason it is not const is because the DrawContext is non-const.
        auto newDrawRect = context.drawRect.intersectedWith(child->frame());
        newDrawRect.translate(-ul.x, -ul.y);  // this is faster than .translated(), though less convenient
        UIContext newContext = { context.theme, context.dc, newDrawRect, context.isWindowActive };
        child->draw(newContext);

        context.dc.translate(-ul.x, -ul.y);
    }
}

}  // namespace uitk
