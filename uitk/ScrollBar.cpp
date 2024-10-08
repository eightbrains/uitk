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

#include "ScrollBar.h"

#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct ScrollBar::Impl
{
    double thumbSizeFraction = 1.0;  // fraction relative to length, in [0, 1]
    bool thumbNeedsResize = false;
};

ScrollBar::ScrollBar(Dir dir)
    : SliderLogic(dir == Dir::kHoriz ? SliderDir::kHoriz : SliderDir::kVertZeroAtTop)
    , mImpl(new Impl())
{
}

ScrollBar::~ScrollBar()
{
}

ScrollBar* ScrollBar::setRange(double minValue, double maxValue,
                               double viewingSize, double contentSize)
{
    setLimits(minValue, maxValue);  // calls setNeedsDraw()
    if (contentSize > viewingSize && contentSize > 0.0f) {
        mImpl->thumbSizeFraction = viewingSize / contentSize;
    } else {
        mImpl->thumbSizeFraction = 1.0f;
    }
    mImpl->thumbNeedsResize = true;
    return this;
}

Widget* ScrollBar::setFrame(const Rect& frame)
{
    Super::setFrame(frame);
    mImpl->thumbNeedsResize = true;
    return this;
}

Size ScrollBar::preferredSize(const LayoutContext& context) const
{
    auto thickness = context.theme.calcPreferredScrollbarThickness(context.dc);
    if (direction() == SliderDir::kHoriz) {
        return Size(kDimGrow, thickness);
    } else {
        return Size(thickness, kDimGrow);
    }
}

Size ScrollBar::preferredThumbSize(const LayoutContext& context) const
{
    auto fm = context.theme.params().labelFont.metrics(context.dc);
    auto minLength = context.dc.roundToNearestPixel(fm.ascent + fm.descent);

    mImpl->thumbNeedsResize = true;
    double minValue = doubleMinLimit();
    double maxValue = doubleMaxLimit();
    auto thickness = context.theme.calcPreferredScrollbarThickness(context.dc);
    if (direction() == SliderDir::kHoriz) {
        return Size(std::max(minLength, float(mImpl->thumbSizeFraction) * frame().width), thickness);
    } else {
        return Size(thickness, std::max(minLength, float(mImpl->thumbSizeFraction) * frame().height));
    }
}

void ScrollBar::drawTrack(UIContext& context, const Point& thumbMid)
{
    context.theme.drawScrollbarTrack(context, direction(), bounds(), thumbMid,
                                     style(themeState()), themeState());
}

void ScrollBar::drawThumb(UIContext& context, Widget *thumb)
{
    if (mImpl->thumbNeedsResize) {
/*        auto thumbPref = preferredThumbSize(LayoutContext{context.theme, context.dc});
        auto thumbMid = thumb->frame().center();
        Rect r(thumbMid.x - 0.5f * thumbPref.width,
               thumbMid.y - 0.5f * thumbPref.height,
               thumbPref.width,
               thumbPref.height);
        if (direction() == SliderDir::kHoriz) {
            if (r.x < frame().x) {
                r.x = frame().x;
            } else if (r.maxX() > frame().maxX()) {
                r.x = frame().maxX() - r.width;
            }
        } else {
            if (r.y < frame().y) {
                r.y = frame().y;
            } else if (r.maxY() > frame().maxY()) {
                r.y = frame().maxY() - r.height;
            }
        } */
        mImpl->thumbNeedsResize = false;
    }

    bool alreadyDrew = false;
    auto *parent = this->parent();
    if (parent && (parent->style(Theme::WidgetState::kNormal).flags & Theme::WidgetStyle::Flags::kBGColorSet)) {
        float bgLightness = parent->style(Theme::WidgetState::kNormal).bgColor.toGrey().red();
        Color thumbColor = context.theme.params().scrollbarColor;
        float thumbLightness;
        if (style(themeState()).flags & Theme::WidgetStyle::Flags::kFGColorSet) {
            thumbColor = style(themeState()).bgColor;
        }
        thumbLightness = thumbColor.toGrey().red();
        if (std::abs(thumbLightness - bgLightness) < 0.2f) {
            auto s = style(themeState());  // copy
            if (bgLightness < 0.5f) {
                s.bgColor = Color(1.0f, 1.0f, 1.0f, thumbColor.alpha());
            } else {
                s.bgColor = Color(0.0f, 0.0f, 0.0f, thumbColor.alpha());
            }
            s.flags |= Theme::WidgetStyle::Flags::kBGColorSet;
            context.theme.drawScrollbarThumb(context, thumb->frame(), s, themeState());
            alreadyDrew = true;
        }
    }
    if (!alreadyDrew) {
        context.theme.drawScrollbarThumb(context, thumb->frame(), style(themeState()), themeState());
    }
}

}  // namespace uitk
