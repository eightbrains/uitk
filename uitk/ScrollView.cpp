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

#include "ScrollView.h"

#include "Application.h"
#include "Events.h"
#include "ScrollBar.h"
#include "UIContext.h"

#include <limits>

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

class ScrollViewContent : public Widget {};  // so Widget does not automatically layout contents

}  // namespace

struct ScrollView::Impl
{
    Rect bounds;
    Rect contentRect;

    ScrollViewContent *content;  // this is a child; base class owns
    ScrollBar *horizScroll;  // this is a child; base class owns
    ScrollBar *vertScroll;  // this is a child; base class owns
    bool usesHorizScrollbar = false;
    bool usesVertScrollbar = false;
    Tristate drawsFrame = Tristate::kUndefined;
    Application::ScheduledId hideScrollbarsTimer = Application::kInvalidScheduledId;
    double lastShowScrollActionTime = std::numeric_limits<double>::max();
    bool mouseIsInScrollbar = false;

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

    void hideScrollbars()
    {
        this->cancelHideTimer();
        this->horizScroll->setVisible(false);
        this->vertScroll->setVisible(false);
    }

    void cancelHideTimer()
    {
        if (this->hideScrollbarsTimer != Application::kInvalidScheduledId) {
            Application::instance().cancelScheduled(this->hideScrollbarsTimer);
            this->hideScrollbarsTimer = Application::kInvalidScheduledId;
        }
    }
};

ScrollView::ScrollView()
    : mImpl(new Impl)
{
    mImpl->content = new ScrollViewContent();
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
    Super::addChild(mImpl->content);
    Super::addChild(mImpl->horizScroll);
    Super::addChild(mImpl->vertScroll);
}

ScrollView::~ScrollView()
{
    // Cancel the timer in case it is still going. (Do not hideScrollbars(), since setVisible
    // may attempt to use the window, which might be going away.)
    mImpl->cancelHideTimer();
}

Widget* ScrollView::content() const { return mImpl->content; }

Rect ScrollView::contentRect() const { return mImpl->contentRect; }

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
    mImpl->content->setFrame(bounds);

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

void ScrollView::scroll(const PicaPt& dx, const PicaPt& dy)
{
    auto pos = scrollPosition();
    scrollTo(pos.x + dx, pos.y + dy);
}

void ScrollView::scrollTo(const PicaPt& x, const PicaPt& y)
{
    auto maxScrollX = std::max(PicaPt::kZero, bounds().width - frame().width);
    auto maxScrollY = std::max(PicaPt::kZero, bounds().height - frame().height);
    setContentOffset(Point(std::min(std::max(-x, -maxScrollX), PicaPt::kZero),
                           std::min(std::max(-y, -maxScrollY), PicaPt::kZero)));
}

Point ScrollView::scrollPosition() const
{
    auto &b = bounds();
    return Point(-b.x, -b.y);
}

bool ScrollView::isMouseInScrollbar() const { return mImpl->mouseIsInScrollbar; }

AccessibilityInfo ScrollView::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    info.type = AccessibilityInfo::Type::kContainer;
    info.text = "Scrollable container";
    return info;
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

    // Super-mouse: this will handle the scrollbars and the content (including prioritizing
    // scrolling in children).
    bool inScrollbar = ((mImpl->vertScroll->visible() && mImpl->vertScroll->frame().contains(e.pos)) ||
                        (mImpl->horizScroll->visible() && mImpl->horizScroll->frame().contains(e.pos)));
    mImpl->mouseIsInScrollbar = inScrollbar;
    result = Super::mouse(e);
    if (result == EventResult::kConsumed) {
        if (result == EventResult::kConsumed && inScrollbar) {
            mImpl->lastShowScrollActionTime = Application::instance().microTime();
        }
    }

    // Finally, handle scroll events (note that this gives scroll priority to
    // scrollable children).
    if (e.type == MouseEvent::Type::kScroll && result != EventResult::kConsumed) {
        mImpl->lastShowScrollActionTime = Application::instance().microTime();
        auto minOffsetX = calcMinOffsetX(frame(), bounds());
        auto minOffsetY = calcMinOffsetY(frame(), bounds());
        auto offsetX = std::min(PicaPt::kZero, std::max(e.scroll.dx + mImpl->bounds.x, minOffsetX));
        auto offsetY = std::min(PicaPt::kZero, std::max(e.scroll.dy + mImpl->bounds.y, minOffsetY));
        setContentOffset(Point(offsetX, offsetY));
        if (Application::instance().shouldHideScrollbars()) {
            // Should show the scrollbar until the autohide timeout, even if mouse is moved or
            // exits the frame. Mouse in the scroll area will prevent it from being hidden.
            mImpl->horizScroll->setVisible(mImpl->usesHorizScrollbar);
            mImpl->vertScroll->setVisible(mImpl->usesVertScrollbar);
            if (mImpl->hideScrollbarsTimer == Application::kInvalidScheduledId) {
                if (auto *w = window()) {
                    mImpl->hideScrollbarsTimer = Application::instance()
                                                       .scheduleLater(w, 0.1f,
                                                                      Application::ScheduleMode::kRepeating,
                                                                      [this](Application::ScheduledId id) {
                        assert(mImpl->hideScrollbarsTimer == id);
                        auto &app = Application::instance();
                        auto timeToHide = mImpl->lastShowScrollActionTime + app.autoHideScrollbarDelaySecs();
                        if (!mImpl->mouseIsInScrollbar && app.microTime() >= timeToHide) {
                            mImpl->hideScrollbars();
                        }
                    });
                }
            }
        }
        setNeedsDraw();
        result = EventResult::kConsumed;
    }

    return result;
}

void ScrollView::mouseExited()
{
    mImpl->mouseIsInScrollbar = false;
    Super::mouseExited();
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
    Rect frameRect(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);
    if (mImpl->drawsFrame == Tristate::kTrue) {
        context.theme.drawScrollView(context, frameRect, style(themeState()), themeState());
    }

    auto origBounds = mImpl->content->frame();
    auto newBounds = origBounds;
    newBounds.x = context.dc.roundToNearestPixel(newBounds.x);
    newBounds.y = context.dc.roundToNearestPixel(newBounds.y);
    if (origBounds.x != newBounds.x || origBounds.y != newBounds.y) {
        mImpl->content->setFrame(newBounds);
    }

    context.dc.save();
    context.theme.clipScrollView(context, frameRect, style(themeState()), themeState(),
                                 mImpl->drawsFrame == Tristate::kTrue);
    UIContext scrollContext = { context.theme, context.dc, mImpl->contentRect, context.isWindowActive };
    Super::draw(scrollContext);
    context.dc.restore();
}

}  // namespace uitk
