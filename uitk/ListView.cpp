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
#include "Window.h"
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
    auto contextWithWidth = context.withWidth(width);
    auto x = padding.width; // inset a little left and right in case cell draw bg and obscures the selection
    auto y = padding.height;
    for (auto *child : children) {
        auto pref = child->preferredSize(contextWithWidth);
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
    bool keyNavigationWraps = false;
    std::unordered_set<int> selectedIndices;
    std::function<void(ListView*)> onChanged;
    std::function<void(ListView*, int)> onDblClicked;
    int mouseOverIndex = -1;
    int lastClickedRow = 0;

    void setMouseOverIndex(int idx)
    {
        auto items = this->content->children();
        if (this->selectionMode == SelectionMode::kNoItems) {
            this->mouseOverIndex = -1;
            return;
        }

        bool idxChanged = (idx != this->mouseOverIndex);
        bool stateChanged = false;
        if (idxChanged && this->mouseOverIndex >= 0 && this->mouseOverIndex < int(items.size())) {
            if (this->selectedIndices.find(this->mouseOverIndex) != this->selectedIndices.end()) {
                items[this->mouseOverIndex]->setThemeState(Theme::WidgetState::kSelected);
            } else {
                items[this->mouseOverIndex]->setThemeState(Theme::WidgetState::kNormal);
            }
        }
        this->mouseOverIndex = idx;
        if (idx >= 0 && idx < int(items.size())) {
            if (this->selectedIndices.find(idx) != this->selectedIndices.end()) {
                stateChanged |= (items[idx]->themeState() == Theme::WidgetState::kSelected);
                items[idx]->setThemeState(Theme::WidgetState::kSelected);
            } else {
                stateChanged |= (items[idx]->themeState() == Theme::WidgetState::kMouseOver);
                items[idx]->setThemeState(Theme::WidgetState::kMouseOver);
            }
        }
        // We really want the ListView to redraw, but Impl does not know to do that.
        // Conveniently, this will cause the whole chain to be redrawn.
        if (idxChanged || stateChanged) {
            this->content->setNeedsDraw();
        }
    }
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

ListView* ListView::setOnSelectionDoubleClicked(std::function<void(ListView*, int)> onDblClicked)
{
    mImpl->onDblClicked = onDblClicked;
    return this;
}

ListView::SelectionMode ListView::selectionMode() const
{
    return mImpl->selectionMode;
}

ListView* ListView::setSelectionModel(SelectionMode mode)
{
    mImpl->selectionMode = mode;
    if (mode == SelectionMode::kNoItems) {
        clearSelection();
        mImpl->setMouseOverIndex(-1);
        for (auto *child : mImpl->content->children()) {
            if (child->state() != MouseState::kNormal && child->state() != MouseState::kDisabled) {
                child->mouseExited();  // set state to normal
            }
        }
    } else if (mode == SelectionMode::kSingleItem && mImpl->selectedIndices.size() > 1) {
        setSelectedIndex(*mImpl->selectedIndices.begin());
    }
    return this;
}

int ListView::size() const
{
    return int(mImpl->content->children().size());
}

void ListView::clearCells()
{
    clearSelection();
    mImpl->content->clearAllChildren();
    mImpl->setMouseOverIndex(-1);
    setContentOffset(Point::kZero);
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
    auto &childs = mImpl->content->children();
    if (index < 0 || index >= int(childs.size())) {
        return nullptr;
    }
    return childs[index];
}

Widget* ListView::removeCellAtIndex(int index)
{
    auto& childs = mImpl->content->children();
    if (index < 0 || index >= int(childs.size())) {
        return nullptr;
    }

    clearSelection(); // changes all the indices of cells below it, so easiest to just clear
    mImpl->setMouseOverIndex(-1);
    auto *w = childs[index];
    mImpl->content->removeChild(w);
    return w;
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
    auto items = mImpl->content->children();
    for (int idx : mImpl->selectedIndices) {
        if (idx >= 0 && idx < int(items.size())) {
            items[idx]->resetThemeState();
        }
    }
    mImpl->selectedIndices.clear();
    setNeedsDraw();
}

void ListView::setSelectedIndex(int index)
{
    clearSelection();
    setSelectedIndices({index});
    if (!isRowVisible(index)) {
        scrollRowVisible(index);
    }
}

void ListView::setSelectedIndices(const std::unordered_set<int> indices)
{
    auto &items = mImpl->content->children();
    for (int idx : mImpl->selectedIndices) {
        if (idx >= 0 && idx < int(items.size())) {
            items[idx]->setThemeState(Theme::WidgetState::kNormal);
        }
    }

    mImpl->selectedIndices = indices;

    for (int idx : mImpl->selectedIndices) {
        if (idx >= 0 && idx < int(items.size())) {
            items[idx]->setThemeState(Theme::WidgetState::kSelected);
        }
    }
    setNeedsDraw();
}

int ListView::highlightedIndex() const { return mImpl->mouseOverIndex; }

void ListView::setHighlightedIndex(int idx)
{
    mImpl->setMouseOverIndex(idx);
}

bool ListView::keyNavigationWraps() const { return mImpl->keyNavigationWraps; }

void ListView::setKeyNavigationWraps(bool wraps) { mImpl->keyNavigationWraps = wraps; }

void ListView::triggerOnSelectionChanged()
{
    if (mImpl->onChanged) {
        mImpl->onChanged(this);
    }
}

bool ListView::isRowVisible(int index) const
{
    auto &items = mImpl->content->children();
    if (index < 0 || index >= int(items.size())) {
        return false;
    }
    auto r = Rect(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);
    auto scrollOffset = bounds().upperLeft();
    auto rowRect = items[index]->frame().translated(scrollOffset.x, scrollOffset.y);
    return (rowRect.y >= r.y && rowRect.maxY() <= r.maxY());
}

void ListView::scrollRowVisible(int index)
{
    auto &items = mImpl->content->children();
    if (index < 0 || index >= int(items.size())) {
        return;
    }
    auto r = items[index]->frame();
    auto scrollOffset = items[index]->bounds().upperLeft();
    auto minYVisible = -scrollOffset.y;
    auto maxYVisible = r.height - scrollOffset.y;
    if (r.y < minYVisible || r.maxY() > maxYVisible) {
        auto newYOffset = r.midY() - 0.5f * frame().height;
        newYOffset = std::max(PicaPt::kZero, newYOffset);
        newYOffset = std::min(bounds().height - frame().height, newYOffset);
        setContentOffset(Point(scrollOffset.x, -newYOffset));
    }
}

void ListView::scrollRowVisibleAtTop(int index)
{
    auto &items = mImpl->content->children();
    if (index < 0 || index >= int(items.size())) {
        return;
    }
    auto newYOffset = items[index]->frame().minY();
    newYOffset = std::max(PicaPt::kZero, newYOffset);
    newYOffset = std::min(bounds().height - frame().height, newYOffset);
    setContentOffset(Point(bounds().x, -newYOffset));
}

void ListView::scrollRowVisibleAtBottom(int index)
{
    auto &items = mImpl->content->children();
    if (index < 0 || index >= int(items.size())) {
        return;
    }
    auto newYOffset = items[index]->frame().maxY() - frame().height;
    newYOffset = std::max(PicaPt::kZero, newYOffset);
    newYOffset = std::min(bounds().height - frame().height, newYOffset);
    setContentOffset(Point(bounds().x, -newYOffset));
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
    if (mImpl->selectionMode == SelectionMode::kNoItems) {
        return EventResult::kConsumed;
    }

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
            }
            mImpl->lastClickedRow = idx;
            if (selectionChanged && mImpl->onChanged) {
                mImpl->onChanged(this);
            }
        }
        // Normally clicking does not give focus, but we want key navigation
        // to work. But we do not want the visible focus ring, on the click
        // (this is macOS behavior).
        if (auto win = window()) {
            win->setFocusWidget(this, Window::ShowFocusRing::kNo);
        }
    } else if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft && e.button.nClicks == 2) {
        // If we are double-clicking, we are on the same item (if the mouse
        // had moved, it would not be double-click). So we do not want to
        // redo the selection, just call the double-click handler (if any)
        int idx = calcRowIndex(e.pos);
        if (idx >= 0 && mImpl->onDblClicked) {
            mImpl->onDblClicked(this, idx);
        }
    } else if (e.type == MouseEvent::Type::kMove || e.type == MouseEvent::Type::kDrag) {
        mImpl->setMouseOverIndex(calcRowIndex(e.pos));
    }

    auto retval = Super::mouse(e);

    // Since super scrolls we cannot set the mouse over index until after we super,
    // otherwise we get a frame behind. This is a problem when scrolling quickly
    // through a long menu (e.g. fonts).
    if (e.type == MouseEvent::Type::kScroll) {
        mImpl->setMouseOverIndex(calcRowIndex(e.pos));
    }

    return retval;
}

void ListView::mouseExited()
{
    mImpl->setMouseOverIndex(-1);
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

bool ListView::acceptsKeyFocus() const { return true; }

Widget::EventResult ListView::key(const KeyEvent& e)
{
    auto result = Super::key(e);
    if (result != EventResult::kIgnored ||
        mImpl->selectionMode == SelectionMode::kNoItems || e.type == KeyEvent::Type::kKeyUp) {
        return result;
    }

    // At this point, we know we have a key down event, and selection mode
    // is single or multi.
    result = EventResult::kIgnored;
    int idx = mImpl->lastClickedRow;
    if (e.key == Key::kDown || e.key == Key::kUp) {
        int origIdx = idx;
        if (e.key == Key::kDown) {
            ++idx;
            if (idx >= int(size())) {
                if (keyNavigationWraps()) {
                    idx = 0;
                } else {
                    idx = int(size() - 1);
                }
            }
            if (!isRowVisible(idx)) {
                scrollRowVisibleAtBottom(idx);
            }
        } else {
            --idx;
            if (idx < 0) {
                if (keyNavigationWraps()) {
                    idx = int(size() - 1);
                } else {
                    idx = 0;
                }
            }
            if (!isRowVisible(idx)) {
                scrollRowVisibleAtTop(idx);
            }
        }
        mImpl->lastClickedRow = idx;
        if (e.keymods == 0) {
            setSelectedIndex(idx);
        } else if (mImpl->selectionMode == SelectionMode::kMultipleItems && e.keymods & KeyModifier::kShift) {
            std::unordered_set<int> newSelection;
            auto selection = selectedIndices();
            for (int sel : selection) {
                newSelection.insert(sel);
            }
            auto it = newSelection.find(idx);
            if (it == newSelection.end()) {
                newSelection.insert(idx);  // expanding selection: add this index
            } else {
                newSelection.erase(origIdx);  // shrinking selection: remove orig index
            }
            setSelectedIndices(newSelection);
        }
        if (mImpl->onChanged) {
            mImpl->onChanged(this);
        }
        result = EventResult::kConsumed;
    }

    return result;
}

void ListView::draw(UIContext& context)
{
    auto items = mImpl->content->children();

    Rect r(PicaPt::kZero, PicaPt::kZero, frame().width, frame().height);
    context.theme.drawListView(context, r, style(themeState()), themeState());

    context.dc.save();
    context.theme.clipListView(context, r, style(themeState()), themeState());
    context.dc.translate(bounds().x, bounds().y);
    Theme::WidgetStyle s;
    auto parentState = themeState();
    auto width = frame().width;
    // The mouseOverIndex can also be set by keyboard navigation, so don't
    // require the state to be mouseover in order to display it.
    // (mouseExited() will set it -1, so mousing will still work correctly)
    if (mImpl->mouseOverIndex >= 0 && mImpl->mouseOverIndex < int(items.size())) {
        if (mImpl->selectionMode != SelectionMode::kNoItems) {
            auto *item = items[mImpl->mouseOverIndex];
            auto r = item->frame();
            r.x = PicaPt::kZero;
            r.width = width;
            auto rowState = parentState;
            if (item->enabled()) {
                context.theme.drawListViewSpecialRow(context, r, style(themeState()), parentState);
            }
        }
    }
    auto stateForSelection = (parentState == Theme::WidgetState::kDisabled
                                ? parentState
                                : Theme::WidgetState::kNormal);
    for (auto idx : mImpl->selectedIndices) {
        if (idx >= 0 && idx < int(items.size())) {
            auto r = items[idx]->frame();
            r.x = PicaPt::kZero;
            r.width = width;
            context.theme.drawListViewSpecialRow(context, r, s, stateForSelection);
        }
    }
    context.dc.translate(-bounds().x, -bounds().y);
    context.dc.restore();

    Super::draw(context);
}

}  // namespace uitk
