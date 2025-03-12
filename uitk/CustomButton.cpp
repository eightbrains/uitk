//-----------------------------------------------------------------------------
// Copyright 2021 - 2024 Eight Brains Studios, LLC
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

#include "CustomButton.h"

#include "IconAndText.h"
#include "UIContext.h"
#include "themes/Theme.h"

#include <unordered_map>

namespace uitk {

namespace {
int makeStateKey(Button::MouseState state, bool isOn)
{
    return (int(state) | (isOn ? 0x80000000 : 0x0));
}
}  // namespace

struct CustomButton::Impl
{
    std::unordered_map<int, Color> foregroundColors;
};

CustomButton::CustomButton(const std::string& text)
    : Button(text), mImpl(new Impl())
{
}

CustomButton::CustomButton(Theme::StandardIcon icon)
    : Button(icon), mImpl(new Impl())
{
    cell()->setIconIsFullFrame(true);
}

CustomButton::CustomButton(const Theme::Icon& icon)
    : Button(icon), mImpl(new Impl())
{
    cell()->setIconIsFullFrame(true);
}

CustomButton::CustomButton(Theme::StandardIcon icon, const std::string& text)
    : Button(icon, text), mImpl(new Impl())
{
}

CustomButton::CustomButton(const Theme::Icon& icon, const std::string& text)
    : Button(icon, text), mImpl(new Impl())
{
}

Color CustomButton::foregroundColor(MouseState state, bool isOn /*= false*/) const
{
    auto it = mImpl->foregroundColors.find(makeStateKey(state, isOn));
    if (it != mImpl->foregroundColors.end()) {
        return it->second;
    } else {
        return Color(0, 0, 0, 0);
    }
}

CustomButton* CustomButton::setForegroundColor(MouseState state, const Color& color, bool isOn /*= false*/)
{
    mImpl->foregroundColors[makeStateKey(state, isOn)] = color;
    return this;
}

void CustomButton::layout(const LayoutContext& context)
{
    auto margins = context.theme.calcPreferredButtonMargins(context.dc, context.theme.params().labelFont);
    if (cell()->iconIsFullFrame()) {  // proxy value for only-icon
        auto m = std::max(margins.width, margins.height);
        margins = Size(m, m);
    }
    cell()->setFrame(bounds().insetted(margins.width, margins.height));
    Widget::layout(context);  // skip immediate parent (Button), since it is opinionated about the frame
}

void CustomButton::draw(UIContext& context)
{
    auto s = state();
    auto fg = foregroundColor(s, isOn());
    if ((s == MouseState::kMouseOver || s == MouseState::kMouseDown)
        && (fg.red() == 0.0f && fg.green() == 0.0f && fg.blue() == 0.0f && fg.alpha() == 0.0f))
    {
        auto normal = foregroundColor(MouseState::kNormal, isOn());
        if (normal.red() != 0.0f || normal.green() != 0.0f || normal.blue() != 0.0f || normal.alpha() != 0.0f) {
            if (context.theme.params().textColor.toGrey().red() > 0.5f) {  // dark mode
                if (s == MouseState::kMouseOver) {
                    fg = normal.lighter();
                } else {
                    fg = normal.lighter().lighter();
                }
            } else {  // light mode
                if (s == MouseState::kMouseOver) {
                    fg = normal.darker();
                } else {
                    fg = normal.darker().darker();
                }
            }
        }
    }
    Super::draw(context, fg);
}

}  // namespace uitk
