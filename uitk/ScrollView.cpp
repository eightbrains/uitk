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

#include "ScrollView.h"

#include "Application.h"
#include "Events.h"
#include "ScrollBar.h"
#include "UIContext.h"

namespace uitk {

namespace {
static const float kScrollbarDPI = 72.0f;

enum class Tristate { kUndefined = -1, kFalse = 0, kTrue = 1 };

PicaPt calcMinOffsetX(const Rect& frame, const Rect& bounds)
{
    return -std::max(PicaPt::kZero, bounds.width - frame.width);
}

PicaPt calcMinOffsetY(const Rect& frame, const Rect& bounds)
{
    return -std::max(PicaPt::kZero, bounds.height - frame.height);
}

}  // namespace

struct ScrollView::Impl
{
    Rect bounds;
    ScrollBar *horizScroll;  // this is a child; base class owns
    ScrollBar *vertScroll;  // this is a child; base class owns
    Tristate drawsFrame = Tristate::kUndefined;
};

ScrollView::ScrollView()
    : mImpl(new Impl)
{
    mImpl->horizScroll = new ScrollBar(Dir::kHoriz);
    mImpl->horizScroll->setVisible(false);
    mImpl->horizScroll->setOnValueChanged([this](SliderLogic *scroll) {
        setContentOffset(Point(PicaPt::fromPixels(-scroll->doubleValue(), kScrollbarDPI), bounds().y));
    });
    mImpl->vertScroll = new ScrollBar(Dir::kVert);
    mImpl->vertScroll->setVisible(false);
    mImpl->vertScroll->setOnValueChanged([this](SliderLogic *scroll) {
        setContentOffset(Point(bounds().x, PicaPt::fromPixels(-scroll->doubleValue(), kScrollbarDPI)));
    });
    Super::addChild(mImpl->horizScroll);
    Super::addChild(mImpl->vertScroll);
}

ScrollView::~ScrollView()
{
}

const Rect& ScrollView::bounds() const
{
    return mImpl->bounds;
}

ScrollView* ScrollView::setBounds(const Rect& bounds)
{
    mImpl->bounds = bounds;

    auto &f = frame();
    if (bounds.width > f.width) {
        if (!Application::instance().shouldHideScrollbars()) {
            mImpl->horizScroll->setVisible(true);
        }
        mImpl->horizScroll->setRange(0.0f,
                                     -calcMinOffsetX(f, bounds).toPixels(kScrollbarDPI),
                                     f.width.toPixels(kScrollbarDPI),
                                     bounds.width.toPixels(kScrollbarDPI));
        mImpl->horizScroll->setValue(-mImpl->bounds.x.toPixels(kScrollbarDPI));
    } else {
        mImpl->horizScroll->setVisible(false);
    }

    if (bounds.height > f.height) {
        if (!Application::instance().shouldHideScrollbars()) {
            mImpl->vertScroll->setVisible(true);
        }
        mImpl->vertScroll->setRange(0.0f,
                                    -calcMinOffsetY(f, bounds).toPixels(kScrollbarDPI),
                                    f.height.toPixels(kScrollbarDPI),
                                    bounds.height.toPixels(kScrollbarDPI));
        mImpl->vertScroll->setValue(-mImpl->bounds.y.toPixels(kScrollbarDPI));
    } else {
        mImpl->vertScroll->setVisible(false);
    }

    setNeedsDraw();
    return this;
}

ScrollView* ScrollView::setContentSize(const Size& size)
{
    return setBounds(Rect(bounds().x, bounds().y, size.width, size.height));
}

ScrollView* ScrollView::setContentOffset(const Point& offset)
{
    return setBounds(Rect(offset.x, offset.y, bounds().width, bounds().height));
}

Size ScrollView::preferredSize(const LayoutContext& context) const
{
    return Size(kDimGrow, kDimGrow);
}

void ScrollView::layout(const LayoutContext& context)
{
    auto pref = mImpl->vertScroll->preferredSize(context);
    mImpl->vertScroll->setFrame(Rect(frame().width - pref.width, PicaPt::kZero,
                                     pref.width, frame().height));
    pref = mImpl->horizScroll->preferredSize(context);
    mImpl->horizScroll->setFrame(Rect(PicaPt::kZero, frame().height - pref.height,
                                      frame().width, pref.height));
    Super::layout(context);
}

Widget::EventResult ScrollView::mouse(const MouseEvent& e)
{
    auto result = EventResult::kIgnored;

    // Mouse the scrollbars (see comment in draw() for why)
    bool inScrollbar = ((mImpl->vertScroll->visible() && mImpl->vertScroll->frame().contains(e.pos)) ||
                        (mImpl->horizScroll->visible() && mImpl->horizScroll->frame().contains(e.pos)));
    result = mouseChild(e, mImpl->vertScroll, result);
    result = mouseChild(e, mImpl->horizScroll, result);
    if (result == EventResult::kConsumed && inScrollbar) {
        result = Widget::EventResult::kConsumed;
    }

    // Now mouse the scrollable children
    if (result != EventResult::kConsumed) {
        auto mouseE = e;
        mouseE.pos -= mImpl->bounds.upperLeft();
        for (auto *child : children()) {
            if (child != mImpl->horizScroll && child != mImpl->vertScroll) {
                result = mouseChild(mouseE, child, result);
            }
        }
    }

    // Finally, handle scroll events (note that this gives scroll priority to
    // scrollable children).
    if (e.type == MouseEvent::Type::kScroll && result != EventResult::kConsumed) {
        auto minOffsetX = calcMinOffsetX(frame(), bounds());
        auto minOffsetY = calcMinOffsetY(frame(), bounds());
        auto offsetX = std::min(PicaPt::kZero,std::max(e.scroll.dx + mImpl->bounds.x, minOffsetX));
        auto offsetY = std::min(PicaPt::kZero, std::max(e.scroll.dy + mImpl->bounds.y, minOffsetY));
        setContentOffset(Point(offsetX, offsetY));
        if (Application::instance().shouldHideScrollbars()) {
            mImpl->horizScroll->setVisible(true);
            mImpl->vertScroll->setVisible(true);
        }
        setNeedsDraw();
        result = EventResult::kConsumed;
    } else {
        if (!inScrollbar && Application::instance().shouldHideScrollbars()) {
            mImpl->horizScroll->setVisible(false);
            mImpl->vertScroll->setVisible(false);
        }
    }

    return result;
}

void ScrollView::draw(UIContext& context)
{
    Rect boundingRect(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);

    context.dc.save();
    context.theme.clipScrollView(context, boundingRect, style(state()), state());

    if (mImpl->drawsFrame == Tristate::kUndefined) {
        if (typeid(*this) == typeid(ScrollView)) {  // see Widget.cpp, Impl::updateDrawsFrame()
            mImpl->drawsFrame = Tristate::kTrue;
        } else {
            mImpl->drawsFrame = Tristate::kFalse;
        }
    }
    if (mImpl->drawsFrame == Tristate::kTrue) {
        context.theme.drawScrollView(context, boundingRect, style(state()), state());
    }

    // It would be nicer to be able to Super::draw() here (and have the scrollbars on top),
    // but that would require that we have content widget that everything is added to.
    // Since we want scrollview->addChild() to do what you'd expect (add to the scrollable
    // part), and we cannot make addChild() virtual (because you cannot call virtual functions
    // in the constructor, but the constructor is the place where you want to create and
    // add your children), we need to draw them ourselves.

    // Draw the children of the (non-existent) content widget.
    context.dc.translate(mImpl->bounds.x, mImpl->bounds.y);
    for (auto *child : children()) {
        if (child != mImpl->horizScroll && child != mImpl->vertScroll) {
            drawChild(context, child);
        }
    }
    context.dc.translate(-mImpl->bounds.x, -mImpl->bounds.y);

    // Draw the scrollbars last so they are on top.
    drawChild(context, mImpl->horizScroll);
    drawChild(context, mImpl->vertScroll);

    context.dc.restore();
}

}  // namespace uitk
