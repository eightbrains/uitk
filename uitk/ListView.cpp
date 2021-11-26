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

#include "ListView.h"

#include "Events.h"
#include "Label.h"
#include "UIContext.h"
#include "themes/Theme.h"

#include <nativedraw.h>

#include <unordered_set>

namespace uitk {

namespace {

static const PicaPt kUnsetPadding(-10000.0f);

uitk::Size calcPadding(const uitk::LayoutContext& context, const Size& userPadding)
{
    if (userPadding.width != kUnsetPadding && userPadding.height != kUnsetPadding) {
        return userPadding;
    } else {
        auto fm = context.theme.params().labelFont.metrics(context.dc);
        auto em = fm.ascent + fm.descent;
        auto defaultPadding =  context.dc.roundToNearestPixel(0.25f * em);

        Size padding = userPadding;
        if (padding.width == kUnsetPadding) {
            padding.width = defaultPadding;
        }
        if (padding.height == kUnsetPadding) {
            padding.height = defaultPadding;
        }
        return padding;
    }
}

enum class LayoutMode { kCalcHeight, kLayout };

PicaPt layoutItems(const uitk::LayoutContext& context, LayoutMode mode,
                   const Rect& frame, const Size& userPadding, const std::vector<Widget*> children)
{
    auto padding = calcPadding(context, userPadding);

    auto fm = context.theme.params().labelFont.metrics(context.dc);
    auto em = fm.ascent + fm.descent;

    auto width = frame.width - 2.0f * padding.width;
    auto x = padding.width; // inset a little left and right in case cell draw bg and obscures the selection
    auto y = padding.height;
    for (auto *child : children) {
        auto pref = child->preferredSize(context);
        if (mode == LayoutMode::kLayout) {
            if (pref.height < Widget::kDimGrow) {
                child->setFrame(Rect(x, y, width, pref.height));
            } else {
                child->setFrame(Rect(x, y, width, em));
            }
        }
        y += pref.height;
    }

    return y + padding.height;
}

} // namespace

struct ListView::Impl
{
    Widget *content = nullptr;  // super owns this
    Size contentPadding = Size(kUnsetPadding, kUnsetPadding);
    SelectionMode selectionMode = SelectionMode::kSingleItem;
    std::unordered_set<int> selectedIndices;
    std::function<void(ListView*)> onChanged;
    int mouseOverIndex = -1;
    int lastClickedRow = 0;
};

ListView::ListView()
    : mImpl(new Impl())
{
    mImpl->content = new Widget();
    addChild(mImpl->content);
}

ListView::~ListView()
{
}

ListView* ListView::setOnSelectionChanged(std::function<void(ListView*)> onChanged)
{
    mImpl->onChanged = onChanged;
    return this;
}

ListView::SelectionMode ListView::selectionMode() const
{
    return mImpl->selectionMode;
}

ListView* ListView::setSelectionModel(SelectionMode mode)
{
    mImpl->selectionMode = mode;
    if (mode == SelectionMode::kNoItems && !mImpl->selectedIndices.empty()) {
        clearSelection();
    } else if (mode == SelectionMode::kSingleItem && mImpl->selectedIndices.size() > 1) {
        setSelectedIndex(*mImpl->selectedIndices.begin());
    }
    return this;
}

void ListView::clearCells()
{
    clearSelection();
    mImpl->content->clearAllChildren();
    mImpl->mouseOverIndex = -1;
}

ListView* ListView::addCell(Widget *cell)
{
    mImpl->content->addChild(cell);
    return this;
}

ListView* ListView::addStringCell(const std::string& text)
{
    mImpl->content->addChild(new Label(text));
    return this;
}

Widget* ListView::cellAtIndex(int index) const
{
    auto &childs = children();
    if (index < 0 || index >= int(childs.size())) {
        return nullptr;
    }
    return childs[index];
}

int ListView::selectedIndex() const
{
    if (mImpl->selectedIndices.empty()) {
        return -1;
    }
    return *mImpl->selectedIndices.begin();
}

std::vector<int> ListView::selectedIndices() const
{
    std::vector<int> selection;
    selection.reserve(mImpl->selectedIndices.size());
    selection.insert(selection.end(), mImpl->selectedIndices.begin(), mImpl->selectedIndices.end());
    return selection;
}

void ListView::clearSelection()
{
    mImpl->selectedIndices.clear();
    setNeedsDraw();
}

void ListView::setSelectedIndex(int index)
{
    mImpl->selectedIndices.clear();
    mImpl->selectedIndices.insert(index);
    setNeedsDraw();
}

void ListView::setSelectedIndices(const std::unordered_set<int> indices)
{
    mImpl->selectedIndices = indices;
    setNeedsDraw();
}

Size ListView::contentPadding() const
{
    return mImpl->contentPadding;
}

ListView* ListView::setContentPadding(const PicaPt& xPadding, const PicaPt& yPadding)
{
    mImpl->contentPadding = Size(xPadding, yPadding);
    return this;
}

Size ListView::preferredContentSize(const LayoutContext& context) const
{
    auto width = preferredSize(context).width;
    auto height = layoutItems(context, LayoutMode::kCalcHeight, frame(), mImpl->contentPadding,
                              mImpl->content->children());
    return Size(width, height);
}

Size ListView::preferredSize(const LayoutContext& context) const
{
    auto fm = context.theme.params().labelFont.metrics(context.dc);
    auto em = fm.ascent + fm.descent;

    auto padding = calcPadding(context, mImpl->contentPadding);
    auto width = PicaPt::kZero;
    for (auto *child : mImpl->content->children()) {
        width = std::max(width, child->preferredSize(context).width);
    }
    return Size(width + 2 * padding.width, kDimGrow);
}

void ListView::layout(const LayoutContext& context)
{
    auto &f = frame();
    auto height = layoutItems(context, LayoutMode::kLayout, f, mImpl->contentPadding,
                              mImpl->content->children());
    Rect contentRect(mImpl->content->frame().x, mImpl->content->frame().y, f.width, height);
    mImpl->content->setFrame(contentRect);
    setContentSize(Size(f.width, height));

    Super::layout(context);
}

Widget::EventResult ListView::mouse(const MouseEvent& e)
{
    if (e.type == MouseEvent::Type::kButtonUp && e.button.button == MouseButton::kLeft) {
        int idx = calcRowIndex(e.pos);
        bool isEnabled = (mImpl->content && size_t(idx) < mImpl->content->children().size() &&
                          mImpl->content->children()[idx]->enabled());
        if (idx >= 0 && isEnabled) {
            bool selectionChanged = false;
            if (mImpl->selectionMode == SelectionMode::kSingleItem) {
                setSelectedIndex(idx);
                selectionChanged = true;
            } else if (mImpl->selectionMode == SelectionMode::kMultipleItems) {
                if (e.keymods == 0) {
                    setSelectedIndex(idx);
                    selectionChanged = true;
                } else if (e.keymods == KeyModifier::kCtrl) {
                    auto it = mImpl->selectedIndices.find(idx);
                    if (it != mImpl->selectedIndices.end()) {
                        mImpl->selectedIndices.erase(it);
                    } else {
                        mImpl->selectedIndices.insert(idx);
                    }
                    setSelectedIndices(mImpl->selectedIndices);
                    selectionChanged = true;
                } else if (e.keymods == KeyModifier::kShift) {
                    int startIdx = std::min(idx, mImpl->lastClickedRow);
                    int endIdx = std::max(idx, mImpl->lastClickedRow);
                    for (int i = startIdx;  i <= endIdx;  ++i) {
                        mImpl->selectedIndices.insert(i);
                    }
                    setSelectedIndices(mImpl->selectedIndices);
                    selectionChanged = true;
                }
                mImpl->lastClickedRow = idx;
            }
            if (selectionChanged && mImpl->onChanged) {
                mImpl->onChanged(this);
            }
        }
    } else if (e.type == MouseEvent::Type::kMove || e.type == MouseEvent::Type::kDrag) {
        mImpl->mouseOverIndex = calcRowIndex(e.pos);
    }

    return Super::mouse(e);
}

void ListView::mouseExited()
{
    mImpl->mouseOverIndex = -1;
}

int ListView::calcRowIndex(const Point& p) const
{
    Point scrollP = p - bounds().upperLeft();
    auto &childs = mImpl->content->children();
    for (size_t i = 0;  i < childs.size();  ++i) {
        if (childs[i]->frame().contains(scrollP)) {
            auto debug = childs[i]->frame();
            return int(i);
        }
    }
    return -1;
}

void ListView::key(const KeyEvent& e)
{
    Super::key(e);
    if (mImpl->selectionMode == SelectionMode::kNoItems || e.type == KeyEvent::Type::kKeyUp) {
        return;
    }

    switch (e.key) {
        case Key::kDown: {
            int idx = std::max(-1, mImpl->mouseOverIndex) + 1;
            auto items = mImpl->content->children();
            int nItems = int(items.size());
            while (idx < nItems && !items[idx]->enabled()) {
                idx++;
            }
            if (idx < nItems) {
                mImpl->mouseOverIndex = idx;
                setNeedsDraw();
            }
            break;
        }
        case Key::kUp: {
            auto items = mImpl->content->children();
            int nItems = int(items.size());
            int idx = mImpl->mouseOverIndex - 1;
            if (idx < 0) {
                idx = nItems - 1;
            }
            while (idx >= 0 && !items[idx]->enabled()) {
                idx--;
            }
            if (idx >= 0) {
                mImpl->mouseOverIndex = idx;
                setNeedsDraw();
            }
            break;
        }
        case Key::kEnter:
        case Key::kReturn:
        case Key::kSpace:
            if (mImpl->mouseOverIndex >= 0) {
                bool changed = false;
                bool thisIndexIsSelected = (mImpl->selectedIndices.find(mImpl->mouseOverIndex) != mImpl->selectedIndices.end());
                if (thisIndexIsSelected) {
                    if (mImpl->selectionMode == SelectionMode::kSingleItem) {
                        // do nothing
                    } else {
                        auto selected = selectedIndices();
                        auto newSet = std::unordered_set<int>(selected.begin(), selected.end());
                        newSet.erase(mImpl->mouseOverIndex);
                        setSelectedIndices(newSet);
                        changed = true;
                    }
                } else {
                    if (mImpl->selectionMode == SelectionMode::kSingleItem) {
                        setSelectedIndex(mImpl->mouseOverIndex);
                    } else {
                        auto newSet = selectedIndices();
                        newSet.push_back(mImpl->mouseOverIndex);
                        setSelectedIndices(std::unordered_set<int>(newSet.begin(), newSet.end()));
                    }
                    changed = true;
                }
                if (changed && mImpl->onChanged) {
                    mImpl->onChanged(this);
                }
            }
            break;
        default:
            break;
    }
}

void ListView::draw(UIContext& context)
{
    Rect r(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);
    context.theme.drawListView(context, r, style(state()), state());

    context.dc.save();
    context.theme.clipListView(context, r, style(state()), state());
    context.dc.translate(bounds().x, bounds().y);
    Theme::WidgetStyle s;
    auto parentState = state();
    auto width = frame().width;
    // The mouseOverIndex can also be set by keyboard navigation, so don't
    // require the state to be mouseover in order to display it.
    // (mouseExited() will set it -1, so mousing will still work correctly)
    if (mImpl->mouseOverIndex >= 0) {
        if (mImpl->selectionMode != SelectionMode::kNoItems
            && mImpl->mouseOverIndex < mImpl->content->children().size()) {
            auto *item = mImpl->content->children()[mImpl->mouseOverIndex];
            auto r = item->frame();
            r.x = PicaPt::kZero;
            r.width = width;
            auto rowState = parentState;
            if (item->enabled()) {
                context.theme.drawListViewSpecialRow(context, r, style(state()), parentState);
            }
        }
    }
    auto stateForSelection = (parentState == Theme::WidgetState::kDisabled
                                ? parentState
                                : Theme::WidgetState::kNormal);
    for (auto idx : mImpl->selectedIndices) {
        auto r = mImpl->content->children()[idx]->frame();
        r.x = PicaPt::kZero;
        r.width = width;
        context.theme.drawListViewSpecialRow(context, r, s, stateForSelection);
    }
    context.dc.translate(-bounds().x, -bounds().y);
    context.dc.restore();

    Super::draw(context);
}

}  // namespace uitk
