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

#include "RadioButton.h"

#include "UIContext.h"

// Design notes:
// - This inherits from Checkbox because preferredSize/layout is the same.
// - Currently users must implement the radio button exclusivity manually.
//   Something like Qt's RadioGroup is a little awkward, and there are ownership
//   issues. It could be a VLayout that check for RadioButton children (or
//   has an addRadioButton() method), which would allow for easily adding items
//   underneath a button, but this would not allow for items on the same line
//   (unless we searched for RadioButton children recursively, into the HLayout
//   that would be used). But this would not work for RadioButtons in a GridLayout.
//   There is also the question of how to do it, since setOnClicked() can only
//   take one callback, and we need our callback plus the users. (This could be
//   accomplished by wrapping the users's, but it is awkward.) So, punt until
//   we can think of a better idea.

namespace uitk {

RadioButton::RadioButton(const std::string& text)
    : Checkbox(text)
{
}

RadioButton::~RadioButton()
{
}

AccessibilityInfo RadioButton::accessibilityInfo()
{
    auto info = Super::accessibilityInfo();
    info.type = AccessibilityInfo::Type::kRadioButton;
    info.value = isOn();
    return info;
}

Widget::EventResult RadioButton::mouse(const MouseEvent &e)
{
    // Can't uncheck a radio button except via a different one
    if (isOn()) {
        return Widget::EventResult::kIgnored;
    } else {
        return Super::mouse(e);
    }
}

Widget::EventResult RadioButton::key(const KeyEvent& e)
{
    // Can't uncheck a radio button except via a different one
    if (isOn()) {
        return Widget::EventResult::kIgnored;
    } else {
        return Super::key(e);
    }
}

void RadioButton::draw(UIContext& context)
{
    auto size = bounds().height;
    Rect r(bounds().x, bounds().y, size, size);
    context.theme.drawRadioBox(context, r, style(themeState()), themeState(), isOn());

    // Unsually, we don't want to Super::draw(), because Button changes
    // the text's color. So we're going to skip to Widget::draw().
    Widget::draw(context);
}

}  // namespace uitk
