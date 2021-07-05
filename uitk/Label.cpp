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

#include "Label.h"

#include "UIContext.h"
#include "themes/Theme.h"

#include <nativedraw.h>

#define DEBUG_BASELINE 0

namespace uitk {

struct Label::Impl
{
    std::string text;
    int alignment = Alignment::kLeft | Alignment::kTop;
    Color textColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
};

Label::Label(const std::string& text)
    : mImpl(new Impl())
{
    mImpl->text = text;
}

Label::~Label()
{
}

const std::string& Label::text() const { return mImpl->text; }

Label* Label::setText(const std::string& text)
{
    mImpl->text = text;
    setNeedsDraw();
    return this;
}

int Label::alignment() const { return mImpl->alignment; }

Label* Label::setAlignment(int align)
{
    mImpl->alignment = align;
    setNeedsDraw();
    return this;
}

const Color& Label::textColor() const { return mImpl->textColor; }

Label* Label::setTextColor(const Color& c)
{
    mImpl->textColor = c;
    setNeedsDraw();
    return this;
}

void Label::setWidgetState(Theme::WidgetState state)
{
    setState(state);
}

Size Label::preferredSize(const LayoutContext& context) const
{
    auto &font = context.theme.params().labelFont;
    auto fm = context.dc.fontMetrics(font);
    auto tm = context.dc.textMetrics(mImpl->text.c_str(), font, kPaintFill);
    // Because the descent acts as the lower margin visually, all the other
    // margins should be the descent.
    auto margin = context.dc.ceilToNearestPixel(fm.descent);

    return Size(context.dc.ceilToNearestPixel(tm.width) + 2.0f * margin,
                context.dc.ceilToNearestPixel(fm.capHeight) + 2.0f * margin);
}

void Label::draw(UIContext& ui)
{
    Super::draw(ui);

    auto &r = bounds();
    ui.theme.drawFrame(ui, r, style(state()));

    auto &font = ui.theme.params().labelFont;
    auto metrics = font.metrics(ui.dc);
    auto margin = ui.dc.ceilToNearestPixel(metrics.descent);
    Point pt;
    if (mImpl->alignment & Alignment::kBottom) {
        pt.y = r.maxY() - (metrics.ascent + metrics.descent);
    } else if (mImpl->alignment & Alignment::kVCenter) {
        // Visually the descenders (if any) do not feel like they are part of the
        // block of text, so just the cap-height should be centered. However,
        // drawing will start from the ascent (which may be above the cap-height).
        // The descent below acts as the lower margin.
        pt.y = r.midY() - 0.5f * metrics.capHeight - (metrics.ascent - metrics.capHeight);
    } else {
        // The ascent value is kind of arbitrary, and many fonts seem to use it
        // to put the leading in, so it is taller than necessary (either that or there
        // are some really tall glyphs somewhere in those Unicode characters). The cap-height
        // is the visual ascent.
        pt.y = r.minY() + margin - (metrics.ascent - metrics.capHeight);
    }
    if (mImpl->alignment & Alignment::kRight) {
        auto width = ui.dc.textMetrics(mImpl->text.c_str(), font, kPaintFill).width;
        pt.x = r.maxX() - margin - width;
    } else if (mImpl->alignment & Alignment::kHCenter) {
        auto width = ui.dc.textMetrics(mImpl->text.c_str(), font, kPaintFill).width;
        pt.x = r.midX() - 0.5f * width;
    } else {
        pt.x = r.minX() + margin;
    }

    if (mImpl->textColor.alpha() == 0.0f) {  // color is unset
        ui.dc.setFillColor(ui.theme.params().textColor);
    } else {
        ui.dc.setFillColor(mImpl->textColor);
    }
    ui.dc.drawText(mImpl->text.c_str(), pt, font, kPaintFill);

#if DEBUG_BASELINE
    auto onePx = ui.dc.onePixel();
    auto y = ui.dc.roundToNearestPixel(pt.y) +
             ui.dc.floorToNearestPixel(metrics.ascent) +
             0.5 * onePx;

    // Draw the baseline in blue as thin as possible
    ui.dc.setStrokeColor(Color(0.0f, 0.0f, 1.0f, 0.7f));
    ui.dc.setStrokeWidth(onePx);
    ui.dc.drawLines({ Point(pt.x, y),
                      Point(pt.x + PicaPt(36), y)});
    // Draw the upper left pixel in green (this is top of ascent)
    ui.dc.setFillColor(Color::kGreen);
    ui.dc.drawRect(Rect(pt.x, pt.y, onePx, onePx), kPaintFill);
#endif // DEBUG_BASELINE
}

}  // namespace uitk

