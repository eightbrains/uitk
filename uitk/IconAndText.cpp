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

#include "IconAndText.h"

#include "Icon.h"
#include "Label.h"
#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk
{

struct IconAndText::Impl
{
    Label *label = nullptr;  // reference (Super owns it as child)
    Icon *icon = nullptr;  // reference (Super owns it as child)
    bool iconIsFullFrame = false;
};

IconAndText::IconAndText()
    : mImpl(new Impl())
{
    mImpl->label = new Label("");
    mImpl->label->setAlignment(Alignment::kLeft | Alignment::kVCenter);
    mImpl->icon = new Icon(Theme::StandardIcon::kNone);
    addChild(mImpl->icon);  // super owns now
    addChild(mImpl->label);  // super owns now
}

IconAndText::~IconAndText()
{
    // Superclass will delete the label and icon as children
}

Label* IconAndText::label() const
{
    return mImpl->label;
}

Icon* IconAndText::icon() const
{
    return mImpl->icon;
}

bool IconAndText::iconIsFullFrame() const { return mImpl->iconIsFullFrame; }

IconAndText* IconAndText::setIconIsFullFrame(bool isFull)
{
    mImpl->iconIsFullFrame = isFull;
    return this;
}

void IconAndText::setForegroundColorNoRedraw(const Color& c)
{
    mImpl->label->setForegroundColorNoRedraw(c);
    mImpl->icon->setForegroundColorNoRedraw(c);
}

Size IconAndText::preferredSize(const LayoutContext& context) const
{
    bool hasIcon = !mImpl->icon->isEmpty();
    bool hasText = !mImpl->label->text().empty();

    auto font = mImpl->label->font();
    auto width = mImpl->label->preferredSize(context).width;
    if (!hasText) {
        width = PicaPt::kZero;  // preferredSize() may not be zero because of margins
    }
    
    if (hasIcon) {
        width += context.theme.calcStandardIconSize(context.dc, font).width;

        auto textMargins = context.theme.calcPreferredTextMargins(context.dc, font);
        if (hasText) {
            // If there is text, only use the left margin; there is the right margin
            // of the label and a separator (which might be zero)
            width += textMargins.width + context.theme.calcStandardIconSeparator(context.dc, font);;
        } else {
            // If there is no text we need the left and the right margins
            width += 2.0f * textMargins.width;
        }
    }
    return Size(width, context.theme.calcStandardHeight(context.dc, font));
}

void IconAndText::layout(const LayoutContext& context)
{
    bool hasIcon = !mImpl->icon->isEmpty();
    bool hasText = !mImpl->label->text().empty();

    auto font = mImpl->label->font();
    auto width = mImpl->label->preferredSize(context).width;
    if (hasText && !hasIcon) {  // common case
        mImpl->icon->setVisible(false);
        mImpl->icon->setFrame(Rect::kZero);
        mImpl->label->setVisible(true);
        mImpl->label->setFrame(bounds());
    } else if (!hasText && hasIcon) {
        mImpl->icon->setVisible(true);
        if (mImpl->iconIsFullFrame) {
            mImpl->icon->setFrame(bounds());
        } else {
            mImpl->icon->setFrame(context.theme.calcStandardIconRect(context.dc, bounds(), font));
        }
        mImpl->label->setVisible(false);
        mImpl->label->setFrame(Rect::kZero);
    } else {  // uncommon case
        // The text will have text margins, so icon needs to have margins on the left
        // (since the text has them on the right), otherwise centering will be off.
        // And conveninently, it works as a good separator between the text.
        auto textMargins = context.theme.calcPreferredTextMargins(context.dc, font);
        auto iconRect = context.theme.calcStandardIconRect(context.dc, bounds(), font);
        iconRect.x = bounds().x + textMargins.width;
        auto x = iconRect.maxX() + context.theme.calcStandardIconSeparator(context.dc, font);
        auto labelRect = Rect(x, bounds().y,
                              std::min(width, bounds().width - x), bounds().height);
        mImpl->icon->setVisible(true);
        mImpl->icon->setFrame(iconRect);
        mImpl->label->setVisible(true);
        mImpl->label->setFrame(labelRect);
    }

    Super::layout(context);
}

void IconAndText::draw(UIContext& ui)
{
    auto themeState = this->themeState();
    mImpl->icon->setThemeState(themeState);
    mImpl->label->setThemeState(themeState);

    Super::draw(ui);
}

} // namespace uitk
