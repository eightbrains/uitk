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
    Rect contentRect;
    ScrollBar *horizScroll;  // this is a child; base class owns
    ScrollBar *vertScroll;  // this is a child; base class owns
    bool usesHorizScrollbar = false;
    bool usesVertScrollbar = false;
    Tristate drawsFrame = Tristate::kUndefined;

    void updateContentRect(const Rect& frame)
    {
        this->contentRect = Rect(PicaPt::kZero, PicaPt::kZero, frame.width, frame.height);

        // If the scrollbars only show while scrolling on the trackpad, they should
        // appear above the content, but if they are always there, the content rect
        // must be smaller.
        if (!Application::instance().shouldHideScrollbars()) {
            Size dsize(PicaPt::kZero, PicaPt::kZero);
            if (this->vertScroll->visible()) {
                dsize.width = this->vertScroll->frame().width;
            }
            if (this->horizScroll->visible()) {
                dsize.height = this->vertScroll->frame().width;
            }
            this->contentRect.width -= dsize.width;
            this->contentRect.height -= dsize.height;
        }
    }

    void updateScrollFrames(const Rect& frame)
    {
        if (this->vertScroll->visible() && this->horizScroll->visible()) {
            auto f = this->vertScroll->frame();
            f.height = frame.height - this->horizScroll->frame().height;
            this->vertScroll->setFrame(f);
            f = this->horizScroll->frame();
            f.width = frame.width - this->vertScroll->frame().width;
            this->horizScroll->setFrame(f);
        } else if (this->vertScroll->visible()) {
            auto f = this->vertScroll->frame();
            f.height = frame.height;
            this->vertScroll->setFrame(f);
        } else {
            auto f = this->horizScroll->frame();
            f.width = frame.width;
            this->horizScroll->setFrame(f);
        }
    }
};

ScrollView::ScrollView()
    : mImpl(new Impl)
{
    mImpl->horizScroll = new ScrollBar(Dir::kHoriz);
    mImpl->horizScroll->setVisible(false);
    mImpl->horizScroll->setOnValueChanged([this](SliderLogic *scroll) {
        setContentOffset(Point(PicaPt::fromPixels(float(-scroll->doubleValue()), kScrollbarDPI), bounds().y));
    });
    mImpl->vertScroll = new ScrollBar(Dir::kVert);
    mImpl->vertScroll->setVisible(false);
    mImpl->vertScroll->setOnValueChanged([this](SliderLogic *scroll) {
        setContentOffset(Point(bounds().x, PicaPt::fromPixels(float(-scroll->doubleValue()), kScrollbarDPI)));
    });
    Super::addChild(mImpl->horizScroll);
    Super::addChild(mImpl->vertScroll);
}

ScrollView::~ScrollView()
{
}

ScrollView* ScrollView::setFrame(const Rect& frame)
{
    Super::setFrame(frame);
    mImpl->updateContentRect(frame);
    mImpl->updateScrollFrames(frame);
    return this;
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
        mImpl->usesHorizScrollbar = true;
    } else {
        mImpl->horizScroll->setVisible(false);
        mImpl->usesHorizScrollbar = false;
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
        mImpl->usesVertScrollbar = true;
    } else {
        mImpl->vertScroll->setVisible(false);
        mImpl->usesVertScrollbar = false;
    }

    mImpl->updateContentRect(frame());
    mImpl->updateScrollFrames(frame());

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
    Rect vertFrame(frame().width - pref.width, PicaPt::kZero,
                   pref.width, frame().height);
    pref = mImpl->horizScroll->preferredSize(context);
    Rect horizFrame(PicaPt::kZero, frame().height - pref.height,
                    frame().width, pref.height);
    mImpl->vertScroll->setFrame(vertFrame);
    mImpl->horizScroll->setFrame(horizFrame);

    mImpl->updateContentRect(frame());
    mImpl->updateScrollFrames(frame());

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
            mImpl->horizScroll->setVisible(mImpl->usesHorizScrollbar);
            mImpl->vertScroll->setVisible(mImpl->usesVertScrollbar);
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
    if (mImpl->drawsFrame == Tristate::kUndefined) {
        if (typeid(*this) == typeid(ScrollView)) {  // see Widget.cpp, Impl::updateDrawsFrame()
            mImpl->drawsFrame = Tristate::kTrue;
        } else {
            mImpl->drawsFrame = Tristate::kFalse;
        }
    }
    if (mImpl->drawsFrame == Tristate::kTrue) {
        Rect frameRect(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);
        context.theme.drawScrollView(context, frameRect, style(themeState()), themeState());
    }

    context.dc.save();
    context.theme.clipScrollView(context, mImpl->contentRect, style(themeState()), themeState());

    // It would be nicer to be able to Super::draw() here (and have the scrollbars on top),
    // but that would require that we have a content widget that everything is added to.
    // Since we want scrollview->addChild() to do what you'd expect (add to the scrollable
    // part), and we cannot make addChild() virtual (because you cannot call virtual functions
    // in the constructor, but the constructor is the place where you want to create and
    // add your children), we need to draw them ourselves.

    // Draw the children of the (non-existent) content widget.
    Rect contentRect(-mImpl->bounds.x, -mImpl->bounds.y, frame().width, frame().height);
    context.dc.translate(mImpl->bounds.x, mImpl->bounds.y);
    for (auto *child : children()) {
        if (child != mImpl->horizScroll && child != mImpl->vertScroll
            && contentRect.intersects(child->frame())) {
            drawChild(context, child);
        }
    }
    context.dc.translate(-mImpl->bounds.x, -mImpl->bounds.y);
    context.dc.restore();

    // Draw the scrollbars last so they are on top.
    drawChild(context, mImpl->horizScroll);
    drawChild(context, mImpl->vertScroll);
}

}  // namespace uitk
