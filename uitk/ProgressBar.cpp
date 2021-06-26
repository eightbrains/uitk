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

#include "ProgressBar.h"

#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct ProgressBar::Impl
{
    float value = 0;
};

ProgressBar::ProgressBar()
    : mImpl(new Impl())
{
}

ProgressBar::~ProgressBar()
{
}

float ProgressBar::value() const { return mImpl->value; }

ProgressBar* ProgressBar::setValue(float percent)
{
    mImpl->value = std::max(0.0f, std::min(100.0f, percent));
    setNeedsDraw();
    return this;
}

Size ProgressBar::preferredSize(const LayoutContext& context) const
{
    return Size(kDimGrow, context.theme.calcPreferredProgressBarSize(context.dc).height);
}

void ProgressBar::draw(UIContext& context)
{
    Super::draw(context);

    context.theme.drawProgressBar(context, bounds(), mImpl->value, style(state()), state());
}

}  // namespace uitk


