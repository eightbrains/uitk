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

#include "Checkbox.h"

#include "UIContext.h"
#include "themes/Theme.h"

namespace {
uitk::PicaPt calcSpacing(const uitk::DrawContext& dc, const uitk::Font& font)
{
    auto fm = dc.fontMetrics(font);
    return dc.ceilToNearestPixel(0.1f * (fm.ascent + fm.descent));
}

}  // namespace

namespace uitk {

Checkbox::Checkbox(const std::string& text)
    : Button(text)
{
    setToggleable(true);
    label()->setAlignment(Alignment::kLeft | Alignment::kVCenter);
}

Checkbox::~Checkbox()
{
}

Size Checkbox::preferredSize(const LayoutContext& context) const
{
    auto font = context.theme.params().labelFont;
    auto boxSize = context.theme.calcPreferredCheckboxSize(context.dc, font);
    auto labelSize = label()->preferredSize(context);
    return Size(boxSize.width + calcSpacing(context.dc, font) + labelSize.width,
                boxSize.height);
}

void Checkbox::layout(const LayoutContext& context)
{
    auto font = context.theme.params().labelFont;
    auto f = frame();
    auto x = PicaPt::kZero + f.height + calcSpacing(context.dc, font);
    label()->setFrame(Rect(x, PicaPt::kZero, f.width - x, f.height));
}

void Checkbox::draw(UIContext& context)
{
    auto size = bounds().height;
    Rect r(bounds().x, bounds().y, size, size);
    context.theme.drawCheckbox(context, r, style(state()), state(), isOn());

    // Unsually, we don't want to Super::draw(), because Button changes
    // the text's color. So we're going to skip to Widget::draw().
    Widget::draw(context);
}

}  // namespace uitk
