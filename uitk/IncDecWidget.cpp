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

#include "IncDecWidget.h"

#include "Events.h"
#include "UIContext.h"

namespace uitk {

struct IncDecWidget::Impl
{
    enum MouseOverItem { kNone, kMouseOverInc, kMouseOverDec };
    MouseOverItem mouseOverItem = kNone;
    std::function<void(IncDecWidget*, int)> onClicked;
};

IncDecWidget::IncDecWidget()
    : mImpl(new IncDecWidget::Impl())
{
}

IncDecWidget::~IncDecWidget()
{
}

void IncDecWidget::setOnClicked(std::function<void(IncDecWidget*, int)> onClicked)
{
    mImpl->onClicked = onClicked;
}

Size IncDecWidget::preferredSize(const LayoutContext& context) const
{
    return context.theme.calcPreferredIncDecSize(context.dc);
}

Widget::EventResult IncDecWidget::mouse(const MouseEvent &e)
{
    auto oldItem = mImpl->mouseOverItem;
    mImpl->mouseOverItem = ((e.pos.y < bounds().midY()) ? Impl::kMouseOverInc : Impl::kMouseOverDec);
    if (mImpl->mouseOverItem != oldItem) {
        setNeedsDraw();
    }

    if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft && e.keymods == 0) {
        if (mImpl->onClicked) {
            mImpl->onClicked(this, (e.pos.y < bounds().midY()) ? 1 : -1);
        }
    }
    return Super::mouse(e);
}

void IncDecWidget::draw(UIContext& context)
{
    Super::draw(context);
    context.theme.drawIncDec(context, bounds(),
            mImpl->mouseOverItem == Impl::kMouseOverInc ? state() : Theme::WidgetState::kNormal,
            mImpl->mouseOverItem == Impl::kMouseOverDec ? state() : Theme::WidgetState::kNormal);
}

}  // namespace uitk
