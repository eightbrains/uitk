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

#include "SearchBar.h"

#include "Icon.h"
#include "StringEdit.h"
#include "UIContext.h"
#include "themes/Theme.h"

namespace uitk {

struct SearchBar::Impl
{
    Icon *icon;  // reference; owned by parent as a child object
    StringEdit *edit;  // reference; owned by parent as a child object
};

SearchBar::SearchBar()
    : mImpl(new Impl())
{
    mImpl->icon = new Icon(Theme::StandardIcon::kSearch);
    addChild(mImpl->icon);  // we no longer own 'icon'

    mImpl->edit = new StringEdit();
    mImpl->edit->setBorderWidth(PicaPt::kZero);
    mImpl->edit->setBorderColor(Color::kTransparent);
    addChild(mImpl->edit);  // we no longer own 'edit'
}

SearchBar::~SearchBar()
{
}

Icon* SearchBar::icon() const
{
    return mImpl->icon;
}

const std::string& SearchBar::text() const
{
    return mImpl->edit->text();
}

SearchBar* SearchBar::setText(const std::string& text)
{
    mImpl->edit->setText(text);
    return this;
}

const std::string& SearchBar::placeholderText() const
{
    return mImpl->edit->placeholderText();
}

SearchBar* SearchBar::setPlaceholderText(const std::string& text)
{
    mImpl->edit->setPlaceholderText(text);
    return this;
}

int SearchBar::alignment() const
{
    return mImpl->edit->alignment();
}

SearchBar* SearchBar::setAlignment(int alignment)
{
    mImpl->edit->setAlignment(alignment);
    return this;
}

void SearchBar::setOnTextChanged(std::function<void(const std::string&)> onChanged)
{
    mImpl->edit->setOnTextChanged(onChanged);
}

void SearchBar::setOnValueChanged(std::function<void(SearchBar*)> onChanged)
{
    mImpl->edit->setOnValueChanged([this, onChanged](StringEdit*) {
        onChanged(this);
    });
}

Size SearchBar::preferredSize(const LayoutContext& context) const
{
    auto font = context.theme.params().labelFont;
    auto pref = Super::preferredSize(context);
    return Size(pref.width, context.theme.calcStandardHeight(context.dc, font));
}

void SearchBar::layout(const LayoutContext& context)
{
    // As of 2022 a search bar typically has a clear button, but not always.
    // The theme specifies this independently of the whether text editing uses
    // a clear button (which is much less frequently on desktops). Set this
    // value here, since a new theme will need to call layout.
    if (context.theme.params().useClearTextButtonForSearch) {
        mImpl->edit->setUseClearButton(StringEdit::UseClearButton::kYes);
    } else {
        mImpl->edit->setUseClearButton(StringEdit::UseClearButton::kNo);
    }

    // The StringEdit will have margins, so the icon needs to use the left
    // margin to balance the right.
    auto font = context.theme.params().labelFont;
    auto margins = context.theme.calcPreferredTextMargins(context.dc, font);
    auto &r = bounds();
    auto iconRect = context.theme.calcStandardIconRect(context.dc, r, font);
    iconRect.x = margins.width;
    iconRect.width = iconRect.height;
    auto x= iconRect.maxX();
    Rect editRect(x, r.y, r.width - x, r.height);

    mImpl->icon->setFrame(iconRect);
    mImpl->edit->setFrame(editRect);

    Super::layout(context);
}

void SearchBar::draw(UIContext& context)
{
    context.theme.drawSearchBar(context, bounds(), style(themeState()), themeState());
    Super::draw(context);
}


}  // namespace uitk
