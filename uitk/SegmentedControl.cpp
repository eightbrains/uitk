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

#include "SegmentedControl.h"

#include "Events.h"
#include "Label.h"
#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct SegmentedControl::Impl
{
    struct Item {
        std::string name;
        Theme::WidgetState state = Theme::WidgetState::kNormal;
        bool isOn = false;
    };
    std::vector<Item> items;
    std::function<void(int)> onClicked;
    Action action = Action::kButton;
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

    auto *label = new Label(name);
    label->setAlignment(Alignment::kCenter);
    addChild(label);  // calls setNeedsDraw(), we don't need to
}

SegmentedControl::Action SegmentedControl::action() const
{
    return mImpl->action;
}

SegmentedControl* SegmentedControl::setAction(Action act)
{
    mImpl->action = act;
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
        return;
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
}

SegmentedControl* SegmentedControl::setOnClicked(std::function<void(int)> onClicked)
{
    mImpl->onClicked = onClicked;
}

Size SegmentedControl::preferredSize(const LayoutContext& context) const
{
    Size pref;
    auto font = context.theme.params().labelFont;
    for (auto &item : mImpl->items) {
        Size segPref = context.theme.calcPreferredSegmentSize(context.dc, font, item.name);
        pref.width += segPref.width;
        pref.height = std::max(pref.height, segPref.height);
    }
    return pref;
}

void SegmentedControl::layout(const LayoutContext& context)
{
    if (!mImpl->items.empty()) {
        auto b = bounds();
        PicaPt total = PicaPt::kZero;
        std::vector<PicaPt> widths;
        widths.reserve(mImpl->items.size());
        auto font = context.theme.params().labelFont;
        for (auto &item : mImpl->items) {
            Size pref = context.theme.calcPreferredSegmentSize(context.dc, font, item.name);
            widths.push_back(pref.width);
            total += pref.width;
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
            child->setFrame(Rect(x, PicaPt::kZero, widths[i], b.height));
            x += widths[i];
            i++;
        }
    }

    Super::layout(context);
}

Widget::EventResult SegmentedControl::mouse(const MouseEvent& e)
{
    auto oldState = state();
    Super::mouse(e);
    auto newState = state();

    if (e.type == MouseEvent::Type::kButtonUp) {
        for (size_t i = 0;  i < mImpl->items.size();  ++i) {
            if (children()[i]->frame().contains(e.pos)) {
                switch (mImpl->action) {
                    case Action::kSelectOne:
                        setSegmentOn(i, true);
                        break;
                    case Action::kSelectMultiple:
                        setSegmentOn(i, !mImpl->items[i].isOn);
                        break;
                    case Action::kButton:
                        break;  // do nothing, button action is not set on/off
                }
                if (mImpl->onClicked) {
                    mImpl->onClicked(int(i));
                }
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
}

void SegmentedControl::mouseExited()
{
    for (auto &item : mImpl->items) {
        item.state = Theme::WidgetState::kNormal;
    }
    setNeedsDraw();  // might not actually need this, caller might call this?
}

void SegmentedControl::draw(UIContext& context)
{
    auto ctrlState = enabled() ? Theme::WidgetState::kNormal : Theme::WidgetState::kDisabled;
    context.theme.drawSegmentedControl(context, bounds(), style(ctrlState), ctrlState);
    int nItems = int(mImpl->items.size());
    for (int i = 0;  i < nItems;  ++i) {
        auto &item = mImpl->items[i];
        if (item.isOn || item.state != Theme::WidgetState::kNormal) {
            context.theme.drawSegment(context, children()[i]->frame(), item.state,
                                      mImpl->action == Action::kButton,
                                      item.isOn, i, nItems);
        }
        auto ws = context.theme.segmentTextStyle(item.state, item.isOn);
        static_cast<Label*>(children()[i])->setTextColor(ws.fgColor);
    }

    const auto &childs = children();
    for (size_t i = 1;  i < childs.size();  ++i) {
        auto *child = childs[i];
        context.theme.drawSegmentDivider(context, child->frame().upperLeft(), child->frame().lowerLeft(),
                                         style(ctrlState), ctrlState);
    }

    Super::draw(context);
}

}  // namespace uitk

