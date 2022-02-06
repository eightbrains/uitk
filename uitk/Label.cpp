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

#include "Application.h"
#include "UIContext.h"
#include "Window.h"
#include "themes/Theme.h"

#include <nativedraw.h>

#define DEBUG_BASELINE 0

namespace uitk {

struct Label::Impl
{
    bool wordWrap = false;
    int alignment = Alignment::kLeft | Alignment::kTop;
    Color textColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
    bool usesThemeFont = true;
    Font customFont;
    Text text;

    mutable std::shared_ptr<TextLayout> layout;
    mutable uint32_t layoutRGBA = 0;  // so we can compare colors (not helpful to compare a bunch of floats)

    void clearLayout()
    {
        this->layout.reset();
        this->layoutRGBA = 0;
    }

    void updateTextLayout(const DrawContext& dc, const Theme& theme, const Color& fg, const Size& size)
    {
        this->layout = createTextLayout(dc, theme, fg, size);
        this->layoutRGBA = fg.toRGBA();
    }

    std::shared_ptr<TextLayout> createTextLayout(const DrawContext& dc, const Theme& theme,
                                                 const Color& fg, const Size& size)
    {
        auto font = (this->usesThemeFont ? theme.params().labelFont : this->customFont);
        auto margin = calcMargin(dc, theme);
        PicaPt w = size.width;
        if (size.width > PicaPt::kZero) {
            w -= 2.0f * margin;
        }
        PicaPt h = size.height;
        if (size.height > PicaPt::kZero) {
            h -= 2.0f * margin;
        }
        auto wrap = (this->wordWrap ? kWrapWord : kWrapNone);
        return dc.createTextLayout(this->text, font, fg, Size(w, h), this->alignment, wrap);
    }

    PicaPt calcMargin(const DrawContext& dc, const Theme& theme)
    {
        // Because the descent acts as the lower margin visually, all the other
        // margins should be the descent.
        auto font = (this->usesThemeFont ? theme.params().labelFont : this->customFont);
        auto metrics = font.metrics(dc);
        auto margin = dc.ceilToNearestPixel(metrics.descent);
        return margin;
    }
};

Label::Label(const std::string& text)
    : mImpl(new Impl())
{
    mImpl->text = Text(text, Font(), Color::kTextDefault);
}

Label::Label(const Text& text)
    : mImpl(new Impl())
{
    mImpl->text = text;
}

Label::~Label()
{
}

const std::string& Label::text() const { return mImpl->text.text(); }

Label* Label::setText(const std::string& text)
{
    return setRichText(Text(text, Font(), Color::kTextDefault));
}

const Text& Label::richText() const { return mImpl->text; }

Label* Label::setRichText(const Text& richText)
{
    mImpl->text = richText;
    mImpl->clearLayout();
    setNeedsLayout();
    setNeedsDraw();
    return this;
}

bool Label::wordWrapEnabled() const { return mImpl->wordWrap; }

Label* Label::setWordWrapEnabled(bool enabled)
{
    mImpl->wordWrap = enabled;
    mImpl->clearLayout();
    setNeedsDraw();
    return this;
}

int Label::alignment() const { return mImpl->alignment; }

Label* Label::setAlignment(int align)
{
    mImpl->alignment = align;
    mImpl->clearLayout();
    setNeedsDraw();
    return this;
}

Font Label::font() const
{
    if (mImpl->usesThemeFont) {
        return Application::instance().theme()->params().labelFont;
    } else {
        return mImpl->customFont;
    }
}

Label* Label::setFont(const Font& font)
{
    mImpl->usesThemeFont = false;
    mImpl->customFont = font;
    mImpl->clearLayout();
    return this;
}

const Color& Label::textColor() const { return mImpl->textColor; }

Label* Label::setTextColor(const Color& c)
{
    setTextColorNoRedraw(c);
    setNeedsDraw();
    return this;
}

void Label::setTextColorNoRedraw(const Color& c)
{
    mImpl->textColor = c;
    mImpl->text.setColor(c);
    mImpl->clearLayout();
}

Widget* Label::setFrame(const Rect& frame)
{
    Super::setFrame(frame);
    mImpl->clearLayout();
    return this;
}

Size Label::preferredSize(const LayoutContext& context) const
{
    auto &font = context.theme.params().labelFont;
    auto fm = context.dc.fontMetrics(font);
    auto margin = mImpl->calcMargin(context.dc, context.theme);
    auto constrainedWidth = kDimGrow;
    if (mImpl->wordWrap) {
        constrainedWidth = context.constraints.width;
    }
    auto tm = mImpl->createTextLayout(context.dc, context.theme, Color(0.5f, 0.5f, 0.5f),
                                      Size(constrainedWidth, PicaPt::kZero))->metrics();
    bool isOneLine = (tm.height < 1.5f * fm.lineHeight);
    if (isOneLine) {
        return Size(context.dc.ceilToNearestPixel(tm.width) + 2.0f * margin,
                    context.dc.ceilToNearestPixel(fm.capHeight) + 2.0f * margin);
    } else {
        return Size(context.dc.ceilToNearestPixel(tm.width) + 2.0f * margin,
                    context.dc.ceilToNearestPixel(tm.height - (fm.ascent - fm.capHeight) - fm.descent) + 2.0f * margin);
    }
}

void Label::layout(const LayoutContext& context)
{
    mImpl->clearLayout();
    Super::layout(context);
}

void Label::draw(UIContext& ui)
{
    Super::draw(ui);

    auto &r = bounds();
    auto &themeStyle = style(themeState());
    ui.theme.drawFrame(ui, r, themeStyle);

    Color fg;
    if (mImpl->textColor.alpha() == 0.0f) {  // color is unset
         fg = ui.theme.labelStyle(themeStyle, themeState()).fgColor;
     } else {
         fg = mImpl->textColor;
     }
    if (!mImpl->layout || fg.toRGBA() != mImpl->layoutRGBA) {
        mImpl->updateTextLayout(ui.dc, ui.theme, fg, r.size());
    }
    auto margin = mImpl->calcMargin(ui.dc, ui.theme);
    // This is really r.upperLeft() + margin, but r.upperLeft() is always (0, 0)
    ui.dc.drawText(*mImpl->layout, Point(margin, margin));

#if DEBUG_BASELINE
    auto onePx = ui.dc.onePixel();
    auto y = ui.dc.roundToNearestPixel(pt.y + margin) +
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

