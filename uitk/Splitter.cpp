//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#include "Splitter.h"

#include "Cursor.h"
#include "Length.h"
#include "UIContext.h"
#include "Window.h"

namespace uitk {

class SplitterThumb : public Widget
{
    using Super = Widget;
public:
    SplitterThumb(Splitter& parent, int parentIdx)
        : mParent(parent), mParentIdx(parentIdx)
    {}

    AccessibilityInfo accessibilityInfo() override
    {
        auto lengths = mParent.panelLengths();
        auto total = PicaPt::kZero;
        for (auto len : lengths) {
            total += len;
        }
        auto inc = 0.01 * total;

        auto info = Super::accessibilityInfo();
        info.type = AccessibilityInfo::Type::kSplitterThumb;
        info.value = double(std::round((lengths[mParentIdx / 2] / total) * 100.0f));
        info.performDecrementNumeric = [this, inc]() {
            performDragTo(frame().x - inc);
        };
        info.performIncrementNumeric = [this, inc]() {
            performDragTo(frame().x + inc);
        };
        return info;
    }

    void performDragTo(const PicaPt& parentCoord)
    {
        // lengths will be valid if dragging, so do not recompute, but will be
        // empty for accessibility
        bool inMouse = !mLengths.empty();

        auto lengths = mLengths;
        if (!inMouse) {
            cacheLengths();
        }

        PicaPt thumbLen;
        if (mParent.direction() == Dir::kHoriz) {
            thumbLen = frame().width;
        } else {
            thumbLen = frame().height;
        }
        PicaPt pos = std::min(mMax_parent, std::max(mMin_parent, parentCoord));
        auto prevSegLen = pos - mMin_parent;
        // Note that a handle will always be in-between two panels
        prevSegLen = mOnePx * std::round(prevSegLen / mOnePx);
        lengths[mParentIdx / 2] = prevSegLen;
        lengths[mParentIdx / 2 + 1] = mMax_parent - mMin_parent - thumbLen - prevSegLen;
        mParent.setPanelLengths(lengths);

        if (!inMouse) {
            clearLengths();
        }
    }

    Size preferredSize(const LayoutContext& context) const override
    {
        auto length = context.theme.calcPreferredSplitterThumbThickness(context.dc);

        if (mParent.direction() == Dir::kHoriz) {
            return Size(length, kDimGrow);
        } else {
            return Size(kDimGrow, length);
        }
    }

    void mouseEntered() override
    {
        Super::mouseEntered();
        if (auto *w = window()) {
            if (mParent.direction() == Dir::kHoriz) {
                w->pushCursor(Cursor::resizeLeftRight());
            } else {
                w->pushCursor(Cursor::resizeUpDown());
            }
        }
    }

    void mouseExited() override
    {
        Super::mouseExited();
        if (auto *w = window()) {
            w->popCursor();
        }
    }

    EventResult mouse(const MouseEvent& e) override
    {
        switch (e.type) {
            case MouseEvent::Type::kButtonDown: {
                if (mParent.direction() == Dir::kHoriz) {
                    mMouseDownCoord = e.pos.x;
                } else {
                    mMouseDownCoord = e.pos.y;
                }
                cacheLengths();
                return EventResult::kConsumed;
            }
            case MouseEvent::Type::kDrag:
                // We could get a drag without a mouse down if we mouse down elsewhere
                // then drag across the handle.
                if (!mLengths.empty()) {
                    if (mParent.direction() == Dir::kHoriz) {
                        performDragTo(e.pos.x + mParent.children()[mParentIdx]->frame().x);
                    } else {
                        performDragTo(e.pos.y + mParent.children()[mParentIdx]->frame().y);
                    }
                }
                return EventResult::kConsumed;
            case MouseEvent::Type::kButtonUp: {
                clearLengths();
                return EventResult::kConsumed;
            }
            default:
                return Super::mouse(e);
        }
    }

    void layout(const LayoutContext& context) override
    {
        mOnePx = context.dc.onePixel();
        Super::layout(context);
    }

    void draw(UIContext& context) override
    {
        Super::draw(context);
        auto state = themeState();
        context.theme.drawSplitterThumb(context, bounds(), style(state), state);
    }

private:
    Splitter &mParent;
    int mParentIdx;
    PicaPt mOnePx;
    PicaPt mMouseDownCoord;
    // ---- only valid between mouse down and mouse up
    PicaPt mMin_parent;
    PicaPt mMax_parent;
    std::vector<PicaPt> mLengths;
    // ----

    void cacheLengths()
    {
        mLengths = mParent.panelLengths();
        auto &siblings = mParent.children();
        if (mParentIdx > 0) {
            auto f = siblings[mParentIdx - 1]->frame();
            if (mParent.direction() == Dir::kHoriz) {
                mMin_parent = f.x;
            } else {
                mMin_parent = f.y;
            }
        } else {
            mMin_parent = PicaPt::kZero;
        }

        Rect f;
        if (mParentIdx < int(siblings.size()) - 1) {
            f = siblings[mParentIdx + 1]->frame();
        } else {
            f = siblings.back()->frame();
        }
        if (mParent.direction() == Dir::kHoriz) {
            mMax_parent = f.maxX();
        } else {
            mMax_parent = f.maxY();
        }
    }

    void clearLengths()
    {
        mLengths.clear();
    }
};

//-----------------------------------------------------------------------------
struct Splitter::Impl
{
    Dir dir;
    std::vector<Length> lengths;
};

Splitter::Splitter(Dir dir)
    : mImpl(new Impl())
{
    mImpl->dir = dir;
}

Splitter* Splitter::addPanel(Widget *panel)
{
    if (children().size() > 0) {
        addChild(new SplitterThumb(*this, int(children().size())));
    }
    addChild(panel);
    assert(children().size() & 0x1);  // number of children must always be odd (because of thumbs)
    return this;
}

Widget* Splitter::removePanel(Widget *panel)
{
    assert(children().size() & 0x1);
    auto &childs = children();
    for (size_t i = 0;  i < childs.size();  ++i) {
        if (childs[i] == panel) {
            if (i > 0) {
                delete removeChild(childs[i]);
            }
            return removeChild(panel);
        }
    }
    return nullptr;
}

Dir Splitter::direction() const { return mImpl->dir; }

std::vector<PicaPt> Splitter::panelLengths() const
{
    auto &childs = children();
    std::vector<PicaPt> lens;
    lens.reserve(childs.size());  // this is extra, but faster to calculate
    for (size_t i = 0;  i < childs.size();  i += 2) {
        PicaPt len = (mImpl->dir == Dir::kHoriz ? childs[i]->frame().width
                                                : childs[i]->frame().height);
        lens.push_back(len);
    }
    return lens;
}

Splitter* Splitter::setPanelLengths(const std::vector<Length>& lengths)
{
    mImpl->lengths = lengths;
    setNeedsLayout();
    return this;
}

Splitter* Splitter::setPanelLengths(const std::vector<PicaPt>& lengths)
{
    mImpl->lengths.clear();
    mImpl->lengths.reserve(lengths.size());
    for (auto &len : lengths) {
        mImpl->lengths.push_back(len);
    }
    setNeedsLayout();
    return this;
}

Splitter* Splitter::setPanelLengthsEm(const std::vector<float>& lengths)
{
    mImpl->lengths.clear();
    mImpl->lengths.reserve(lengths.size());
    for (auto &len : lengths) {
        mImpl->lengths.emplace_back(len, Length::Units::kEm);
    }
    setNeedsLayout();
    return this;
}

Splitter* Splitter::setPanelLengthsPercent(const std::vector<float>& lengths)
{
    mImpl->lengths.clear();
    mImpl->lengths.reserve(lengths.size());
    for (auto &len : lengths) {
        mImpl->lengths.emplace_back(len, Length::Units::kPercent);
    }
    setNeedsLayout();
    return this;
}

AccessibilityInfo Splitter::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    info.type = AccessibilityInfo::Type::kSplitter;
    // The text is a hack to avoid adding a field to AccessibilityInfo just for Splitters
    // can store their direction. Note that we cannot use info.text, or we will not be
    // a pass-through group, so use placeholderText, instead.
    info.placeholderText = (mImpl->dir == Dir::kHoriz ? "horizontal splitter" : "vertical splitter");
    auto &childs = children();
    for (size_t i = 0;  i < childs.size();  ++i) {
        auto childInfo = childs[i]->accessibilityInfo();
        childInfo.indexInParent = int(i);
        if (childInfo.type == AccessibilityInfo::Type::kNone) {
            childInfo.type = AccessibilityInfo::Type::kContainer;
        }
        info.children.push_back(childInfo);
    }
    return info;
}

Size Splitter::preferredSize(const LayoutContext& context) const
{
    Size size;
    PicaPt border = (borderColor().alpha() > 0.001f ? borderWidth() : PicaPt::kZero);
    if (mImpl->dir == Dir::kHoriz) {
        for (auto child : children()) {
            auto pref = child->preferredSize(context);
            size.width += pref.width;
            size.height = std::max(size.height, pref.height);
        }
    } else {
        for (auto child : children()) {
            auto pref = child->preferredSize(context);
            size.width = std::max(size.height, pref.height);
            size.height += pref.width;
        }
    }
    return Size(context.dc.ceilToNearestPixel(size.width + 2.0f * border),
                context.dc.ceilToNearestPixel(size.height + 2.0f * border));
}

void Splitter::layout(const LayoutContext& context)
{
    auto& r = bounds();
    PicaPt border = (borderColor().alpha() > 0.001f ? borderWidth() : PicaPt::kZero);
    PicaPt hundredPercent;
    Size thumbPref;
    auto &childs = children();
    auto nThumbs = int(childs.size()) / 2;  // int / int = int (truncated)
    if (nThumbs > 0) {
        thumbPref = childs[1]->preferredSize(context);
    }
    if (mImpl->dir == Dir::kHoriz) {
        hundredPercent = r.width - 2.0f * border - float(nThumbs) * thumbPref.width;
    } else {
        hundredPercent = r.height - 2.0f * border - float(nThumbs) * thumbPref.height;
    }

    size_t nPanels = childs.size() - size_t(nThumbs);
    if (nPanels > 0) {
        size_t nLengths = std::min(nPanels, mImpl->lengths.size());
        PicaPt len = PicaPt::kZero;
        std::vector<PicaPt> lengths;
        lengths.reserve(nPanels);
        for (size_t i = 0;  i < nLengths;  ++i) {
            lengths.push_back(mImpl->lengths[i].toPicaPt(context, hundredPercent));
            if (lengths.back() >= PicaPt::kZero) {
                len += lengths.back();
            }
        }
        // Remove negative lengths; see below
        for (auto &l : lengths) {
            if (l < PicaPt::kZero) {
                nLengths -= 1;
            }
        }

        if (len > hundredPercent) {
            float adjust = hundredPercent / len;
            for (auto &l : lengths) {
                l *= adjust;
            }
            len *= adjust;
            len = std::min(len, hundredPercent);  // just in case of floating point error on 'adjust'
        }
        if (nLengths < nPanels) {
            // If the panels already fill up the space, we cannot fit any more in,
            // so adjust them according to what percentage of extra space we need.
            if (len / hundredPercent > 0.99f) {
                float adjust = float(nLengths) / float(nPanels);
                for (auto &l : lengths) {
                    l *= adjust;
                }
                len *= adjust;
            }

            // Treat negative size as not added, which allows us to set the size
            // of the first and last panel (for example) without needing to know the
            // size the middle panel(s), which is useful if you are setting by
            // PicaPt or Em.
            auto extra = (hundredPercent - len) / (nPanels - nLengths);
            for (auto &l : lengths) {
                if (l < PicaPt::kZero) {
                    l = extra;
                    nLengths += 1;
                }
            }
            for (auto i = 0;  i < nPanels - nLengths;  ++i) {
                lengths.push_back(extra);
            }
        }

        auto onePx = context.dc.onePixel();
        std::vector<int> iLengths;
        iLengths.reserve(lengths.size());
        for (auto &l : lengths) {
            // Grr, floating point error: can't truncate because might be ever-so-slightly
            // smaller than an integer value, in which case truncating loses a pixel.
            // This is not a problem in the static case, but it causes very visible
            // jittering when resizing panels with a mouse.
            iLengths.push_back(int(std::round(l / onePx)));
        }
        int lengthPx = int(std::floor(hundredPercent / onePx));
        int currentPx = 0;
        for (auto px : iLengths) {
            currentPx += px;
        }

        int extraPx = lengthPx - currentPx;  // might be negative
        while (extraPx != 0) {
            int dir = extraPx / int(nPanels);
            if (dir == 0) {
                dir = extraPx / std::abs(extraPx);
            }
            // Iterate backwards. For large chunks there will be no difference,
            // but when resizing the panels with the thumb, any jitter caused by
            // floating point error (of which there should be none), this will
            // jitter the later panels, which looks less visible / more natural than if the
            // first ones are jittering when you resize later panels.
            size_t idx = nPanels - 1;
            while (extraPx != 0 && std::abs(extraPx) >= std::abs(dir)) {
                iLengths[idx] += dir;
                extraPx -= dir;
                idx -= 1;
                if (idx < 0) {
                    idx = nPanels - 1;
                }
            }
        }

        auto x = PicaPt::kZero;
        auto y = PicaPt::kZero;
        for (size_t idx = 0;  idx < iLengths.size();  ++idx) {
            bool hasThumb = (idx < iLengths.size() - 1);
            auto length = float(iLengths[idx]) * onePx;
            if (mImpl->dir == Dir::kHoriz) {
                childs[2 * idx]->setFrame(Rect(x, PicaPt::kZero, length, r.height));
                x += length;
                if (hasThumb) {
                    childs[2 * idx + 1]->setFrame(Rect(x, PicaPt::kZero, thumbPref.width, r.height));
                    x += thumbPref.width;
                }
            } else {
                childs[2 * idx]->setFrame(Rect(PicaPt::kZero, y, r.width, length));
                y += length;
                if (hasThumb) {
                    childs[2 * idx + 1]->setFrame(Rect(PicaPt::kZero, y, r.width, thumbPref.height));
                    y += thumbPref.height;
                }
            }
        }

        // The calculation for lengthPx might have dropped part of a pixel.
        if (mImpl->dir == Dir::kHoriz) {
            auto f = childs.back()->frame();
            f.width = r.maxX() - f.x;
            childs.back()->setFrame(f);
        } else {
            auto f = childs.back()->frame();
            f.height = r.maxY() - f.y;
            childs.back()->setFrame(f);
        }
    }

    Super::layout(context);
}

void Splitter::draw(UIContext& context)
{
    for (auto child : children()) {
        context.dc.save();
        context.dc.clipToRect(child->frame());
        drawChild(context, child);
        context.dc.restore();
    }
}

} // namespace uitk
