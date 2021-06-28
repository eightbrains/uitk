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

#include "Slider.h"

#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

Slider::Slider()
    : SliderLogic(SliderDir::kHoriz)
{
}

Slider::~Slider()
{
}

Size Slider::preferredThumbSize(const LayoutContext& context) const
{
    return context.theme.calcPreferredSliderThumbSize(context.dc);
}

void Slider::drawTrack(UIContext& context, const Point& thumbMid)
{
    context.theme.drawSliderTrack(context, direction(), bounds(), thumbMid, style(state()), state());
}

void Slider::drawThumb(UIContext& context, Widget *thumb)
{
    context.theme.drawSliderThumb(context, thumb->frame(),
                                  thumb->style(thumb->state()), thumb->state());
}

}  // namespace
