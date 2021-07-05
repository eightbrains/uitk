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

#include "Widget.h"

#include "Events.h"
#include "UIContext.h"
#include "Window.h"
#include <nativedraw.h>

namespace uitk {

static const int kNStyles = 4;

struct Widget::Impl {
    Window* window = nullptr;  // we do not own this
    Widget* parent = nullptr;  // we do not own this
    std::vector<Widget*> children;  // we own these
    Rect frame;
    Rect bounds;
    Theme::WidgetStyle styles[kNStyles];
    Theme::WidgetState state = Theme::WidgetState::kNormal;
    bool drawsFrame = false;
    bool visible = true;
    bool enabled = true;

    void updateDrawsFrame(const Widget *w)
    {
        // typeid() == typeid() should be fast, see
        //     https://stackoverflow.com/a/13894738/218226
        // but it is (hopefully) faster to only do the check when a
        // frame value has been altered, since we almost never have a
        // true Widget with a frame (but it should still work).
        // Note that typeid(w) == typeid(Widget*) is always true, and is not
        // a usable check that we are an actual base class.
        if (typeid(*w) == typeid(Widget)) {
            this->drawsFrame = true;
        }
    }
};

const PicaPt Widget::kDimGrow = PicaPt::fromPixels(32000.0f, 72.0f);

Widget::Widget()
    : mImpl(new Impl())
{
}

Widget::~Widget()
{
    removeAllChildren();
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
    mImpl->visible = vis;
    return this;
}

bool Widget::enabled() const { return mImpl->enabled; }

Widget* Widget::setEnabled(bool enabled)
{
    mImpl->enabled = enabled;
    setState(enabled ? Theme::WidgetState::kNormal : Theme::WidgetState::kDisabled);
    // Recursively disable, so that children's state is also kDisabled, so that
    // they draw correctly.
    for (auto *child : mImpl->children) {
        child->setEnabled(enabled);
    }
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
    return this;
}

Widget* Widget::removeChild(Widget *w)
{
    for (auto it = mImpl->children.begin();  it != mImpl->children.end();  ++it) {
        if (*it == w) {
            mImpl->children.erase(it);
            return w;
        }
    }
    return nullptr;
}

void Widget::removeAllChildren()
{
    for (auto *child : mImpl->children) {
        delete child;
    }
}

const std::vector<Widget*> Widget::children() const
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

Point Widget::convertToLocalFromWindow(const Point& windowPt)
{
    Point localPt = windowPt;
    const Widget *w = this;
    while (w->mImpl->parent) {
        localPt -= w->frame().upperLeft();
        w = w->mImpl->parent;
    }
    return localPt;
}

void Widget::setNeedsDraw()
{
    if (Window *win = window()) {
        win->setNeedsDraw();
    }
}

Theme::WidgetState Widget::state() const { return mImpl->state; }

void Widget::setState(Theme::WidgetState state)
{
    if (mImpl->state == Theme::WidgetState::kNormal && state == Theme::WidgetState::kMouseDown) {
        state = state;
    }
    
    auto oldState = mImpl->state;
    mImpl->state = state;

    if (oldState == Theme::WidgetState::kNormal && state != Theme::WidgetState::kNormal) {
        mouseEntered();
    } else if (oldState != Theme::WidgetState::kNormal && state == Theme::WidgetState::kNormal) {
        mouseExited();
    }

    if (oldState != state) {
        setNeedsDraw();
    }
}

Theme::WidgetStyle& Widget::style(Theme::WidgetState state)
{
    return mImpl->styles[int(state)];
}

Size Widget::preferredSize(const LayoutContext& context) const
{
    return Size(kDimGrow, kDimGrow);
}

void Widget::layout(const LayoutContext& context)
{
    for (auto *child : mImpl->children) {
        child->layout(context);
    }
}

Widget::EventResult Widget::mouse(const MouseEvent& e)
{
    if (!enabled()) {
        return EventResult::kIgnored;
    }

    auto result = EventResult::kIgnored;
    for (auto *child : mImpl->children) {
        result = mouseChild(e, child, result);
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
            case MouseEvent::Type::kScroll:
                child->setState(Theme::WidgetState::kMouseOver);
                break;
            case MouseEvent::Type::kButtonDown:
            case MouseEvent::Type::kDrag:
                child->setState(Theme::WidgetState::kMouseDown);
                break;
            case MouseEvent::Type::kButtonUp:
                child->setState(Theme::WidgetState::kMouseOver);
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
                if (e.type == MouseEvent::Type::kButtonDown && !window()->mouseGrabWidget()) {
                    window()->setMouseGrab(child);
                }
            }
        }
    } else {
        // Mouse not in child, check if it exited the widget
        child->setState(Theme::WidgetState::kNormal);
    }

    return result;
}

void Widget::mouseEntered()
{
}

void Widget::mouseExited()
{
    // Normally it is not necessary to set the state back to normal, as the mouse
    // event handler will take care of this. However, if the application loses focus
    // be a key event, this event will be generated but obviously the mouse will not
    // have moved.
    if (state() != Theme::WidgetState::kNormal) {
        setState(Theme::WidgetState::kNormal);
    }

    for (auto *child : mImpl->children) {
        if (child->state() == Theme::WidgetState::kMouseOver
            || child->state() == Theme::WidgetState::kMouseDown) {
            child->mouseExited();
        }
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

    for (auto *child : mImpl->children) {
        drawChild(context, child);
    }
}

void Widget::drawChild(UIContext& context, Widget *child)
{
    if (child->visible()) {
        auto ul = child->frame().upperLeft();
        context.dc.translate(ul.x, ul.y);
        child->draw(context);
        context.dc.translate(-ul.x, -ul.y);
    }
}

}  // namespace uitk
