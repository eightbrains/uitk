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
#include <nativedraw.h>

namespace uitk {

static const int kNStyles = 4;

struct Widget::Impl {
    std::vector<Widget*> children;
    Rect frame;
    Theme::WidgetStyle styles[kNStyles];
    Theme::WidgetState state = Theme::WidgetState::kNormal;
    bool visible = true;
    bool enabled = true;
};

const PicaPt Widget::kDimGrow = PicaPt::fromPixels(32000.0f, 72.0f);

Widget::Widget()
    : mImpl(new Impl())
{
}

Widget::~Widget()
{
    for (auto *child : mImpl->children) {
        delete child;
    }
}

const Rect& Widget::frame() const { return mImpl->frame; }

Widget* Widget::setFrame(const Rect& frame)
{
    mImpl->frame = frame;
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
    return this;
}

Widget* Widget::addChild(Widget *w)
{
    mImpl->children.push_back(w);
    return this;
}

const std::vector<Widget*> Widget::children() const
{
    return mImpl->children;
}

Theme::WidgetState Widget::state() const { return mImpl->state; }

void Widget::setState(Theme::WidgetState state)
{
    auto oldState = mImpl->state;
    mImpl->state = state;

    if (oldState == Theme::WidgetState::kNormal && state != Theme::WidgetState::kNormal) {
        mouseEntered();
    } else if (oldState != Theme::WidgetState::kNormal && state == Theme::WidgetState::kNormal) {
        mouseExited();
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

    auto &r = frame();

    auto result = EventResult::kIgnored;
    auto childE = e;
    childE.pos -= r.upperLeft();
    for (auto *child : mImpl->children) {
        if (!child->enabled()) {
            continue;
        }
        if (child->frame().contains(childE.pos)) {
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
                if (child->mouse(childE) == EventResult::kConsumed) {
                    result = EventResult::kConsumed;
                }
            }
        } else {
            // Mouse not in child, check if it exited the widget
            child->setState(Theme::WidgetState::kNormal);
        }
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

        for (auto *child : mImpl->children) {
            if (child->state() != Theme::WidgetState::kNormal) {
                child->mouseExited();
            }
        }
    }
}

void Widget::draw(UIContext& ui)
{
    auto ul = frame().upperLeft();
    ui.dc.translate(ul.x, ul.y);
    for (auto *child : mImpl->children) {
        child->draw(ui);
    }
    ui.dc.translate(-ul.x, -ul.y);
}

}  // namespace uitk
