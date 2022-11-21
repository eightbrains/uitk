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

#include "Waiting.h"

#include "Application.h"
#include "UIContext.h"
#include "Window.h"

#include <assert.h>

#include <unordered_map>

namespace uitk {

namespace {

static const float kTickSecs = 0.15f;

static const int kNBlobs = 11;
const int kNTailBlobs = 4;
const float kPercentMaxWidth = 1.0f;  // 100%
const float kPercentRadius = 0.666f;  // 66%

class SynchronizedAnimator
{
    // Keep a global-window state:
    // 1) if we have multiple indicators each with their own timers, each calling setNeedsDraw()
    //    we will have way more draws than we need and will max out the CPU for no reason.
    // 2) this way all of the indicators in a window are synchronized; it just looks better than
    //    than having each indicator be in a different position on the circle.
    struct State {
        int tick = 0;
        Application::ScheduledId tickTimer = Application::kInvalidScheduledId;
        std::set<Waiting*> animatingWidgets;  // these are refs
    };
    std::unordered_map<Window*, State> mStates;  // these pointers are refs
    std::unordered_map<Waiting*, Window*> mWidgetToWindow;  // these pointers are refs

public:
    // Don't need to do anything in the destructor since
    // a) this will be destructed on program exit and all the widgets should be gone
    // b) widgets should deregister themselves in their destructor anyway.
    // c) these pointers are all refs, so we don't actually own anything.
    // d) if the app is quitting we might just exit the run loop and let the OS cleanup,
    //    so we cannot even assert that everything is empty.

    void add(Waiting* waiting, Window* window)
    {
        if (mWidgetToWindow.find(waiting) != mWidgetToWindow.end()) {  // already animating
            return;
        }

        auto sIt = mStates.find(window);
        if (sIt == mStates.end()) {
            auto timer = Application::instance().scheduleLater(window, kTickSecs,
                                                               Application::ScheduleMode::kRepeating,
                                                               [this, window](Application::ScheduledId tid) {
                auto sIt = mStates.find(window);
                if (sIt != mStates.end()) {
                    sIt->second.tick += 1;
                    if (sIt->second.tick < 0) { // overflowed (very unlikely, will take years, but make it work)
                        sIt->second.tick = 0;
                    }
                    window->setNeedsDraw();
                } else {
                    Application::instance().cancelScheduled(tid);
                }
            });
            mStates[window] = State();
            mStates[window].tickTimer = timer;
            sIt = mStates.find(window);
        }

        sIt->second.animatingWidgets.insert(waiting);
        mWidgetToWindow[waiting] = window;
    }

    // Don't require the Window, in case the widget has been removed from the window
    // (e.g. the window is closing)--this is not ideal for us but we still need to not crash.
    void remove(Waiting* waiting)
    {
        auto wIt = mWidgetToWindow.find(waiting);
        if (wIt != mWidgetToWindow.end()) {
            auto *w = wIt->second;
            auto sIt = mStates.find(w);
            if (sIt != mStates.end()) {
                auto &state = sIt->second;
                state.animatingWidgets.erase(waiting);
                if (state.animatingWidgets.empty()) {
                    if (state.tickTimer != Application::kInvalidScheduledId) {
                        Application::instance().cancelScheduled(state.tickTimer);
                    }
                    state.tickTimer = Application::kInvalidScheduledId;
                    mStates.erase(sIt);
                }
            }
            mWidgetToWindow.erase(waiting);
        }
    }

    int getTickFor(Waiting* waiting)
    {
        auto wIt = mWidgetToWindow.find(waiting);
        if (wIt != mWidgetToWindow.end()) {
            auto *w = wIt->second;
            auto sIt = mStates.find(w);
            if (sIt != mStates.end()) {
                return sIt->second.tick;
            }
        }
        return -1;
    }
};
static SynchronizedAnimator gAnimator;

}  // namespace

struct Waiting::Impl
{
    bool isAnimating = false;
};

Waiting::Waiting()
    : mImpl(new Impl())
{
}

Waiting::~Waiting()
{
    gAnimator.remove(this);
}

bool Waiting::isAnimating() const { return mImpl->isAnimating; }

Waiting* Waiting::setAnimating(bool animating)
{
    if (mImpl->isAnimating == animating) {
        return this;
    }
    mImpl->isAnimating = animating;
    if (animating) {
        auto *w = window();
        assert(w);
        if (!w) {
            return this;
        }
        gAnimator.add(this, w);
    } else {
        gAnimator.remove(this);
    }
    return this;
}

Size Waiting::preferredSize(const LayoutContext& context) const
{
    auto h = context.theme.params().labelFont.pointSize();
    return Size(h, h);
}

void Waiting::draw(UIContext& context)
{
    Super::draw(context);

    if (mImpl->isAnimating) {
        auto fg = context.theme.params().textColor;  // copy

        auto tick = gAnimator.getTickFor(this);
        if (tick < 0) {  // something is wrong
            assert(mImpl->isAnimating);
            return;
        }

        auto r = bounds();
        auto size = std::min(r.width, r.height);
        r.x = r.x + 0.5f * (r.width - size);
        r.y = r.y + 0.5f * (r.height - size);
        r.width = size;
        r.height = size;
        auto margin = context.dc.roundToNearestPixel(0.05f * size);
        r.inset(margin, margin);
        size -= 2.0f * margin;
        auto radius = 0.5f * size;
        auto anglePerBlob = 2.0f * 3.141592f / float(kNBlobs);
        auto maxWidth = 2.0f * radius * (1.0f - kPercentRadius) * std::tan(0.5f * anglePerBlob);
        maxWidth = std::max(maxWidth, context.dc.onePixel());
        auto w = kPercentMaxWidth * maxWidth;
        auto h = kPercentRadius * radius;
        auto midX = r.midX();
        auto midY = r.midY();

        auto path = context.dc.createBezierPath();
        path->addRoundedRect(Rect(midX - 0.5f * w, r.y, w, h), 0.5f * w);

        float dAlpha = fg.alpha() / float(kNTailBlobs + 1);
        int nCompleteRevolutions = tick / kNBlobs;
        int t = tick - nCompleteRevolutions * kNBlobs;

        context.dc.save();
        context.dc.translate(midX, midY);
        context.dc.rotate(-float(t) * anglePerBlob * 180.0f / 3.141592f);
        context.dc.translate(-midX, -midY);
        for (int i = 0;  i < kNTailBlobs;  ++i) {
            context.dc.setFillColor(fg);
            context.dc.drawPath(path, kPaintFill);
            context.dc.translate(midX, midY);
            context.dc.rotate(anglePerBlob * 180.0f / 3.141592f);
            context.dc.translate(-midX, -midY);
            fg = Color(fg, fg.alpha() - dAlpha);
        }
        context.dc.restore();
    }
}

}  // namespace uitk
