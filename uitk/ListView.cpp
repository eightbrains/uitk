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

uitk::PicaPt calcMargin(const uitk::LayoutContext& context)
{
    auto fm = context.theme.params().labelFont.metrics(context.dc);
    auto em = fm.ascent + fm.descent;
    return context.dc.roundToNearestPixel(0.25f * em);
}

} // namespace

struct ListView::Impl
{
    Widget *content = nullptr;  // super owns this
    SelectionMode selectionMode = SelectionMode::kSingleItem;
    std::unordered_set<int> selectedIndices;
    std::function<void(ListView*)> onChanged;
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
}

void ListView::clearCells()
{
    clearSelection();
    mImpl->content->removeAllChildren();
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

Size ListView::preferredSize(const LayoutContext& context) const
{
    //auto margin = calcMargin(context);
    return Size(kDimGrow, kDimGrow);
}

void ListView::layout(const LayoutContext& context)
{
    auto margin = calcMargin(context);

    auto fm = context.theme.params().labelFont.metrics(context.dc);
    auto em = fm.ascent + fm.descent;

    auto width = frame().width - 2.0f * margin;
    auto x = margin; // inset a little left and right in case cell draw bg and obscures the selection
    auto y = margin;
    for (auto *child : mImpl->content->children()) {
        auto pref = child->preferredSize(context);
        if (pref.height < kDimGrow) {
            child->setFrame(Rect(x, y, width, pref.height));
        } else {
            child->setFrame(Rect(x, y, width, em));
        }
        y += pref.height;
    }

    Rect contentRect(mImpl->content->frame().x, mImpl->content->frame().y, width, y);
    mImpl->content->setFrame(contentRect);
    setContentSize(Size(width, contentRect.maxY() + margin));

    Super::layout(context);
}

Widget::EventResult ListView::mouse(const MouseEvent& e)
{
    if (e.type == MouseEvent::Type::kButtonUp && e.button.button == MouseButton::kLeft) {
        int idx = calcRowIndex(e.pos);
        if (idx >= 0) {
            if (mImpl->selectionMode == SelectionMode::kSingleItem) {
                setSelectedIndex(idx);
            } else if (mImpl->selectionMode == SelectionMode::kMultipleItems) {
                if (e.keymods == 0) {
                    setSelectedIndex(idx);
                } else if (e.keymods == KeyModifier::kCtrl) {
                    auto it = mImpl->selectedIndices.find(idx);
                    if (it != mImpl->selectedIndices.end()) {
                        mImpl->selectedIndices.erase(it);
                    } else {
                        mImpl->selectedIndices.insert(idx);
                    }
                    setSelectedIndices(mImpl->selectedIndices);
                } else if (e.keymods == KeyModifier::kShift) {
                    int startIdx = std::min(idx, mImpl->lastClickedRow);
                    int endIdx = std::max(idx, mImpl->lastClickedRow);
                    for (int i = startIdx;  i <= endIdx;  ++i) {
                        mImpl->selectedIndices.insert(i);
                    }
                    setSelectedIndices(mImpl->selectedIndices);
                }
                mImpl->lastClickedRow = idx;
            }
        }
    }

    return Super::mouse(e);
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
    for (auto idx : mImpl->selectedIndices) {
        auto r = mImpl->content->children()[idx]->frame();
        r.x = PicaPt::kZero;
        r.width = width;
        context.theme.drawListViewSelectedRow(context, r, s, parentState);
    }
    context.dc.translate(-bounds().x, -bounds().y);
    context.dc.restore();

    Super::draw(context);
}

}  // namespace uitk
