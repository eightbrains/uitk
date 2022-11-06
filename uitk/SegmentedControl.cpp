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

#include "SegmentedControl.h"

#include "Events.h"
#include "Icon.h"
#include "Label.h"
#include "IconAndText.h"
#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct SegmentedControl::Impl
{
    struct Item {
        std::string name;
        Rect frame;
        Theme::WidgetState state = Theme::WidgetState::kNormal;
        bool isOn = false;
    };
    std::vector<Item> items;
    std::function<void(int)> onClicked;
    DrawStyle drawStyle = DrawStyle::kNormal;
    Action action = Action::kButton;
    int keyFocusSegmentIdx = -1;  // when has key focus, this is the segment that "has focus"
};

SegmentedControl::SegmentedControl()
    : mImpl(new Impl())
{
}

SegmentedControl::SegmentedControl(const std::vector<std::string>& items)
    : mImpl(new Impl())
{
    for (auto &name : items) {
        addItem(name);
    }
}

SegmentedControl::~SegmentedControl()
{
}

void SegmentedControl::clearItems()
{
    for (auto child : children()) {
        delete removeChild(child);
    }
    mImpl->items.clear();
    setNeedsDraw();
}

SegmentedControl* SegmentedControl::addItem(const std::string& name)
{
    mImpl->items.push_back({name});

    auto *cell = new IconAndText();
    cell->label()->setText(name);
    cell->label()->setAlignment(Alignment::kCenter);
    addChild(cell);  // calls setNeedsDraw(), we don't need to
    return this;
}

SegmentedControl* SegmentedControl::addItem(Theme::StandardIcon icon)
{
    mImpl->items.push_back({""});

    auto *cell = new IconAndText();
    cell->icon()->setIcon(icon);
    addChild(cell);
    return this;
}

SegmentedControl* SegmentedControl::addItem(const Theme::Icon& icon)
{
    mImpl->items.push_back({""});

    auto *cell = new IconAndText();
    cell->icon()->setIcon(icon);
    addChild(cell);
    return this;
}

SegmentedControl* SegmentedControl::addItem(Theme::StandardIcon icon, const std::string& name)
{
    mImpl->items.push_back({name});

    auto *cell = new IconAndText();
    cell->icon()->setIcon(icon);
    cell->label()->setText(name);
    cell->label()->setAlignment(Alignment::kCenter);
    addChild(cell);
    return this;
}

SegmentedControl* SegmentedControl::addItem(const Theme::Icon& icon, const std::string& name)
{
    mImpl->items.push_back({name});

    auto *cell = new IconAndText();
    cell->icon()->setIcon(icon);
    cell->label()->setText(name);
    cell->label()->setAlignment(Alignment::kCenter);
    addChild(cell);
    return this;
}

SegmentedControl::Action SegmentedControl::action() const
{
    return mImpl->action;
}

SegmentedControl* SegmentedControl::setAction(Action act)
{
    mImpl->action = act;
    return this;
}

bool SegmentedControl::isSegmentOn(int index) const
{
    if (index < 0 || size_t(index) >= mImpl->items.size()) {
        return false;
    }
    return mImpl->items[index].isOn;
}

SegmentedControl* SegmentedControl::setSegmentOn(int index, bool on)
{
    if (mImpl->action == Action::kButton) {
        return this;
    }

    if (index >= 0 || size_t(index) < mImpl->items.size()) {
        if (mImpl->action == Action::kSelectOne) {
            for (auto &item : mImpl->items) {
                item.isOn = false;
            }
        }
        mImpl->items[index].isOn = on;
        setNeedsDraw();
    }
    return this;
}

SegmentedControl::DrawStyle SegmentedControl::drawStyle() const
{
    return mImpl->drawStyle;
}

SegmentedControl* SegmentedControl::setDrawStyle(DrawStyle s)
{
    mImpl->drawStyle = s;
    setNeedsDraw();
    return this;
}

SegmentedControl* SegmentedControl::setOnClicked(std::function<void(int)> onClicked)
{
    mImpl->onClicked = onClicked;
    return this;
}

Size SegmentedControl::preferredSize(const LayoutContext& context) const
{
    Size pref;
    auto font = context.theme.params().labelFont;
    auto margins = context.theme.calcPreferredSegmentMargins(context.dc, font);
    for (auto *item : children()) {
        auto segPref = item->preferredSize(context);
        pref.width += segPref.width + 2.0f * margins.width;
        pref.height = std::max(pref.height, segPref.height + 2.0f * margins.height);
    }
    return pref;
}

void SegmentedControl::layout(const LayoutContext& context)
{
    if (!mImpl->items.empty()) {
        auto b = bounds();
        PicaPt total = PicaPt::kZero;
        std::vector<PicaPt> prefs;
        std::vector<PicaPt> widths;
        prefs.reserve(mImpl->items.size());
        widths.reserve(mImpl->items.size());
        auto font = context.theme.params().labelFont;
        auto margins = context.theme.calcPreferredSegmentMargins(context.dc, font);
        for (auto *item : children()) {
            auto segPref = item->preferredSize(context);
            prefs.push_back(segPref.width);
            widths.push_back(segPref.width + 2.0f * margins.width);
            total += widths.back();
        }

        if (total != b.width) {
            auto dw = (b.width - total) / float(mImpl->items.size());
            for (auto &w : widths) {
                w += dw;
                w = context.dc.roundToNearestPixel(w);
            }
        }

        auto x = PicaPt::kZero;
        int i = 0;
        for (auto child : children()) {
            auto w = std::min(prefs[i], widths[i]);
            auto xMargin = context.dc.roundToNearestPixel(0.5f * (widths[i] - w));
            child->setFrame(Rect(x + xMargin, PicaPt::kZero, w, b.height));
            mImpl->items[i].frame = Rect(x, PicaPt::kZero, widths[i], b.height);
            x += widths[i];
            i++;
        }
    }

    Super::layout(context);
}

Widget::EventResult SegmentedControl::mouse(const MouseEvent& e)
{
    auto oldState = themeState();
    auto result = Super::mouse(e);
    auto newState = themeState();

    if (e.type == MouseEvent::Type::kButtonDown) {
        // Like Button, we don't do anything for mouse down, but it *does*
        // change state, and we do want to be the grab widget.
        result = EventResult::kConsumed;
    } else if (e.type == MouseEvent::Type::kButtonUp) {
        for (size_t i = 0;  i < mImpl->items.size();  ++i) {
            if (mImpl->items[i].frame.contains(e.pos)) {
                switch (mImpl->action) {
                    case Action::kSelectOne:
                        setSegmentOn(int(i), true);
                        break;
                    case Action::kSelectMultiple:
                        setSegmentOn(int(i), !mImpl->items[i].isOn);
                        break;
                    case Action::kButton:
                        break;  // do nothing, button action is not set on/off
                }
                if (mImpl->onClicked) {
                    mImpl->onClicked(int(i));
                }
                result = EventResult::kConsumed;
                break;
            }
        }
    }

    if (oldState != newState && newState == Theme::WidgetState::kNormal) {
        for (auto &item : mImpl->items) {
            item.state = Theme::WidgetState::kNormal;
        }
        // Since state transitioned, setState() called setNeedsDraw()
    }

    bool needsDraw = false;
    if (newState == Theme::WidgetState::kMouseOver || newState == Theme::WidgetState::kMouseDown) {
        for (size_t i = 0;  i < mImpl->items.size();  ++i) {
            auto oldSegState = mImpl->items[i].state;
            if (children()[i]->frame().contains(e.pos)) {
                mImpl->items[i].state = newState;
            } else {
                mImpl->items[i].state = Theme::WidgetState::kNormal;
            }
            if (oldSegState != mImpl->items[i].state) {
                needsDraw = true;
            }
        }
    }
    if (needsDraw) {
        setNeedsDraw();
    }

    return result;
}

void SegmentedControl::mouseExited()
{
    for (auto &item : mImpl->items) {
        item.state = Theme::WidgetState::kNormal;
    }
    setNeedsDraw();  // might not actually need this, caller might call this?
}

bool SegmentedControl::acceptsKeyFocus() const { return true; }

Widget::EventResult SegmentedControl::key(const KeyEvent& e)
{
    auto result = Super::key(e);
    if (result != EventResult::kIgnored) {
        return result;
    }

    if (mImpl->action == Action::kSelectOne) {
        if (e.type == KeyEvent::Type::kKeyDown) {
            switch (e.key) {
                case Key::kLeft:
                case Key::kRight: {
                    int onIdx = -1;
                    for (size_t i = 0;  i < mImpl->items.size();  ++i) {
                        if (mImpl->items[i].isOn) {
                            onIdx = int(i);
                            break;
                        }
                    }
                    if (onIdx >= 0) {
                        if (e.key == Key::kRight) {
                            ++onIdx;
                        } else {
                            --onIdx;
                        }
                    }
                    if (onIdx < 0 || onIdx >= int(mImpl->items.size())) {
                        if (e.key == Key::kRight) {
                            onIdx = 0;
                        } else {
                            onIdx = int(mImpl->items.size() - 1);
                        }
                    }
                    setSegmentOn(onIdx, true);
                    if (mImpl->onClicked) {
                        mImpl->onClicked(onIdx);
                    }
                    return EventResult::kConsumed;
                }
                default:
                    break;
            }
        }
    } else {
        const int nSegs = int(mImpl->items.size());
        switch (e.key) {
            case Key::kLeft:
            case Key::kRight: {
                if (e.type == KeyEvent::Type::kKeyDown) {
                    if (mImpl->keyFocusSegmentIdx >= 0) {
                        if (e.key == Key::kRight) {
                            ++mImpl->keyFocusSegmentIdx;
                        } else {
                            --mImpl->keyFocusSegmentIdx;
                        }
                    }
                    if (mImpl->keyFocusSegmentIdx < 0 || mImpl->keyFocusSegmentIdx >= nSegs) {
                        if (e.key == Key::kRight) {
                            mImpl->keyFocusSegmentIdx = 0;
                        } else {
                            mImpl->keyFocusSegmentIdx = nSegs - 1;
                        }
                    }
                    setNeedsDraw();
                }
                return EventResult::kConsumed;
            }
            case Key::kSpace:
            case Key::kReturn:
            case Key::kEnter: {
                const auto focusIdx = mImpl->keyFocusSegmentIdx;
                if (focusIdx >= 0 && focusIdx < nSegs) {
                    if (e.type == KeyEvent::Type::kKeyDown) {
                        mImpl->items[focusIdx].state = Theme::WidgetState::kMouseDown;
                    } else if (mImpl->items[focusIdx].state == Theme::WidgetState::kMouseDown) {
                        setSegmentOn(focusIdx, !isSegmentOn(focusIdx));
                        if (mImpl->onClicked) {
                            mImpl->onClicked(focusIdx);
                        }
                        mImpl->items[focusIdx].state = Theme::WidgetState::kNormal;
                    }
                    setNeedsDraw();
                }
                return EventResult::kConsumed;
            }
            case Key::kEscape: {
                const auto focusIdx = mImpl->keyFocusSegmentIdx;
                if (focusIdx >= 0 && focusIdx < nSegs && mImpl->items[focusIdx].state == Theme::WidgetState::kMouseDown) {
                    mImpl->items[focusIdx].state = Theme::WidgetState::kNormal;
                }
                setNeedsDraw();
                return EventResult::kConsumed;
            }
            default:
                break;
        }
    }
    return EventResult::kIgnored;
}

void SegmentedControl::keyFocusEnded()
{
    mImpl->keyFocusSegmentIdx = -1;
}

void SegmentedControl::draw(UIContext& context)
{
    auto ctrlState = enabled() ? Theme::WidgetState::kNormal : Theme::WidgetState::kDisabled;
    auto ds = (mImpl->drawStyle == DrawStyle::kNoDecoration
                    ? Theme::SegmentDrawStyle::kNoDecoration
                    : Theme::SegmentDrawStyle::kNormal);
    context.theme.drawSegmentedControl(context, bounds(), ds, style(ctrlState), ctrlState);
    for (auto &item : mImpl->items) { // do first, so that segments are under focus rect
        context.theme.drawSegmentDivider(context, item.frame.upperLeft(), item.frame.lowerLeft(),
                                         ds, style(ctrlState), ctrlState);
    }
    int nItems = int(mImpl->items.size());
    for (int i = 0;  i < nItems;  ++i) {
        auto &item = mImpl->items[i];
        bool showKeyFocus = (i == mImpl->keyFocusSegmentIdx);
        if (item.isOn || item.state != Theme::WidgetState::kNormal || showKeyFocus) {
            context.theme.drawSegment(context, item.frame, ds, item.state,
                                      mImpl->action == Action::kButton,
                                      item.isOn, showKeyFocus, i, nItems);
        }
        auto ws = context.theme.segmentTextStyle(item.state, ds, item.isOn);
        static_cast<IconAndText*>(children()[i])->setForegroundColorNoRedraw(ws.fgColor);
    }

    Super::draw(context);
}

}  // namespace uitk

