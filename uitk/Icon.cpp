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

#include "Icon.h"

#include "Application.h"
#include "UIContext.h"
#include "themes/IconPainter.h"

namespace uitk {

struct Icon::Impl
{
    Theme::StandardIcon icon = Theme::StandardIcon::kNone;
    Theme::Icon drawFunc;
    Color fg = Color(0.0f, 0.0f, 0.0f, 0.0f);
};

Icon::Icon(Theme::StandardIcon icon)
    : mImpl(new Impl())
{
    mImpl->icon = icon;
}

Icon::Icon(Theme::Icon drawFunc)
    : mImpl(new Impl())
{
    mImpl->drawFunc = drawFunc;
}

bool Icon::isEmpty() const
{
    return (mImpl->icon == Theme::StandardIcon::kNone && !mImpl->drawFunc);
}

Icon* Icon::setIcon(Theme::Icon icon)
{
    bool wasEmpty = isEmpty();

    mImpl->icon = Theme::StandardIcon::kNone;
    mImpl->drawFunc = icon;

    if (wasEmpty != isEmpty()) {
        setNeedsLayout();
    }
    return this;
}

Icon* Icon::setIcon(Theme::StandardIcon icon)
{
    bool wasEmpty = isEmpty();

    mImpl->icon = icon;
    mImpl->drawFunc = nullptr;

    if (wasEmpty != isEmpty()) {
        setNeedsLayout();
    }
    return this;
}

const Color& Icon::color() const
{
    return mImpl->fg;
}

Icon* Icon::setColor(const Color& fg)
{
    mImpl->fg = fg;
    setNeedsDraw();
    return this;
}

void Icon::setColorNoRedraw(const Color& fg)
{
    mImpl->fg = fg;
}

Size Icon::preferredSize(const LayoutContext& context) const
{
    auto em = context.theme.params().labelFont.pointSize();
    return Size(em, em);
}

void Icon::draw(UIContext& context)
{
    Super::draw(context);
    auto& border = borderWidth();
    bool hasFrame = (border > PicaPt::kZero && borderColor().alpha() > 0.0f);
    auto size = bounds().size();

    auto painter = Application::instance().iconPainter();
    auto fg = context.theme.params().textColor;
    if (mImpl->fg.alpha() > 0.0f) {
        fg = mImpl->fg;
    }

    Rect iconRect(border, border, size.width - border, size.height - border);
    if (mImpl->drawFunc) {
        context.theme.drawIcon(context, iconRect, mImpl->drawFunc, fg);
         mImpl->drawFunc(context.dc, context.theme, iconRect, fg);
    } else {
        context.theme.drawIcon(context, iconRect, mImpl->icon, fg);
    }
}

} // namespace uitk
