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

#include <map>

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

    // Creating text objects is expensive, particularly if you have a list
    // of, say, 1000 of them. So we cache all the text information. In particular
    // this helps
    // - drawing: we can draw cached text, also we do not create a text to
    //     compute the margins
    // - layout
    // - ListView: the preferred width of the ListView needs to query all the
    //     texts to determine the maximum width; caching the preferred size
    //     really speeds this up. This is especially noticable when resizing
    //     the window.
    mutable std::map<PicaPt, Size> preferredSizeByConstraintWidth;
    mutable float preferredSizeDpi = 0.0f;
    mutable Size drawMargins = Size::kZero;
    mutable std::shared_ptr<TextLayout> layout;
    mutable uint32_t layoutRGBA = 0;  // so we can compare colors (not helpful to compare a bunch of floats)

    // This should be called any time the text or font size would change.
    // Color and alignment do not affect the preferred size, just the layout.
    void clearPreferredSize()
    {
        this->preferredSizeByConstraintWidth.clear();
        this->preferredSizeDpi = 0.0f;
    }

    // This should be called any time the text needs to be recreated, which is when
    // pretty much anything changes. (In particular, color is part of the layout.)
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
        auto fm = dc.fontMetrics(font);
        auto margins = calcMargin(dc, theme);
        PicaPt w = size.width;
        if (size.width > PicaPt::kZero) {
            w -= 2.0f * margins.width;
        }
        PicaPt h = size.height;
        if (size.height > PicaPt::kZero) {
            // We want to keep the margins (so that, e.g. kTop is aligned to the bottom of
            // the top margin), but because the descent counts as part of the bottom margin
            // we need to adjust accordingly.
            if (this->alignment & Alignment::kVCenter) {
                h -= 2.0f * margins.height - 0.5f * fm.descent;
            } else {
                h -= 2.0f * margins.height - fm.descent;
            }
        }
        auto wrap = (this->wordWrap ? kWrapWord : kWrapNone);

        this->drawMargins = margins;
        return dc.createTextLayout(this->text, font, fg, Size(w, h), this->alignment, wrap);
    }

    Font currentFont(const Theme& theme) const
    {
        return (this->usesThemeFont ? theme.params().labelFont : this->customFont);
    }

    Size calcMargin(const DrawContext& dc, const Theme& theme)
    {
        return theme.calcPreferredTextMargins(dc, currentFont(theme));
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
    mImpl->clearPreferredSize();
    mImpl->clearLayout();
    setNeedsLayout();
    setNeedsDraw();
    return this;
}

bool Label::wordWrapEnabled() const { return mImpl->wordWrap; }

Label* Label::setWordWrapEnabled(bool enabled)
{
    mImpl->wordWrap = enabled;
    mImpl->clearPreferredSize();
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
    mImpl->clearPreferredSize();
    mImpl->clearLayout();
    return this;
}

const Color& Label::textColor() const { return mImpl->textColor; }

Label* Label::setTextColor(const Color& c)
{
    setForegroundColorNoRedraw(c);
    setNeedsDraw();
    return this;
}

void Label::setForegroundColorNoRedraw(const Color& c)
{
    // Layout is expensive, so try to avoid doing it. While this is called by setTextColor(),
    // it is also called by widgets that use the label as child object, and in that case the
    // text color is likely to actually be the same.
    bool needsRelayout = (c.toRGBA() != mImpl->layoutRGBA && !mImpl->text.text().empty());
    mImpl->textColor = c;
    mImpl->text.setColor(c);
    if (needsRelayout) {
        mImpl->clearLayout();
    }
}

Widget* Label::setFrame(const Rect& frame)
{
    // The layout is not dependent on the x,y position, only the width and height.
    // Since recomputing it is expensive, only do it if the width or height change.
    auto oldFrame = this->frame();
    if (oldFrame.width != frame.width || oldFrame.height != frame.height)
    {
        mImpl->clearLayout();
    }
    Super::setFrame(frame);
    return this;
}

AccessibilityInfo Label::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    info.type = AccessibilityInfo::Type::kLabel;
    info.text = mImpl->text.text();
    return info;
}

void Label::themeChanged()
{
    Super::themeChanged();

    // Clear both preferred size and layout, since text size may have changed.
    mImpl->clearPreferredSize();
    mImpl->clearLayout();
}

Size Label::preferredSize(const LayoutContext& context) const
{
    if (context.dc.dpi() != mImpl->preferredSizeDpi) {
        mImpl->clearPreferredSize();
        mImpl->preferredSizeDpi = context.dc.dpi();
    }

    auto it = mImpl->preferredSizeByConstraintWidth.find(context.constraints.width);
    if (it == mImpl->preferredSizeByConstraintWidth.end()) {
        auto fm = context.dc.fontMetrics(mImpl->currentFont(context.theme));
        auto margin = mImpl->calcMargin(context.dc, context.theme);
        auto constrainedWidth = kDimGrow;
        if (mImpl->wordWrap) {
            constrainedWidth = context.constraints.width;
        }
        auto tm = mImpl->createTextLayout(context.dc, context.theme, Color(0.5f, 0.5f, 0.5f),
                                          Size(constrainedWidth, PicaPt::kZero))->metrics();
        bool isOneLine = (tm.height < 1.5f * fm.lineHeight);
        Size pref;
        if (isOneLine) {
            pref = Size(context.dc.ceilToNearestPixel(tm.width) + 2.0f * margin.width,
                        context.dc.ceilToNearestPixel(fm.capHeight) + 2.0f * margin.height);
        } else {
            pref = Size(context.dc.ceilToNearestPixel(tm.width) + 2.0f * margin.width,
                        context.dc.ceilToNearestPixel(tm.height - (fm.ascent - fm.capHeight) - fm.descent) + 2.0f * margin.height);
        }
        mImpl->preferredSizeByConstraintWidth[context.constraints.width] = pref;
        it = mImpl->preferredSizeByConstraintWidth.find(context.constraints.width);
    }
    return it->second;
}

void Label::layout(const LayoutContext& context)
{
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
    // This is really r.upperLeft() + margin, but r.upperLeft() is always (0, 0)
    // Note: use the cached margins, calculating the margins creates a text object
    //       which is expensive. A ListView of text gets really slow to draw.
    ui.dc.drawText(*mImpl->layout, Point(mImpl->drawMargins.width, mImpl->drawMargins.height));

#if DEBUG_BASELINE
    auto onePx = ui.dc.onePixel();
    auto y = ui.dc.roundToNearestPixel(pt.y + mImpl->drawMargins.height) +
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
