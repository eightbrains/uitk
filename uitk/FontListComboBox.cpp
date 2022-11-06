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

#include "FontListComboBox.h"

#include "Application.h"
#include "Label.h"
#include "UIContext.h"
#include "themes/Theme.h"

#include <nativedraw.h>

namespace uitk {

namespace {

class FontNameLabel : public Label
{
    using Super = Label;
public:
    FontNameLabel(const std::string& fontName)
        : Label(fontName)
    {
        setAlignment(Alignment::kLeft | Alignment::kVCenter);
    }

    void setUseThemeFont(bool use)
    {
        if (use != mUseThemeFont) {
            mUseThemeFont = use;
            mNeedsLayout = true;
        }
    }

    void draw(UIContext& context) override
    {
        if (mNeedsLayout) {
            auto fontName = text();
            if (mUseThemeFont) {
                setText(fontName);
            } else {
                auto labelFontSize = context.theme.params().labelFont.pointSize();
                auto labelFontMetrics = context.theme.params().labelFont.metrics(context.dc);
                Text t(fontName, Font(), textColor());
                Font f(fontName, labelFontSize);
                auto thisMetrics = f.metrics(context.dc);
                if (thisMetrics.lineHeight > PicaPt::kZero) {
                    // Taking the ratio of line heights rather than (ascent + descent) works better,
                    // even though we only have one line so leading is not used. In particular,
                    // a barcode font named "MICR E" looks really awful without it.
                    auto adjust = labelFontMetrics.lineHeight / thisMetrics.lineHeight;
                    f = Font(fontName, labelFontSize * adjust);
                    t.setFont(f);
                    setRichText(t);
                } else {
                    setText(fontName + " [error]");
                }
            }

            mNeedsLayout = false;
        }

        Super::draw(context);
    }

private:
    bool mUseThemeFont = true;
    bool mNeedsLayout = true;
};

} // namespace

//-----------------------------------------------------------------------------
struct FontListComboBox::Impl
{
    bool useThemeFont = true;
};

FontListComboBox::FontListComboBox()
    : FontListComboBox(Application::instance().availableFontFamilies())
{
}

FontListComboBox::FontListComboBox(const std::vector<std::string>& fontNames)
    : mImpl(new Impl())
{
    for (auto &fontName : fontNames) {
        addFont(fontName);
    }
    didHideMenu();  // make sure selected item is normal font
}

void FontListComboBox::addFont(const std::string& fontName)
{
    auto *item = new FontNameLabel(fontName);
    item->setUseThemeFont(mImpl->useThemeFont);
    addItem(item);
}

bool FontListComboBox::drawWithFont() const
{
    return mImpl->useThemeFont;
}

FontListComboBox* FontListComboBox::setDrawWithFont(bool with)
{
    mImpl->useThemeFont = !with;

    int n = size();
    for (int i = 0;  i < n;  ++i) {
        auto *item = static_cast<FontNameLabel*>(itemAtIndex(i));
        item->setUseThemeFont(!with);
    }
    didChangeSelection();
    setNeedsDraw();

    return this;
}

void FontListComboBox::willChangeSelection()
{
    auto *item = static_cast<FontNameLabel*>(itemAtIndex(selectedIndex()));
    if (item) {
        item->setUseThemeFont(mImpl->useThemeFont);
    }
}

void FontListComboBox::didChangeSelection()
{
    auto *item = static_cast<FontNameLabel*>(itemAtIndex(selectedIndex()));
    if (item) {
        item->setUseThemeFont(true);
    }
}

void FontListComboBox::willShowMenu()
{
    auto *item = static_cast<FontNameLabel*>(itemAtIndex(selectedIndex()));
    if (item) {
        item->setUseThemeFont(mImpl->useThemeFont);
    }
}

void FontListComboBox::didHideMenu()
{
    auto *item = static_cast<FontNameLabel*>(itemAtIndex(selectedIndex()));
    if (item) {
        item->setUseThemeFont(true);
    }
}

}  // namespace uitk
