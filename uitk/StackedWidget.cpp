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

#include "StackedWidget.h"

#include <algorithm>

namespace uitk {

struct StackedWidget::Impl
{
    int index = kNoIndex;
    PreferredSize preferredSizeAlgo = PreferredSize::kMaxPanelSize;
};

StackedWidget::StackedWidget()
    : mImpl(new Impl())
{
}

StackedWidget::~StackedWidget()
{
}

StackedWidget* StackedWidget::addPanel(Widget *w)
{
    Super::addChild(w);
    if (children().size() == 1) {
        setIndexShowing(0);
    } else {
        w->setVisible(false);
    }
    return this;
}

Widget* StackedWidget::removePanel(Widget *w)
{
    w->setVisible(true);
    Super::removeChild(w);
    setIndexShowing(mImpl->index);
    return w;
}

int StackedWidget::indexShowing() const { return mImpl->index; }

void StackedWidget::setIndexShowing(int index)
{
    auto &panels = children();
    index = std::min(index, int(panels.size()));
    mImpl->index = index;

    for (size_t i = 0;  i < panels.size();  ++i) {
        auto *p = panels[i];
        if (i == size_t(index)) {
            p->setVisible(true);
        } else {
            p->setVisible(false);
        }
    }

    updateKeyFocusOnVisibilityOrEnabledChange();
}

Widget* StackedWidget::currentPanel() const
{
    auto &panels = children();
    if (mImpl->index >= 0 && mImpl->index < panels.size()) {
        return children()[mImpl->index];
    }
    return nullptr;
}

StackedWidget::PreferredSize StackedWidget::preferredSizeCalculation() const
    { return mImpl->preferredSizeAlgo; }

StackedWidget* StackedWidget::setPreferredizeCalculation(PreferredSize& mode)
{
    mImpl->preferredSizeAlgo = mode;
    setNeedsLayout();
    return this;
}

Size StackedWidget::preferredSize(const LayoutContext& context) const
{
    Size size;
    if (mImpl->preferredSizeAlgo == PreferredSize::kMaxPanelSize) {
        for (auto *p : children()) {
            auto pref = p->preferredSize(context);
            size.width = std::max(size.width, pref.width);
            size.height = std::max(size.height, pref.height);
        }
    } else {
        auto &panels = children();
        if (mImpl->index >= 0 && mImpl->index < panels.size()) {
            return panels[mImpl->index]->preferredSize(context);
        }
    }
    return size;
}

void StackedWidget::layout(const LayoutContext& context)
{
    auto &panels = children();
    for (auto *p : panels) {
        p->setFrame(bounds());
    }
    Super::layout(context);
}

}  // namespace uitk
